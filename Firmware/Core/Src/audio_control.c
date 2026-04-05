#include <stdint.h>

static int8_t music_volume = 50;  // private to this file

//Volume Flag
volatile bool g_vol_up_pressed = false;
volatile bool g_vol_down_pressed = false;

// Volume Buttons
ButtonState vol_up_btn;
ButtonState vol_down_btn;

void AudioControl_SetVolume(int8_t new_volume) {
    if (new_volume > 100) new_volume = 100;
    if (new_volume < 0) new_volume = 0;
    music_volume = new_volume;
}

void AudioControl_AdjustVolume(int8_t delta) {
    AudioControl_SetVolume(music_volume + delta);
}