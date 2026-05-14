#pragma once
void print_xy(int x, int y, const char *str, unsigned int color);
void draw_char(int x, int y, char c, unsigned int color, unsigned int bgcolor);
extern unsigned short  ascii_16x26_font[94][26];
extern unsigned short ascii_7x11_font[94][11];
extern unsigned char ascii_8x13_font[95][13];
