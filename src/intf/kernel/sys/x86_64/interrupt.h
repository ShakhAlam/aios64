#pragma once  // interrupts.h

// Declarations for interrupt handlers
void keyboard_handler(void* frame);
void timer_handler(void* frame);
void gp_fault_handler(uint64_t* stack) ;

extern void isr_keyboard();
extern void isr_timer();
extern void isr_page_fault();
extern void isr_gp_fault();

static inline void sti(void) {
    __asm__ __volatile__ ("sti");
}

static inline void cli(void) {
    __asm__ __volatile__ ("cli");
}
