#include <stdint.h>

#include <stddef.h>

inline void vsync();

void clear_screen(void* framebuffer_addr, unsigned int width, unsigned int height, unsigned int pitch, unsigned int color) {
    size_t row_bytes = width * 4;             // 4 bytes per pixel
    size_t screen_bytes = height * pitch;

    // Optimization: we use rep stosq to write 8 bytes at a time (2 pixels)
    size_t qword_count = screen_bytes / 8;

    __asm__ __volatile__ (
        "cld\n\t"                  // Clear direction flag (forward)
        "rep stosq\n\t"           // Repeat store quadword
        :
        : "D"(framebuffer_addr),  // destination (rdi)
          "a"((unsigned long)color | ((unsigned long)color << 32)), // rax = color repeated
          "c"(qword_count)        // count (rcx)
        : "memory"
    );
}


inline void vsync() {
    __asm__ __volatile__ (
        "1:\n\t"
        "inb $0x3DA, %%al\n\t"       // Read VGA Input Status Register 1
        "test $0x08, %%al\n\t"       // Check Vertical Retrace bit (bit 3)
        "jz 1b\n\t"                  // Wait until set

        "2:\n\t"
        "inb $0x3DA, %%al\n\t"       // Read again
        "test $0x08, %%al\n\t"       // Wait for retrace to end
        "jnz 2b\n\t"                 // Wait until cleared
        :
        :
        : "al"
    );
}

static inline void fake_vsync_delay() {
    for (volatile int i = 0; i < 100000; i++) {
        __asm__ __volatile__("pause");
    }
}
