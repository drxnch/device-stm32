/* music_player.c
 *
 * STM32L496 MP3 Player — Implementation
 *
 * Pipeline:
 *   SD Card → FATFS f_read() → mp3_read_buf[]
 *        → mp3dec_decode_frame() → pcm_dma_buf[] (ping-pong)
 *        → SAI DMA (circular) → PCM5102A
 *
 * SAI DMA runs in CIRCULAR mode over pcm_dma_buf[MUSIC_PCM_BUF_SIZE].
 * The DMA fires a half-complete IRQ when it finishes the first half (ping),
 * and a full-complete IRQ when it finishes the second half (pong).
 * We decode one MP3 frame into whichever half just finished playing.
 *
 * NOTE: You must define MINIMP3_IMPLEMENTATION in exactly ONE .c file.
 *       This file does that — do not add it anywhere else.
 */

/* -----------------------------------------------------------------------
 * minimp3 — single-header implementation
 * ----------------------------------------------------------------------- */
#include "display.h"
#include <stdio.h>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD      /* Cortex-M4 has no SSE/NEON via this path    */
#define MINIMP3_ONLY_MP3     /* strip MP1/MP2 to save ~4KB of flash         */
#include "minimp3.h"
#include "display.h"
#include "ssd1306.h"

/* -----------------------------------------------------------------------
 * Standard includes
 * ----------------------------------------------------------------------- */
#include "music_player.h"
#include "fatfs.h"
#include <string.h>   /* memmove, memset */
#include <stdint.h>

/* -----------------------------------------------------------------------
 * Internal state
 * ----------------------------------------------------------------------- */

/* FATFS file handle */
static FIL      s_file;
static uint8_t  s_file_open = 0;

/* Raw MP3 byte buffer — fed from SD card */
static uint8_t  s_read_buf[MUSIC_READ_BUF_SIZE];
static uint32_t s_read_buf_len = 0;   /* valid bytes currently in s_read_buf */
static uint8_t  s_eof = 0;            /* set when f_read returns 0 bytes     */

/* minimp3 decoder state */
static mp3dec_t s_mp3dec;
static mp3dec_frame_info_t s_frame_info;

/* DMA ping-pong audio buffer (SAI transmits from this in circular DMA mode) */
static int16_t  s_pcm_dma_buf[MUSIC_PCM_BUF_SIZE];  /* ping: [0..half-1]  */
                                                      /* pong: [half..end-1]*/

/* Flags set from ISR, consumed in MusicPlayer_Process() */
static volatile uint8_t s_fill_ping = 0;   /* 1 → refill first  half */
static volatile uint8_t s_fill_pong = 0;   /* 1 → refill second half */

/* Player state */
typedef enum {
    STATE_IDLE    = 0,
    STATE_PLAYING,
    STATE_PAUSED,
} PlayerState_t;

static PlayerState_t s_state = STATE_IDLE;

/* -----------------------------------------------------------------------
 * Private helpers
 * ----------------------------------------------------------------------- */

/**
 * @brief  Top up s_read_buf from the SD card until it's full or EOF.
 */
static void prv_FillReadBuffer(void)
{
    if (s_eof || !s_file_open) return;

    uint32_t space = MUSIC_READ_BUF_SIZE - s_read_buf_len;
    if (space == 0) return;

    UINT bytes_read = 0;
    FRESULT res = f_read(&s_file, s_read_buf + s_read_buf_len, space, &bytes_read);

    if (res != FR_OK || bytes_read == 0) {
        s_eof = 1;
    } else {
        s_read_buf_len += bytes_read;
    }
}

/**
 * @brief  Consume `n` bytes from the front of s_read_buf.
 */
static void prv_ConsumeReadBuffer(uint32_t n)
{
    if (n == 0 || n > s_read_buf_len) return;
    memmove(s_read_buf, s_read_buf + n, s_read_buf_len - n);
    s_read_buf_len -= n;
}

/**
 * @brief  Decode one MP3 frame into `dest` (MUSIC_PCM_HALF_SIZE shorts).
 *         If there aren't enough samples to fill the half-buffer (e.g. the
 *         decoded frame has fewer samples), the remainder is zero-padded
 *         to avoid DMA starving with stale audio.
 *
 *         Returns 0 on success, -1 if we've exhausted the stream.
 */
