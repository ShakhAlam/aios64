#include "print.h"
#include "ascii_font.h"

#include <stddef.h>

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


static inline void vsync() {
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

//#include <stdint.h>

#define PI 3.141592653589793
#define TWO_PI 6.283185307179586

double cosine(double x) {
    // Normalize x to range [-PI, PI]
    while (x > PI)  x -= TWO_PI;
    while (x < -PI) x += TWO_PI;

    double term = 1.0;
    double sum = 1.0;
    double x2 = x * x;

    for (int i = 1; i <= 10; ++i) {
        term *= -x2 / ((2 * i - 1) * (2 * i));
        sum += term;
    }

    return sum;
}


#define DEG_TO_RAD (PI / 180.0)

static const double cos_lut[360] = {
    1.000000, 0.999848, 0.999391, 0.998630, 0.997564, 0.996195, 0.994522, 0.992546, 0.990268, 0.987688,
    0.984808, 0.981627, 0.978148, 0.974370, 0.970296, 0.965926, 0.961262, 0.956305, 0.951057, 0.945519,
    0.939693, 0.933580, 0.927184, 0.920505, 0.913545, 0.906308, 0.898794, 0.891007, 0.882948, 0.874620,
    0.866025, 0.857167, 0.848048, 0.838671, 0.829038, 0.819152, 0.809017, 0.798636, 0.788011, 0.777146,
    0.766044, 0.754710, 0.743145, 0.731354, 0.719340, 0.707107, 0.694658, 0.681998, 0.669131, 0.656059,
    0.642788, 0.629321, 0.615661, 0.601815, 0.587785, 0.573576, 0.559193, 0.544639, 0.529919, 0.515038,
    0.500000, 0.484810, 0.469472, 0.453990, 0.438371, 0.422618, 0.406737, 0.390731, 0.374607, 0.358368,
    0.342020, 0.325568, 0.309017, 0.292372, 0.275637, 0.258819, 0.241921, 0.224951, 0.207912, 0.190809,
    0.173648, 0.156434, 0.139173, 0.121869, 0.104528, 0.087156, 0.069756, 0.052336, 0.034899, 0.017452,
    0.000000, -0.017452, -0.034899, -0.052336, -0.069756, -0.087156, -0.104528, -0.121869, -0.139173, -0.156434,
    -0.173648, -0.190809, -0.207912, -0.224951, -0.241921, -0.258819, -0.275637, -0.292372, -0.309017, -0.325568,
    -0.342020, -0.358368, -0.374607, -0.390731, -0.406737, -0.422618, -0.438371, -0.453990, -0.469472, -0.484810,
    -0.500000, -0.515038, -0.529919, -0.544639, -0.559193, -0.573576, -0.587785, -0.601815, -0.615661, -0.629321,
    -0.642788, -0.656059, -0.669131, -0.681998, -0.694658, -0.707107, -0.719340, -0.731354, -0.743145, -0.754710,
    -0.766044, -0.777146, -0.788011, -0.798636, -0.809017, -0.819152, -0.829038, -0.838671, -0.848048, -0.857167,
    -0.866025, -0.874620, -0.882948, -0.891007, -0.898794, -0.906308, -0.913545, -0.920505, -0.927184, -0.933580,
    -0.939693, -0.945519, -0.951057, -0.956305, -0.961262, -0.965926, -0.970296, -0.974370, -0.978148, -0.981627,
    -0.984808, -0.987688, -0.990268, -0.992546, -0.994522, -0.996195, -0.997564, -0.998630, -0.999391, -0.999848
};

double cosine_lut(double x) {
    // Normalize x to [0, TWO_PI)
    while (x < 0)        x += TWO_PI;
    while (x >= TWO_PI)  x -= TWO_PI;

    int degree = (int)(x * (180.0 / PI));  // Convert radians to degrees
    if (degree >= 360) degree = 0;

    return cos_lut[degree];
}

/*
void putpixel(uint32_t x, uint32_t y, uint32_t color, 
              uint8_t* framebuffer_addr, uint32_t pitch) {
    uint32_t* pixel = (uint32_t*)(framebuffer_addr + y * pitch + x * 4);
    *pixel = color;
}*/

void putpixel(unsigned int x, unsigned int y, unsigned int color,
              unsigned char* framebuffer_addr, unsigned int pitch) {
        
        unsigned int offset = y * pitch + x * 4;
        unsigned int* pixel = (unsigned int*)(framebuffer_addr + offset);
        *pixel = color;        
}



extern unsigned long framebuffer_addr,
                     framebuffer_width,
                     framebuffer_height,
                     framebuffer_depth,
                     framebuffer_pitch;

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

#define WIDTH 1920
#define HEIGHT 1080
#define BYTES_PER_PIXEL 4
#define SCREEN_SIZE (WIDTH * HEIGHT *BYTES_PER_PIXEL )

unsigned char back_buffer[SCREEN_SIZE];  // 1920x1080x4

void blit_to_framebuffer(void* dest, const void* src, unsigned int size) {
    __asm__ __volatile__ (
        "cld\n\t"              // Clear direction flag
        "rep movsq\n\t"        // Copy EAX bytes from source to dest
        :
        : "S"(src), "D"(dest), "c"(size/8 )
        : "memory"
    );
}

void clear_back_buffer() {
    unsigned long long *dst = (unsigned long long *)back_buffer;
    unsigned long long count = SCREEN_SIZE / sizeof(unsigned long long);

    __asm__ __volatile__ (
        "cld\n\t"              // Clear direction flag
        "rep stosq\n\t"        // Fill [rdi] with rax, rcx times
        :
        : "D" (dst), "a" (0), "c" (count)
        : "memory"
    );
}

void kernel_main(){

    /* output to serial port */
    strprint_serial("\n\n**** In 64 bit Kernel! ****\nFramebuffer Address: 0x");
    hexprint_serial(framebuffer_addr);

    strprint_serial("\nPitch: 0x");
    hexprint_serial(framebuffer_pitch);

    strprint_serial("\nWidth: 0x");
    hexprint_serial(framebuffer_width);
    

    strprint_serial("\nHeight: 0x");
    hexprint_serial(framebuffer_height);
    

    strprint_serial("\nDepth: 0x");
    hexprint_serial(framebuffer_depth);

    strprint_serial("\nBackbuffer Address: 0x");
    hexprint_serial(&back_buffer);
    /*for(int x=7; x >= 0; x--){
        char ch = (char)(((framebuffer_addr >> (x*4)) & 0xf));
        ch = (ch > 10) ? ch + 55 : ch + 48;
        write_serial((char)ch);
    }*/

    write_serial((char)'\n');

    putpixel(1920,1080,random24(), (uint8_t*) framebuffer_addr, framebuffer_pitch);
        
    
    printf_gfx_term(0xFFFFFF,"Hi!!");

    /* output to framebuffer */
    for(int z = 0; 1;z++){
        
        for(int f = 1; f < 50; f++){

            printf_gfx_term(random24(),"Hi!! %d\t", f);

            for(float x = 0; x < 600* PI; x += 0.1){
                
                //printk("%f\n", 600*cosine(x) + 960);

                //putpixel(x,((int)x%400) *cosine(x/f )+500,0x00ff *f, (uint8_t*) back_buffer, framebuffer_pitch);
                
            }
            //vsync();
            fake_vsync_delay();
            /*
            char buf[94];
            for(int x=32; x < 127;x++){
                buf[x-32] = x;
            }
            buf[93] = 0;

            print_xy(10, 10, buf,0xFFFFFFFF);
            */

            blit_to_framebuffer((void*) framebuffer_addr, back_buffer, SCREEN_SIZE);
            //clear_back_buffer();
            print_xy(200, 40, "Welcome to AIOS!",random24());

        }

        for(int f = 50; f > 1; f--){
            
            printf_gfx_term(random24(),"Hi!! %d\t", f);

            for(float x = 0; x < 600* PI; x += 0.1){
                 
                //printk("%f\n", 600*cosine(x) + 960);

                //putpixel(x,((int)x%300)*cosine(x/f )+500,0xFF00 * (int)f, (uint8_t*) back_buffer, framebuffer_pitch);
                //fake_vsync_delay();      

            }
            //vsync();
            fake_vsync_delay();
            print_xy(200, 250, "Welcome to AIOS!",random24());
            //clear_back_buffer();

            blit_to_framebuffer((void*) framebuffer_addr, back_buffer, SCREEN_SIZE);
            //clear_back_buffer();
            //if(f % 3 == 0) clear_screen((void *)framebuffer_addr,framebuffer_width, framebuffer_height,framebuffer_pitch,0);
        }
    }
    //for(int z = 0; z < framebuffer_pitch /2; z++)
    for (int x = 0, y = 0; x<100; x++, y++){
        if( y >= 200)
            y = 0;

        //printk("%f\n", 10*cosine(x/10) + 10);

        putpixel(x,10*cosine(x)+10,random24(), (uint8_t*) back_buffer, framebuffer_pitch);
 
        blit_to_framebuffer((void*) framebuffer_addr, back_buffer, SCREEN_SIZE);
        clear_back_buffer();

        //write_serial('B');
        //((unsigned long*)(framebuffer_addr))[x] = random24();
    }

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