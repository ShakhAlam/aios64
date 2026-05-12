// draw_char.c

#include "ascii_font.h"
#include "framebuffer.h"
#include <stdarg.h>


void putpixel(unsigned int x, unsigned int y, unsigned int color,
    unsigned char* framebuffer_addr, unsigned int pitch);


    // Draw a single character on the screen at position (x, y) with the given color

void draw_char(int x, int y, char c, unsigned int color) {
    if (c < 32 || c > 127) return;  // Only print printable ASCII chars

    const unsigned char *glyph = ascii_8x13_font[c - 32];  // Get the font bitmap for the character

    // Iterate over the 8 rows of the character's bitmap
    for (int row = 0; row < 13; row++) {
        unsigned short bits = glyph[row];  // Each row of the font (8 bits)

        // Iterate over the 8 columns (pixels per row)
        for (int col = 0; col < 8; col++) {
            // Check if the bit at position `col` in the row is set
            if (bits & (1  << col )) {  // Bits are stored from MSB to LSB
                int px = (x + (9-col))*1;  // Calculate the x position
                int py = (y + (14-row))*1;  // Calculate the y position
 
                // Calculate the pixel index in the buffer using pitch
                int index = (py * framebuffer_pitch) + (px * 4);  // 4 bytes per pixel (BGRA format)
                //printk("-->glyph: %x row: %d col: %d %d %d %d\n",c,row, col, px,py,index);
                // Set the pixel to the specified color (BGRA format)
               // back_buffer[index + 0] = 0xFF; //(color >>  0) & 0xFF; // Blue
               // back_buffer[index + 1] = (color >>  8) & 0xFF; // Green
               // back_buffer[index + 2] = (color >> 16) & 0xFF; // Red
               // back_buffer[index + 3] = 0xFF;                 // Alpha

                putpixel(px,py, color,(unsigned char*) back_buffer, framebuffer_pitch);
                putpixel(px-1,py, color,(unsigned char*) back_buffer, framebuffer_pitch);
                putpixel(px,py-1, color,(unsigned char*) back_buffer, framebuffer_pitch);
                              
            }
        }
    }
}

// Draw a single character on the screen at position (x, y) with the given color
void print_xy(int x, int y, const char *str, unsigned int color) {
    int cursor_x = x;
    int cursor_y = y;
    int counter = 0;
    while (*str) {
        if (*str == '\n' || counter > 80) {
            cursor_x = x;
            cursor_y += 15; // Move to the next row (height of the font)
            counter = 0;
        } else {
            draw_char(cursor_x, cursor_y, *str, color);
            cursor_x += 10; // Move to the next character
        }
        str++;
        counter++;
    }
}



void printf_gfx(int x, int y, unsigned int color, const char *fmt, ...) {
    char buffer[1024];
    char *buf_ptr = buffer;

    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    *buf_ptr++ = c;
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    if (num < 0) {
                        *buf_ptr++ = '-';
                        num = -num;
                    }
                    char temp[16];
                    int i = 0;
                    do {
                        temp[i++] = '0' + (num % 10);
                        num /= 10;
                    } while (num > 0);
                    while (i--) *buf_ptr++ = temp[i];
                    break;
                }
                case 'l': {
                    fmt++; // check for 'u' in %lu
                    if (*fmt == 'u') {
                        unsigned long num = va_arg(args, unsigned long);
                        char temp[32];
                        int i = 0;
                        do {
                            temp[i++] = '0' + (num % 10);
                            num /= 10;
                        } while (num > 0);
                        while (i--) *buf_ptr++ = temp[i];
                    }
                    break;
                }
                case 'x': {
                    unsigned int num = va_arg(args, unsigned int);
                    char hex_chars[] = "0123456789abcdef";
                    char temp[16];
                    int i = 0;
                    do {
                        temp[i++] = hex_chars[num % 16];
                        num /= 16;
                    } while (num > 0);
                    *buf_ptr++ = '0';
                    *buf_ptr++ = 'x';
                    while (i--) *buf_ptr++ = temp[i];
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, const char *);
                    while (*str) {
                        *buf_ptr++ = *str++;
                    }
                    break;
                }
                default:
                    *buf_ptr++ = '%';
                    *buf_ptr++ = *fmt;
            }
        } else {
            *buf_ptr++ = *fmt;
        }
        fmt++;
    }

    *buf_ptr = '\0';
    va_end(args);

    // draw formatted string using print_xy
    print_xy(x, y, buffer, color);
}


#define CHAR_WIDTH 10
#define CHAR_HEIGHT 14
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

static int term_cursor_x = 0;
static int term_cursor_y = 0;

