#include "print.h"
extern unsigned long framebuffer_addr,
                     framebuffer_width,
                     framebuffer_height,
                     framebuffer_depth;

void write_serial(char c) {
    __asm__ volatile (
        "movb %0, %%al \n\t"  // Load character into AL
        "movw $0x3F8, %%dx \n\t"  // Load COM1 port address into DX
        "outb %%al, %%dx \n\t"  // Output character to COM1
        :
        : "r"(c)  // Input operand: character to print
        : "al", "dx"  // Clobbered registers
    );
}

#define RED 0xff0000
#define GREEN 0xff00
#define BLUE 0xff
#define YELLOW 0xffff00

static uint32_t seed = 0x12345678;

uint32_t random24() {
    // Linear Congruential Generator (LCG) parameters
    seed = seed * 1664525 + 1013904223;

    // Mask to keep only the lowest 24 bits
    return seed & 0xFFFFFF;
}

void strprint_serial(char *str){
    while(*str++){
        write_serial(*str);
    }
}

void hexprint_serial(unsigned long n){
    for(int x=sizeof(n)-1; x >= 0; x--){
        char ch = (char)(((n >> (x*4)) & 0xf));
        ch = (ch > 10) ? ch + 55 : ch + 48;
        write_serial((char)ch);
    }
    write_serial((char)'\n');
}

void kernel_main(){

    /* output to serial port */
    strprint_serial("\n\n**** In 64 bit Kernel! ****\nFramebuffer Address: 0x");
    hexprint_serial(framebuffer_addr);

    strprint_serial("\nWidth: 0x");
    hexprint_serial(framebuffer_width);
    

    strprint_serial("\nHeight: 0x");
    hexprint_serial(framebuffer_height);
    

    strprint_serial("\nDepth: 0x");
    hexprint_serial(framebuffer_depth);

    /*for(int x=7; x >= 0; x--){
        char ch = (char)(((framebuffer_addr >> (x*4)) & 0xf));
        ch = (ch > 10) ? ch + 55 : ch + 48;
        write_serial((char)ch);
    }*/

    write_serial((char)'\n');


    /* output to framebuffer */
    while(1)
    for (int x = 0; x<0x2FFFF; x++){

        //write_serial('B');
        ((unsigned long*)(framebuffer_addr))[x] = random24();
    }

    
    /* output to CGA*/
    print_clear();
    print_set_color(PRINT_COLOR_LIGHT_BLUE, PRINT_COLOR_BLACK);
    print_str("Welcome to AIOS!");
    

}