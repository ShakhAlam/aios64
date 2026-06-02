// keyboard_handler.c
#include "kernel/io/keyboard.h"
#include "kernel/sys/x86_64/ports.h"
#include "kernel/sys/x86_64/pic.h"

#define SCANCODE_BUFFER_SIZE 128
volatile unsigned char scancode_buffer[SCANCODE_BUFFER_SIZE];
volatile int scancode_head = 0;
volatile int scancode_tail = 0;

void keyboard_handler(void* frame) {
    unsigned char scancode = inb(0x60);
    //write_serial(scancode);
    //return; 
    
    int next = (scancode_head + 1) % SCANCODE_BUFFER_SIZE;
    if (next != scancode_tail) {
        scancode_buffer[scancode_head] = scancode;
        scancode_head = next;
    }

    pic_send_eoi(1); // IRQ1 - keyboard
}
