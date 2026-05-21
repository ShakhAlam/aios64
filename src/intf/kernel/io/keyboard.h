#pragma once

#define KEYBOARD_BUFFER_SIZE 256
extern char keyboard_buffer[KEYBOARD_BUFFER_SIZE];

extern int kb_buf_head;
extern int kb_buf_tail;


unsigned char getch();
char* k_fgets(char* buffer, int max_len) ;


char scancode_to_ascii(unsigned char scancode);

int read_scancode_blocking(void);
int read_scancode_nonblock(unsigned char* out_scancode);




