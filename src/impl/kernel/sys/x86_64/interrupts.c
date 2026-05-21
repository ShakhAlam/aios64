#include "kernel/sys/x86_64/idt.h"

#include "kernel/sys/x86_64/ports.h" 
#include "print.h"

extern void isr_timer();

void irq_handler_common() {
    // Acknowledge IRQ to PIC
    outb(0x20, 0x20); // Master PIC
}

//__attribute__((interrupt))
void timer_handler(void* frame) {
    static int tick =0;

    tick++;
    printf_gfx_term(0xFFFFFF,0,"%d\t", tick);
    
    write_serial('!');

    pic_send_eoi(0);  // IRQ0 is the timer
}
