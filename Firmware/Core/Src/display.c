#include <string.h>
#include <stdio.h>
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include "ssd1306_fonts.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static const char* song_name = "Thriller";

void DrawHomePage(int8_t scroll) {
    uint8_t y = 0;
    char volume[8];
    ssd1306_Fill(Black);
    sprintf(volume, "%d", scroll);

    //Time and Battery
    #ifdef SSD1306_INCLUDE_FONT_6x8
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("9:41pm", Font_6x8, White); // TODO: Change to actual time

    ssd1306_SetCursor(75, y);
    ssd1306_WriteString("Battery", Font_6x8, White); // TODO: Change to Battery Symbol
    y += 8;
    #endif

    //Song and Artist
    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString(song_name, Font_7x10, White); // TODO: Change to live song name
    y += 10;
    
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Michael Jackson", Font_7x10, White); // TODO: Change to live artist name
    y += 10;
    #endif
    
    //Additional Status Icons
    #ifdef SSD1306_INCLUDE_FONT_6x8
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString(volume, Font_6x8, White); //TODO: Symbols that update here dynamically
    #endif

    //Update the damn screen
    ssd1306_UpdateScreen();
}

void DrawMusicPlayerIcon() {
    ssd1306_Fill(Black);

    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(35, 35);
    ssd1306_WriteString("Music Icon Here", Font_7x10, Black); // TODO: Change to music icon
    #endif

    ssd1306_UpdateScreen();
}

void DrawMusicPlayerHomePage_AllSongsSelected() {
    uint8_t y=2;
    ssd1306_Fill(Black);

    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("-", Font_7x10, Black); // TODO: Change to music icon

    ssd1306_SetCursor(9, y);
    ssd1306_WriteString("All Songs", Font_7x10, Black); // TODO: Change to music icon
    y+=10;

    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Playlists", Font_7x10, Black); // TODO: Change to music icon
    y+=10;

    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Albums", Font_7x10, Black); // TODO: Change to music icon

    #endif
    ssd1306_UpdateScreen();
}

void DrawMusicPlayerHomePage_PlaylistsSelected() {
    uint8_t y=2;
    ssd1306_Fill(Black);

    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("All Songs", Font_7x10, Black); // TODO: Change to music icon
    y+=10;

    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("-", Font_7x10, Black); // TODO: Change to music icon
    ssd1306_SetCursor(9, y);
    ssd1306_WriteString("Playlists", Font_7x10, Black); // TODO: Change to music icon
    y+=10;

    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Albums", Font_7x10, Black); // TODO: Change to music icon

    #endif
    ssd1306_UpdateScreen();
}

void DrawMusicPlayerHomePage_AlbumsSelected() {
    uint8_t y=2;
    ssd1306_Fill(Black);

    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("All Songs", Font_7x10, Black); // TODO: Change to music icon
    y+=10;

    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Playlists", Font_7x10, Black); // TODO: Change to music icon
    y+=10;

    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("-", Font_7x10, Black); // TODO: Change to music icon
    ssd1306_SetCursor(9, y);
    ssd1306_WriteString("Albums", Font_7x10, Black); // TODO: Change to music icon

    #endif
    ssd1306_UpdateScreen();
}

void DrawSongList(char* song_1[18], char* song_2[18], char* song_3[18]) {
    uint8_t y=2;
    ssd1306_Fill(Black);

    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString(*song_1, Font_7x10, Black);
    y+=10;

    ssd1306_SetCursor(2, y);
    ssd1306_WriteString(*song_2, Font_7x10, Black); 
    y+=10;

    ssd1306_SetCursor(2, y);
    ssd1306_WriteString(*song_3, Font_7x10, Black); 

    #endif
    ssd1306_UpdateScreen();
}

void DrawAlarmIcon() {
    ssd1306_Fill(Black);
    
    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(35, 35);
    ssd1306_WriteString("Alarm Icon Here", Font_7x10, Black); // TODO: Change to alarm icon
    #endif
    ssd1306_UpdateScreen();
}

void DrawTimerIcon() {
    ssd1306_Fill(Black);
    
    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(35, 35);
    ssd1306_WriteString("Timer Icon Here", Font_7x10, Black); // TODO: Change to Timer icon
    #endif
    ssd1306_UpdateScreen();
}