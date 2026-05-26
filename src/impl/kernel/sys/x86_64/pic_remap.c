#include "kernel/sys/x86_64/ports.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define ICW1_INIT    0x11
#define ICW4_8086    0x01

void io_wait(void);  // Typically implemented as a short delay using out to 0x80
 
void pic_remap() {
    // Start initialization sequence for both master and slave PICs
    outb(PIC1_COMMAND, ICW1_INIT); // Start init sequence (ICW1) for master
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT); // Start init sequence (ICW1) for slave
    io_wait();

    // Set vector offset — remap PIC interrupts to avoid conflicts with CPU exceptions
    outb(PIC1_DATA, 0x20); // Master PIC vector offset to 0x20 (32)
    io_wait();
    outb(PIC2_DATA, 0x28); // Slave PIC vector offset to 0x28 (40)
    io_wait();

    // Tell Master PIC that Slave PIC is at IRQ2 (binary 0000 0100)
    outb(PIC1_DATA, 4);
    io_wait();
    
    // Tell Slave PIC its cascade identity (IRQ2)
    outb(PIC2_DATA, 2);
    io_wait();

    // Set both PICs to 8086/88 mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // Mask all IRQs (you can selectively unmask later)
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    //    asm volatile ("sti");

    //outb(PIC1_DATA, 0xFC); // 0b11111100 = enable IRQ0 and IRQ1 only
    //outb(PIC2_DATA, 0xFC); // 0b11111100 = enable IRQ0 and IRQ1 only

}

void pic_init_timer1(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;

    outb(0x43, 0x36);               // Command port
    outb(0x40, divisor & 0xFF);     // Low byte
    outb(0x40, (divisor >> 8) & 0xFF); // High byte
}

void pic_init_timer(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    if (divisor > 65535) divisor = 65535;

    outb(0x43, 0x36);               // PIT control port: channel 0, low/high, mode 3
    outb(0x40, divisor & 0xFF);     // Send low byte
    outb(0x40, (divisor >> 8) & 0xFF); // Send high byte
}

void pic_unmask_irqs(){

    outb(PIC1_DATA, 0xFC); // 0b11111100 = enable IRQ0 and IRQ1 only
    outb(PIC2_DATA, 0xFC); // 0b11111100 = enable IRQ0 and IRQ1 only

}

void pic_unmask_irq(unsigned char irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);  // Clear the bit to unmask
    outb(port, value);
}

#define PIC1_COMMAND 0x20
#define PIC2_COMMAND 0xA0
#define PIC_EOI      0x20

void pic_send_eoi(unsigned char irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);  // Send EOI to slave PIC
    }
    outb(PIC1_COMMAND, PIC_EOI);      // Always send EOI to master PIC
}
