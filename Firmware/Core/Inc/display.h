#ifndef DISPLAY_H
#define DISPLAY_H

void DrawHomePage(int8_t scroll);
void DrawMusicPlayerIcon();
void DrawMusicPlayerHomePage(uint8_t options);
void DrawAlarmIcon();
void DrawTimerIcon();
void DrawSongList(char* song_1[18], char* song_2[18], char* song_3[18]);
void DrawVolume(uint8_t volume);
void DrawTextToScreen(char text[20]);
void DisplayOptions(int8_t current_option, char option_1[20], char option_2[20], char option_3[20], char option_4[20]);
void DisplayOptionsTwo(int8_t current_option, char option_1[20], char option_2[20]);
void PageNotMade(char page[15]);



#define ALL_SONGS_OPTION 1
#define PLAYLISTS_OPTION 2
#define ARTISTS_OPTION 3
#define ALBUMS_OPTION 4

#endif