static int prv_DecodeFrameInto(int16_t *dest)
{
    /* Zero the destination first — handles padding + silence on EOF */
    memset(dest, 0, MUSIC_PCM_HALF_SIZE * sizeof(int16_t));

    uint32_t written = 0;

    while (written < MUSIC_PCM_HALF_SIZE) {

        /* Refill the read buffer if it's running low */
        if (s_read_buf_len < (MUSIC_READ_BUF_SIZE / 2)) {
            prv_FillReadBuffer();
        }

        /* Nothing left to decode */
        if (s_read_buf_len == 0 && s_eof) {
            return (written == 0) ? -1 : 0;   /* -1 only if we got nothing */
        }

        int16_t frame_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        int samples = mp3dec_decode_frame(
                          &s_mp3dec,
                          s_read_buf,
                          (int)s_read_buf_len,
                          frame_pcm,
                          &s_frame_info);

        if (s_frame_info.frame_bytes == 0) {
            /* Insufficient data — shouldn't happen with 16KB buffer, but
               guard against it by treating as EOF */
            s_eof = 1;
            break;
        }

        /* Always consume frame_bytes, even when samples == 0 (ID3 skip) */
        prv_ConsumeReadBuffer(s_frame_info.frame_bytes);

        if (samples <= 0) {
            if (samples > 0 && s_frame_info.hz != 0) {
            // char sr[32];
            // sprintf(sr, "SR:%d CH:%d", s_frame_info.hz, s_frame_info.channels);
            // DrawTextToScreen(sr);
            // HAL_Delay(3000);
}
            /* ID3 tag or invalid frame skipped — try next frame */
            continue;
        }

        /* Copy decoded samples into dest, but don't overflow the half-buffer */
        uint32_t to_copy = (uint32_t)samples;
        if (written + to_copy > MUSIC_PCM_HALF_SIZE) {
            to_copy = MUSIC_PCM_HALF_SIZE - written;
        }

        memcpy(dest + written, frame_pcm, to_copy * sizeof(int16_t));
        written += to_copy;
    }

    return 0;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void MusicPlayer_Init(void)
{
    mp3dec_init(&s_mp3dec);
    memset(s_pcm_dma_buf, 0, sizeof(s_pcm_dma_buf));
    s_state        = STATE_IDLE;
    s_file_open    = 0;
    s_read_buf_len = 0;
    s_eof          = 0;
    s_fill_ping    = 0;
    s_fill_pong    = 0;
}

int MusicPlayer_Play(const char *path)
{
    /* Stop anything currently playing */
    MusicPlayer_Stop();

    static FATFS s_fatfs;
    FRESULT mres = f_mount(&s_fatfs, "0:", 1);
    if (mres != FR_OK) {
        char err[24];
        sprintf(err, "Mount fail: %d", mres);
        DrawTextToScreen(err);
        return -2;
    }
    /* Open the file */
    FRESULT res = f_open(&s_file, path, FA_READ);
    if (res != FR_OK) {
        DrawTextToScreen("Didn't Find Song");
        return -1;  /* File not found or FATFS error */
    }
    s_file_open = 1;
    s_eof       = 0;
    s_read_buf_len = 0;

    char err[20];

    /* Re-initialise decoder for the new track */
    mp3dec_init(&s_mp3dec);

    /* Pre-fill the read buffer */
    prv_FillReadBuffer();

    /* Pre-decode two half-buffers so DMA has something to play immediately */
    prv_DecodeFrameInto(&s_pcm_dma_buf[0]);                     /* ping */
    prv_DecodeFrameInto(&s_pcm_dma_buf[MUSIC_PCM_HALF_SIZE]);   /* pong */

    s_fill_ping = 0;
    s_fill_pong = 0;
    s_state     = STATE_PLAYING;

    /* Start SAI DMA in CIRCULAR mode — it will fire half/full callbacks
     * automatically as each half is consumed by the DAC.
     *
     * Size parameter is in HALFWORDS (uint16_t) for 16-bit I2S. */
HAL_StatusTypeDef sai_status = HAL_SAI_Transmit_DMA(&MUSIC_SAI_HANDLE,
                                 (uint8_t *)s_pcm_dma_buf,
                                 MUSIC_PCM_BUF_SIZE);
char dbg[20];
sprintf(dbg, "SAI:%d", sai_status);
DrawTextToScreen(dbg);
HAL_Delay(2000);
    return 0;
}

void MusicPlayer_Stop(void)
{
    if (s_state == STATE_IDLE) return;

    HAL_SAI_DMAStop(&MUSIC_SAI_HANDLE);

    if (s_file_open) {
        f_close(&s_file);
        s_file_open = 0;
    }

    s_state        = STATE_IDLE;
    s_eof          = 0;
    s_read_buf_len = 0;
    s_fill_ping    = 0;
    s_fill_pong    = 0;

    memset(s_pcm_dma_buf, 0, sizeof(s_pcm_dma_buf));
}

void MusicPlayer_Pause(void)
{
    if (s_state != STATE_PLAYING) return;
    HAL_SAI_DMAPause(&MUSIC_SAI_HANDLE);
    s_state = STATE_PAUSED;
}

void MusicPlayer_Resume(void)
{
    if (s_state != STATE_PAUSED) return;
    HAL_SAI_DMAResume(&MUSIC_SAI_HANDLE);
    s_state = STATE_PLAYING;
}

uint8_t MusicPlayer_IsPlaying(void)
{
    return (s_state == STATE_PLAYING) ? 1 : 0;
}

void MusicPlayer_Process(void)
{
    if (s_state != STATE_PLAYING) return;

    /* Proactively refill read buffer every loop — not just during decode */
    prv_FillReadBuffer();

    if (s_fill_ping) {
        s_fill_ping = 0;
        if (prv_DecodeFrameInto(&s_pcm_dma_buf[0]) < 0) {
            MusicPlayer_Stop();
            return;
        }
    }

    if (s_fill_pong) {
        s_fill_pong = 0;
        if (prv_DecodeFrameInto(&s_pcm_dma_buf[MUSIC_PCM_HALF_SIZE]) < 0) {
            MusicPlayer_Stop();
            return;
        }
    }
}

/* -----------------------------------------------------------------------
 * SAI DMA callbacks
 *
 * Wire these up in stm32l4xx_it.c (or wherever CubeMX put the SAI IRQ)
 * by calling them from the HAL weak callbacks:
 *
 *   void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai) {
 *       MusicPlayer_SAI_HalfCpltCallback();
 *   }
 *   void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai) {
 *       MusicPlayer_SAI_CpltCallback();
 *   }
 *
 * ----------------------------------------------------------------------- */

void MusicPlayer_SAI_HalfCpltCallback(void)
{
    /* DMA finished playing the PING half — signal main loop to refill it */
    s_fill_ping = 1;
}

void MusicPlayer_SAI_CpltCallback(void)
{
    /* DMA finished playing the PONG half — signal main loop to refill it */
    s_fill_pong = 1;
}