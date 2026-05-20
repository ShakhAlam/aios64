#include <stdint.h>

void io_wait(void) {
    __asm__ __volatile__ (
        "outb %%al, $0x80"
        :
        : "a"(0)
    );
}


