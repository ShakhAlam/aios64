#pragma once

#include <stdint.h>  // For uint types

// IDT entry (64-bit format)
struct idt_entry {
    uint16_t offset_low;     // Bits 0–15 of ISR address
    uint16_t selector;       // Code segment selector
    uint8_t ist;             // Bits 0–2: IST offset, rest zero
    uint8_t type_attr;       // Type and attributes
    uint16_t offset_mid;     // Bits 16–31 of ISR address
    uint32_t offset_high;    // Bits 32–63 of ISR address
    uint32_t zero;           // Reserved, must be zero
} __attribute__((packed));

// IDT pointer format used by `lidt` instruction
struct idt_ptr {
    uint16_t limit;  // Size of IDT - 1
    uint64_t base;   // Address of the first `idt_entry`
} __attribute__((packed));

// Number of IDT entries (standard is 256 for x86_64)
#define IDT_ENTRIES 256

// Function to initialize the IDT
void idt_init(void);

// Function to set an individual entry
void set_idt_entry(int vector, void (*isr)(), uint8_t flags);

// Expose the IDT pointer and entries
extern struct idt_entry idt[IDT_ENTRIES];

void idt_load();

