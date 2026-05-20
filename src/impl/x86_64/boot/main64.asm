global long_mode_start

extern kernel_main
extern framebuffer_addr
extern print_string_serial


section .text
bits 64
long_mode_start:
    mov ax, 0       ;nullify data segment registers
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov al, 'A'
    mov dx, 0x3F8              ; Data Register (THR)
    out dx, al

    call kernel_main
    hlt
section .data
    longmode_message2 db 0xA,"Entered 64bit Mode!",0xA