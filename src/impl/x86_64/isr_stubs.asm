global isr_keyboard
global isr_timer
global load_idt

global isr_page_fault

extern keyboard_handler, timer_handler, page_fault_handler

section .text

isr_page_fault:
    cli
    ;mov al, '!'
    ;mov dx, 0x3F8              ; Data Register (THR)
    ;out dx, al


    call page_fault_handler
    sti
    iretq

isr_keyboard:
    
    cli
    ;mov al, '@'
    ;mov dx, 0x3F8              ; Data Register (THR)
    ;out dx, al
    pushaq
    pushfq

    push 0
    call keyboard_handler
    add rsp, 8
    
    popfq
    popaq

    sti
    iretq

isr_timer:
    cli
    push 0
    call timer_handler
    add rsp, 8
    
    ;mov al,0x20
    ;mov dx,0x20
    ;out dx, al
    
    
    sti
    iretq

load_idt:
    mov rax, rdi
    lidt [rax]
    ret
