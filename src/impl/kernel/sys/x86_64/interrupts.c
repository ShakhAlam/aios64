#include "kernel/sys/x86_64/idt.h"

#include "kernel/sys/x86_64/ports.h" 
#include "print.h"
#include "kernel/io/keyboard.h"


extern void isr_keyboard();
extern void isr_timer();

void irq_handler_common() {
    // Acknowledge IRQ to PIC
    outb(0x20, 0x20); // Master PIC
}

//__attribute__((interrupt))
/*void keyboard_handler(void* frame) {
    //

    uint8_t scancode = inb(0x60);
    write_serial(scancode);
    
    printf_gfx_term(0xFFFFFF,0,"[0x%x]", scancode);
    //write_serial('!');

    // handle keyboard input here
    //irq_handler_common();
    pic_send_eoi(1);
}*/

/*void keyboard_handler(void* frame) {
    unsigned char scancode = inb(0x60);

    // Example US QWERTY mapping (just ASCII for A-Z here)
    static char scancode_ascii[] = {
        0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
        'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
        'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ', 0,
        // rest omitted
    };

    char c = scancode_ascii[scancode & 0x7F];
    if (scancode < sizeof(scancode_ascii) && c) {
        int next_head = (kb_buf_head + 1) % KEYBOARD_BUFFER_SIZE;
        if (next_head != kb_buf_tail) { // buffer not full
            keyboard_buffer[kb_buf_head] = c;
            kb_buf_head = next_head;
        }
    }

    pic_send_eoi(1); // IRQ1 - keyboard
}
*/

//__attribute__((interrupt))
void timer_handler(void* frame) {
    static int tick =0;

    tick++;
    printf_gfx_term(0xFFFFFF,0,"%d\t", tick);
    
    //printf_gfx_term(0xFF,0,"!");
    
    write_serial('!');

    pic_send_eoi(0);  // IRQ0 is the timer
    //irq_handler_common();
}
