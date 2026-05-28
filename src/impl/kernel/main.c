#include "print.h"
#include "ascii_font.h"
#include "kernel/io/keyboard.h"
#include "kernel/strings/strings.h"
#include "kernel/util/shell.h"
#include "kernel/util/math.h"
#include "kernel/sys/x86_64/interrupt.h"
#include "kernel/sys/x86_64/idt.h"
#include <stddef.h>

#include "kernel/sys/x86_64/scheduler.h"
#include "kernel/io/serial.h"


extern unsigned long framebuffer_addr,
                     framebuffer_width,
                     framebuffer_height,
                     framebuffer_depth,
                     framebuffer_pitch;



#define RED 0xff0000
#define GREEN 0xff00
#define BLUE 0xff
#define YELLOW 0xffff00


#define WIDTH 1920
#define HEIGHT 1080
#define BYTES_PER_PIXEL 4
#define SCREEN_SIZE (WIDTH * HEIGHT *BYTES_PER_PIXEL )

unsigned int back_buffer[WIDTH * HEIGHT];  // 1920x1080x4

void blit_to_framebuffer(void* dest, const void* src, unsigned int size) {
    for(int x = 0; x < size / 8; x ++){
        ((unsigned long long*)dest)[x] = ((unsigned long long*)src)[x];
    }
    /*
    __asm__ __volatile__ (
        "cld\n\t"              // Clear direction flag
        "rep movsq\n\t"        // Copy EAX bytes from source to dest
        :
        : "S"(src), "D"(dest), "c"(size/8 )
        : "memory"
    );*/
}

void clear_back_buffer() {
    unsigned long long *dst = (unsigned long long *)back_buffer;
    unsigned long long count = (SCREEN_SIZE*4) / sizeof(unsigned long long);

    __asm__ __volatile__ (
        "cld\n\t"              // Clear direction flag
        "rep stosq\n\t"        // Fill [rdi] with rax, rcx times
        :
        : "D" (dst), "a" (0), "c" (count)
        : "memory"
    );
}


void page_fault_handler(void* frame, unsigned long error_code) {
    // Read fault address from CR2
    uint64_t fault_address;
    __asm__ volatile ("mov %%cr2, %0" : "=r" (fault_address));

    struct task * t = get_current_task();
    
    log_serial(critical,"Page Fault Detected."
            "Current Task ID: %d, "
            "Fault Address (CR2): %lx, "
            "Error Code: %lx\n",
            t->pid,
            fault_address,
            error_code
    );

    // Basic debug output
    printf_gfx_term(0xFFFF00, 0, "\nPage Fault!");
    printf_gfx_term(0xFFFF00, 0, "\nCR2 = 0x%x", fault_address);
    printf_gfx_term(0xFFFF00, 0, "\nError Code = 0x%x", error_code);

    //TODO: tech-debt in the following code
    //The switch to an appropriate next task needs to be
    //synchronized. Need kernel synchronization features.
    //Currenly sync is not in MVP
    current_task = current_task->next;
    first_task_start();

    //while(1);
    // Halt system
    while (1) {
        __asm__ volatile ("hlt");
    }
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
    
    write_serial('\n');

    pic_remap();

    idt_init();
    
    set_idt_entry(32, isr_timer, 0x8E);     // IRQ0 (timer)
    set_idt_entry(33, isr_keyboard, 0x8E);  // IRQ1 (keyboard)
    set_idt_entry(14, isr_page_fault, 0x8E); // page fault 

    set_idt_entry(13, (uint64_t) isr_gp_fault, 0x8E); // vector 13 = GPF

    
    pic_init_timer(1);   // 100Hz
    
    pic_unmask_irq(0); // timer

    pic_unmask_irq(1); // keyboard

    idt_load();

    

    init_tasking();
    asm volatile ("sti");  // 💥 Enable interrupts here


    //volatile int* ptr = (int*)0xDEEDBEEF;
    //int val = *ptr; // Access invalid memory to test page fault handler


    //putpixel(1920,1080,random24(), (uint8_t*) framebuffer_addr, framebuffer_pitch);
       
}