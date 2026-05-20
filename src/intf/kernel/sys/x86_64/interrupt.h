#pragma once  // interrupts.h

// Declarations for interrupt handlers
__attribute__((interrupt)) void keyboard_handler(void* frame);
__attribute__((interrupt)) void timer_handler(void* frame);

extern void isr_keyboard();
extern void isr_timer();
extern void isr_page_fault();
