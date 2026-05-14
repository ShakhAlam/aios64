#pragma once

extern unsigned int back_buffer[];

extern unsigned long framebuffer_addr,
                     framebuffer_width,
                     framebuffer_height,
                     framebuffer_depth,
                     framebuffer_pitch;


void putpixel(unsigned int x, unsigned int y, unsigned int color,
              unsigned int* framebuffer_addr, unsigned int pitch);

void vsync();
void clear_screen(void* framebuffer_addr, unsigned int width, unsigned int height, 
                  unsigned int pitch, unsigned int color);
