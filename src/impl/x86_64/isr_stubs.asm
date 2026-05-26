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
    ; Save registers (not using __attribute__((interrupt)))
    cli
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rsp

    lea rdi, [rsp + 128]       ; skip over pushed regs 
    ;mov rdi, rsp              ; Pass pointer to interrupt frame
    call timer_handler        ; Call C handler

    ; Restore registers
    pop rsp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    sti

    iretq

load_idt:
    mov rax, rdi
    lidt [rax]
    ret
