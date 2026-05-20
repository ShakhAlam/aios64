#include "kernel/sys/x86_64/idt.h"


// Reserve space for 256 IDT entries
struct idt_entry idt[IDT_ENTRIES];

// Define the IDT descriptor
static struct idt_ptr idtp;

// Set an IDT entry
void set_idt_entry(int vector, void (*isr)(), uint8_t flags) {
    uint64_t isr_addr = (uint64_t)isr;

    idt[vector].offset_low  = isr_addr & 0xFFFF;
    idt[vector].selector    = 0x08;  // Kernel code segment
    idt[vector].ist         = 0;     // No special IST
    idt[vector].type_attr   = flags; // Present bit, DPL, gate type (e.g. 0x8E = int gate)
    idt[vector].offset_mid  = (isr_addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (isr_addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;     // Must be zero
}

// Load the IDT using `lidt` assembly instruction
extern void load_idt(void*);

// Initialize the IDT
void idt_init(void) {
    // Set up the IDT descriptor
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base  = (uint64_t)&idt;

    // Zero all entries
    for (int i = 0; i < IDT_ENTRIES; i++) {
        set_idt_entry(i, 0, 0);
    }

 
}

void idt_load(){
   // Load the IDT
   load_idt(&idtp);
}
