#pragma once

#define KEYBOARD_BUFFER_SIZE 256
extern char keyboard_buffer[KEYBOARD_BUFFER_SIZE];

extern int kb_buf_head;
extern int kb_buf_tail;


unsigned char getch();
char* k_fgets(char* buffer, int max_len) ;

