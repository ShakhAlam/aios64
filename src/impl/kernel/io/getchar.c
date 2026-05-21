#include "kernel/io/keyboard.h"

char getchar(void) {
    int scancode = read_scancode_blocking();
    return scancode_to_ascii(scancode);
}

int getchar_nonblock(char* out_char) {
    unsigned char scancode;
    if (!read_scancode_nonblock(&scancode)) return 0;

    *out_char = scancode_to_ascii(scancode);
    return 1;
}
