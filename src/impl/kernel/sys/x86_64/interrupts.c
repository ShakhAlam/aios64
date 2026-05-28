#include "kernel/sys/x86_64/idt.h"

#include "kernel/sys/x86_64/ports.h" 
#include "print.h"
#include "kernel/sys/x86_64/scheduler.h"
#include "kernel/io/serial.h"

extern void isr_timer();

void irq_handler_common() {
    // Acknowledge IRQ to PIC
    outb(0x20, 0x20); // Master PIC
}

extern struct task *current_task;

//__attribute__((interrupt))
void timer_handler(void* frame) {
    static int tick =0;
    tick++;
  
    //strprint_serial("\nin timer_handler\n");
    log_serial(debug,"<timer_handler> invoked: %d\n",tick);
    
}
