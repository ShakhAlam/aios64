#include <stdint.h>
#include "print.h"
#include "kernel/io/keyboard.h"

char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
int kb_buf_head = 0;
int kb_buf_tail = 0;

extern void io_wait(void);

#define SCANCODE_BUFFER_SIZE 128
extern volatile unsigned char scancode_buffer[];
extern volatile int scancode_head;
extern volatile int scancode_tail;

int read_scancode_blocking(void) {
    while (scancode_head == scancode_tail) {
        __asm__ volatile ("hlt");
    }

    unsigned char scancode = scancode_buffer[scancode_tail];
    scancode_tail = (scancode_tail + 1) % SCANCODE_BUFFER_SIZE;
    return scancode;
}

int read_scancode_nonblock(unsigned char* out_scancode) {
    if (scancode_head == scancode_tail) {
        return 0; // no scancode available
    }

    *out_scancode = scancode_buffer[scancode_tail];
    scancode_tail = (scancode_tail + 1) % SCANCODE_BUFFER_SIZE;
    return 1;
}

static const char scancode_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', // 0x00 - 0x07
   '7', '8', '9', '0', '-', '=', '\b', '\t', // 0x08 - 0x0F
   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', // 0x10 - 0x17
   'o', 'p', '[', ']', '\n', 0,  'a', 's', // 0x18 - 0x1F
   'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', // 0x20 - 0x27
   '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', // 0x28 - 0x2F
   'b', 'n', 'm', ',', '.', '/', 0,  '*', // 0x30 - 0x37
    0,  ' ', 0,    0,    0,    0,    0,    0,  // 0x38 - 0x3F
    0,  0,   0,    0,    0,    0,    0,    0   // 0x40 - 0x47
    // Add more if you want to support extended scancodes
};


int shift_pressed = 0;
int capslock_active = 0;

char scancode_to_ascii(unsigned char scancode) {
    unsigned char ascii = 0;
    
    /*if (scancode >= sizeof(scancode_ascii)) {
        return 0; // Unknown/unsupported key
    }*/

    int key_released = scancode & 0x80;
    scancode &= 0x7F; // Strip release bit

    // Modifier keys
    if (scancode == 0x2A || scancode == 0x36) { // Left/Right Shift
        shift_pressed = !key_released;
        return 0;
    }

    if (scancode == 0x3A && !key_released) { // Caps Lock press
        capslock_active = !capslock_active;
        return 0;
    }

    if (key_released)
        return 0;

    static const unsigned char normal_map[128] = {
        0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b','\t',
        'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0, 'a','s',
        'd','f','g','h','j','k','l',';','\'','`', 0,'\\','z','x','c','v',
        'b','n','m',',','.','/', 0, '*', 0, ' ', 0
    };

    static const unsigned char shift_map[128] = {
        0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b','\t',
        'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0, 'A','S',
        'D','F','G','H','J','K','L',':','"','~', 0,'|','Z','X','C','V',
        'B','N','M','<','>','?', 0, '*', 0, ' ', 0
    };

    if (shift_pressed || (capslock_active && scancode >= 0x10 && scancode <= 0x32)) {
        ascii = shift_map[scancode];
    } else {
        ascii = normal_map[scancode];
    }

    return ascii;
    //return scancode_ascii[scancode];
}




unsigned char getch() {
    unsigned char scancode = 0;
    unsigned char ascii = 0;


    // Wait for scancode
    __asm__ __volatile__ (
        "1:\n\t"
        "inb $0x64, %%al\n\t"
        "test $1, %%al\n\t"
        "jz 1b\n\t"
        :
        :
        : "al"
    );

    // Read scancode from port 0x60
    __asm__ __volatile__ (
        "inb $0x60, %0\n\t"
        : "=a"(scancode)
    );

    __asm__ __volatile__("pause");


    int key_released = scancode & 0x80;
    scancode &= 0x7F; // Strip release bit

    // Modifier keys
    if (scancode == 0x2A || scancode == 0x36) { // Left/Right Shift
        shift_pressed = !key_released;
        return 0;
    }

    if (scancode == 0x3A && !key_released) { // Caps Lock press
        capslock_active = !capslock_active;
        return 0;
    }

    if (key_released)
        return 0;

    static const unsigned char normal_map[128] = {
        0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b','\t',
        'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0, 'a','s',
        'd','f','g','h','j','k','l',';','\'','`', 0,'\\','z','x','c','v',
        'b','n','m',',','.','/', 0, '*', 0, ' ', 0
    };

    static const unsigned char shift_map[128] = {
        0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b','\t',
        'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0, 'A','S',
        'D','F','G','H','J','K','L',':','"','~', 0,'|','Z','X','C','V',
        'B','N','M','<','>','?', 0, '*', 0, ' ', 0
    };

    if (shift_pressed || (capslock_active && scancode >= 0x10 && scancode <= 0x32)) {
        ascii = shift_map[scancode];
    } else {
        ascii = normal_map[scancode];
    }

    return ascii;
}

char getchar();

/*
char getchar() {
    while (kb_buf_head == kb_buf_tail) {
        // Wait for a character to be buffered by the ISR
        asm volatile ("hlt");
    }

    char c = keyboard_buffer[kb_buf_tail];
    kb_buf_tail = (kb_buf_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

char getchar_nonblock() {
    if (kb_buf_head == kb_buf_tail) {
        return 0; // No character available
    }

    char c = keyboard_buffer[kb_buf_tail];
    kb_buf_tail = (kb_buf_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

*/

char* k_fgets(char* buffer, int max_len) {
    int i = 0;

    while (i < max_len - 1) {
        unsigned char c = getchar();
        if(c == 0){ // skip meta chars
       //     printf_gfx_term(0xFFFFFFFF,0," <ctrl> ");
            c = getchar();
            if(c == 0){ // skip meta chars
         //       printf_gfx_term(0xFFFFFFFF,0," <ctrl> ");
                c = getchar();
            }    
        }

        printf_gfx_term(0xFFFFFFFF,0,"%c",c);

        //printf_gfx_term(0xFFFFFFFF,0," %x ",c);

        // Optional: Echo the typed character (if you're using a serial or screen output)
        // putchar(c);
        //printf_gfx_term(0xFFFFFFFF,0,"%c", c);

        if (c == '\r' || c == '\n') {
            printf_gfx_term(0xFFFFFFFF,0,"\n");
            //buffer[i++] = c;
            break;
        } else if (c == 0x08 || c == 0x7F) { // Backspace
            if (i > 0) {
                i--;
                // Optional: Handle backspace echoing if needed
                // putchar('\b'); putchar(' '); putchar('\b');
                //printf_gfx_term(0xFFFFFFFF,0,"%c", c);
        
            }
        } else {
            buffer[i++] = c;
        }
    }
    //printf_gfx_term(0xFFFFFFFF,0,"%d out of %d", i,max_len);
    //printf_gfx_term(0xFFFFFFFF,0,"%s", " <BR> " );
            
    buffer[i] = 0;

    //printf_gfx_term(0xFFFFFFFF,0," <cmp> ");
    return buffer;
}

