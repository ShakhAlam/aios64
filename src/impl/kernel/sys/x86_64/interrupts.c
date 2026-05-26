#include "kernel/sys/x86_64/idt.h"

#include "kernel/sys/x86_64/ports.h" 
#include "print.h"
#include "kernel/sys/x86_64/scheduler.h"

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
    current_task->rsp = (uint64_t)frame;

    strprint_serial("\nin timer_handler\n");
    printf_serial("<timer_handler> Saved RSP from interrupt frame! current task id: %d rsp: %x, *rsp=%x\n", 
        current_task->pid, current_task->rsp, *((uint64_t*)(current_task->rsp)) );
   
    //printf_gfx_term(0xFFFFFF,0,"%d\t", tick);
    
    //if(tick >= 18){
        strprint_serial("\nin timer_handler - calling schedule\n");
        //tick = 0;
        schedule();
        strprint_serial("\nin timer_handler - after schedule()\n");
    //}
    
    
    pic_send_eoi(0);  // IRQ0 is the timer

}
