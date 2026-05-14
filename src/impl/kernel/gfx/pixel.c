#include "framebuffer.h"

void putpixel(unsigned int x, unsigned int y, unsigned int color,
    unsigned int* framebuffer_addr, unsigned int pitch) {

    unsigned int offset = (y * 1920) + x ;
    unsigned int* pixel = framebuffer_addr + offset;
    *pixel = color;        
}