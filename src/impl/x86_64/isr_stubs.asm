global isr_keyboard
global isr_timer
global load_idt

global isr_page_fault
global isr_gp_fault
global first_task_start

extern current_task, next_task

extern keyboard_handler, timer_handler, gp_fault_handler, page_fault_handler
extern strprint_serial
extern printf_serial
extern log_serial
extern get_current_task, get_next_task

section .text

isr_page_fault:
    cli
    ;mov al, '!'
    ;mov dx, 0x3F8              ; Data Register (THR)
    ;out dx, al


    call page_fault_handler
    sti
    iretq

isr_gp_fault:
    cli
    ;push rax
    ;push rcx
    ;push rdx
    mov rdi, rsp
    call gp_fault_handler
    ;pop rdx
    ;pop rcx
    ;pop rax
    sti
    iretq

isr_keyboard:
    
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
    pushf



    push 0
    call keyboard_handler
    add rsp, 8


    popf
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

isr_timer:
    ; Save registers (not using __attribute__((interrupt)))
    cli
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    ;push rsp
    pushf

    

    push rdi
    mov rdi, rsp       ; skip over pushed registers 
    call timer_handler        ; Call C handler
    pop rdi


    mov rdx, [current_task]
    mov rdx, [rdx+72]
    mov rdi, 4 ; loglevel 
    mov rsi, str_dbg_quantum
    call log_serial

    mov rdx, [current_task]
    mov rdx, [rdx+72]

    cmp rdx,4 ; check quantum
    jle out1 
    
    ; TODO: Save callee-saved registers in struct task
    ; instead of the interrupt stack 
    mov rdi, [current_task]
    
    ;mov [rdi + 0x20], rbx
    ;mov [rdi + 0x28], rbp
    ;mov [rdi + 0x30], r12
    ;mov [rdi + 0x38], r13
    ;mov [rdi + 0x40], r14
    mov rsi, rsp
    mov [rdi + 0x08], rsi    ; Save current task's rsp in struct task->rsp

%if 0
        
    ; Load next task's registers
    ;mov rbx, [rsi + 0x20]
    ;mov rbp, [rsi + 0x28]
    ;mov r12, [rsi + 0x30]
    ;mov r13, [rsi + 0x38]
    ;mov r14, [rsi + 0x40]
    ;mov rsp, [rsi + 8]

%endif
first_task_start:
    cli
    call get_next_task
    mov rsp, [rax+8]
%if 1  ; print thread stack
    mov rcx, 19
.dump_loop1:
    mov rdi, 4 ; loglevel = debug
    mov rsi, str_dbg_stack_entry
    mov rdx, [rsp+rcx*8]
    push rcx
    call log_serial
    pop rcx
    dec rcx
    cmp rcx,0
    jge .dump_loop1

    ;enable the following for debugging RIP
    ;push rdi
    ;push rsi
    ;lea rsi, [rel rip_label]
    ;mov rdi, str_current_rip
    ;call printf_serial
    ;pop rsi
    ;pop rdi
%endif   
out1:

    ;send ack EOI to PIC
    mov al,0x20
    out 0x20, al
    
    popf    ;restore rflags

    ; Restore registers

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    
    sti
rip_label:

    iretq


load_idt:
    mov rax, rdi
    lidt [rax]
    ret


section .bss
align 4096
    counter: resq 1    

section .data
    str_info db 0xa,"in isr timer task switcher ",0xa, 0


    str_rsp db "Current RSP: %x. This will be saved in the current task->rsp.", 0xa, 0
    str_star_rsp db "Current *RSP: %x. This is the RIP for the current task.", 0xa, 0

    str_rsp_next db "Next Task RSP: 0x%x. Will load this to rsp", 0xa, 0
    str_star_rsp_next db "Next Task *RSP: 0x%x. This is the next RIP after retq.", 0xa, 0

    str_task_cs db "Task CS: 0x%lx.", 0xa, 0

    str_task_rflags db "Task RFLAGS: 0x%lx.", 0xa, 0
    str_task_rip db "Task RIP: 0x%lx.", 0xa, 0
    str_current_rip db "Current RIP: 0x%lx.", 0xa, 0
    str_dbg_stack_entry db "%lx at rsp + %d * 8",0xa,0
    str_dbg_quantum db "current_task quantum = %d",0xa,0
