/* music_player.h
 *
 * STM32L496 MP3 Player — Header
 * Decodes MP3 from SD card via FATFS + minimp3,
 * outputs 16-bit PCM over SAI (I2S) to PCM5102A via DMA ping-pong.
 */

#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include "stm32l4xx_hal.h"
#include "fatfs.h"
#include <stdint.h>

/* -----------------------------------------------------------------------
 * Configuration — adjust to match your CubeMX setup
 * ----------------------------------------------------------------------- */

/* Handle for the SAI block you configured (e.g. hsai_BlockA1 or hsai_BlockB1).
 * Declare it extern here; CubeMX defines it in sai.c. */
extern SAI_HandleTypeDef hsai_BlockA1;   // <-- change to match yours
#define MUSIC_SAI_HANDLE    hsai_BlockA1

/* FATFS read buffer size — must be large enough to hold ~10 MP3 frames.
 * 16KB is the recommended minimum from the minimp3 docs. */
#define MUSIC_READ_BUF_SIZE     (16 * 1024)

/* DMA audio buffer: two halves (ping + pong).
 * Each half holds one decoded MP3 frame worth of samples.
 * MINIMP3_MAX_SAMPLES_PER_FRAME = 1152 samples * 2 channels = 2304 shorts. */
#define MUSIC_PCM_HALF_SIZE     (1152*2)      /* 2304 shorts — stereo frame */
#define MUSIC_PCM_BUF_SIZE      (MUSIC_PCM_HALF_SIZE * 2)  /* 4608 total */

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Initialise the music player. Call once after FATFS and SAI are ready.
 */
void MusicPlayer_Init(void);

/**
 * @brief  Open an MP3 file and start playback.
 * @param  path  FatFS path, e.g. "0:/track01.mp3"
 * @retval 0 on success, non-zero on error (file not found, etc.)
 */
int MusicPlayer_Play(const char *path);

/**
 * @brief  Stop playback and close the file.
 */
void MusicPlayer_Stop(void);

/**
 * @brief  Pause / resume toggle.
 */
void MusicPlayer_Pause(void);
void MusicPlayer_Resume(void);

/**
 * @brief  Returns 1 if audio is currently playing.
 */
uint8_t MusicPlayer_IsPlaying(void);

/**
 * @brief  Call this from your main loop (or a low-priority task).
 *         It refills the decode buffer and decodes frames into the
 *         SAI DMA ping-pong buffers as needed.
 *
 *         Do NOT call from an ISR.
 */
void MusicPlayer_Process(void);

/* -----------------------------------------------------------------------
 * SAI DMA callbacks — wire these up in stm32l4xx_it.c or sai.c
 * ----------------------------------------------------------------------- */

/**
 * @brief  Call from HAL_SAI_TxHalfCpltCallback — ping buffer exhausted.
 */
void MusicPlayer_SAI_HalfCpltCallback(void);

/**
 * @brief  Call from HAL_SAI_TxCpltCallback — pong buffer exhausted.
 */
void MusicPlayer_SAI_CpltCallback(void);

#endif /* MUSIC_PLAYER_H */