void scroll_backbuffer(unsigned char *back_buffer,
                       int framebuffer_width,
                       int framebuffer_height,
                       int framebuffer_pitch,
                       int char_height)
{
    int rows_to_scroll = char_height;
    int bytes_per_row = framebuffer_pitch;

    int srcY, srcX, dstX, dstY;

    for(srcY = 1080; srcY > rows_to_scroll+200; srcY--){
        for (srcX = 0; srcX < 1920; srcX++){

            dstX = srcX;
            dstY = srcY - rows_to_scroll;

            unsigned int *dst = ((unsigned int*)back_buffer) + ((dstY * 1920) + dstX);
            unsigned int *src = ((unsigned int*)back_buffer) + ((srcY * 1920) + srcX);

            *dst = *src;

        }
    }

    /*
    // Move framebuffer content up
    for (int y = 0; y < framebuffer_height - rows_to_scroll; y++)
    {
        unsigned char *dst = back_buffer + y * framebuffer_pitch;
        unsigned char *src = back_buffer + (y + rows_to_scroll) * framebuffer_pitch;

        for (int i = 0; i < bytes_per_row; i++)
        {
           // dst[i] = src[i];
        }
    }
    */
    // Clear the last `char_height` rows (black out)
    /*for (int y = framebuffer_height - rows_to_scroll; y < framebuffer_height; y++)
    {
        unsigned char *line = back_buffer + y * framebuffer_pitch;
        for (int i = 0; i < bytes_per_row; i++)
        {
            line[i] = 0;
        }
    }*/
}

void scroll_backbuffer_1(unsigned char* back_buffer,
    int framebuffer_width,
    int framebuffer_height,
    int framebuffer_pitch,
    int char_height) {
    int rows_to_scroll = char_height;
    int bytes_per_row = framebuffer_pitch;

    for (int y = 0; y < framebuffer_height - rows_to_scroll; y++) {
        unsigned char* dst = back_buffer + y * framebuffer_pitch;
        unsigned char* src = back_buffer + (y + rows_to_scroll) * framebuffer_pitch;

        __asm__ __volatile__ (
        "rep movsb"
        :
        : "c"(bytes_per_row),
        "S"(src),
        "D"(dst)
        : "memory", "cc"
        );
    }

    // Clear the last `char_height` lines at the bottom
    /*for (int y = framebuffer_height - rows_to_scroll; y < framebuffer_height; y++) {
        unsigned char* line = back_buffer + y * framebuffer_pitch;

        __asm__ __volatile__ (
        "xor %%rax, %%rax\n"
        "rep stosq"
        :
        : "D"(line),
        "c"(framebuffer_pitch / 8)
        : "rax", "memory", "cc"
        );
    }*/
}
int vsnprintf(char *buffer, int bufsize, const char *fmt, va_list args) {
    int pos = 0;

    while (*fmt && pos < bufsize - 1) {
        if (*fmt == '%') {
            fmt++;
            if (!*fmt) break;

            if (*fmt == 'c') {
                buffer[pos++] = (char)va_arg(args, int);
            } else if (*fmt == 's') {
                const char *s = va_arg(args, const char*);
                while (*s && pos < bufsize - 1) buffer[pos++] = *s++;
            } else if (*fmt == 'd') {
                int val = va_arg(args, int);
                if (val < 0) {
                    buffer[pos++] = '-';
                    val = -val;
                }
                char tmp[16];
                int i = 0;
                do {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                } while (val && i < 16);
                while (i-- && pos < bufsize - 1) buffer[pos++] = tmp[i];
            } else if (*fmt == 'x') {
                unsigned int val = va_arg(args, unsigned int);
                char hex[] = "0123456789abcdef";
                char tmp[16];
                int i = 0;
                do {
                    tmp[i++] = hex[val & 0xF];
                    val >>= 4;
                } while (val && i < 16);
                while (i-- && pos < bufsize - 1) buffer[pos++] = tmp[i];
            } else if (*fmt == 'l' && *(fmt + 1) == 'u') {
                fmt++;
                unsigned long val = va_arg(args, unsigned long);
                char tmp[21];
                int i = 0;
                do {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                } while (val && i < 21);
                while (i-- && pos < bufsize - 1) buffer[pos++] = tmp[i];
            }
        } else {
            buffer[pos++] = *fmt;
        }
        fmt++;
    }

    buffer[pos] = '\0';
    return pos;
}



void printf_gfx_term(unsigned int color, const char *fmt, ...) {
    static int term_x = 10;
    static int term_y = 60;

    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // Calculate how many rows of space needed
    const int char_width = 20;
    const int char_height = 20;
    const int screen_width = 1920;
    const int screen_height = 1080;

    // Wrap to next line when needed
    for (const char *p = buf; *p; p++) {
        if (*p == '\n' || term_x + char_width > screen_width) {
            term_x = 0;
            term_y += char_height;

            if (term_y + char_height >= screen_height) {
                scroll_backbuffer(back_buffer,framebuffer_width,
                    framebuffer_height,framebuffer_pitch, char_height);  // Your own scroll function
                term_y -= char_height;
                //term_y = 20;
                printf_gfx_term(0xFFFFFFFF,"Scrolling!");
            }

            if (*p == '\n') continue;
        }

        draw_char(term_x, term_y, *p, color);
        term_x += char_width;
    }
}
