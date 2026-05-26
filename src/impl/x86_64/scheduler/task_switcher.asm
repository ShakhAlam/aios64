; switch_task:
; void switch_task(task_t* current, task_t* next);
; Saves callee-saved regs to current task and restores from next task

extern strprint_serial
extern printf_serial

section .text
global switch_task
global switch_to_first_task

section .text

%if 0
switch_context:
    ; Arguments: rdi = current, rsi = next

    push rdi
    mov rdi, str_ebx  ; Load the address of the string into EAX
    call strprint_serial     
    pop rdi

    push rdi
    push rsi
    mov rdi, str_rsp
    mov rsi, rsp 
    add rsi,16 
    call printf_serial
    pop rsi
    pop rdi

    push rdi
    push rsi
    mov rdi, str_star_rsp
    mov rsi, [rsp+16]
    call printf_serial
    pop rsi
    pop rdi


    push rdi
    push rsi
    mov rdi, str_rsp_next
    mov rsi, [rsi + 48]
    call printf_serial
    pop rsi
    pop rdi

    push rdi
    push rsi
    push rax
    mov rdi, str_star_rsp_next
    mov rax, [rsi + 48]
    mov rsi, [rax]
    call printf_serial
    pop rax
    pop rsi
    pop rdi

    ; Save callee-saved registers into `current` task struct
    mov [rdi +  0], rbx
    mov [rdi +  8], rbp
    mov [rdi + 16], r12
    mov [rdi + 24], r13
    mov [rdi + 32], r14
    mov [rdi + 40], r15
    mov [rdi + 48], rsp

    ; Load registers from `next` task struct
    mov rbx, [rsi +  0]
    mov rbp, [rsi +  8]
    mov r12, [rsi + 16]
    mov r13, [rsi + 24]
    mov r14, [rsi + 32]
    mov r15, [rsi + 40]
    mov rsp, [rsi + 48]

    ;sti
    ret
%endif


; void switch_task(struct task* current, struct task* next)
switch_task:


    push rdi
    mov rdi, str_ebx  ; Load the address of the string into EAX
    call strprint_serial     
    pop rdi

    push rdi
    push rsi
    mov rdi, str_rsp
    mov rsi, rsp 
    add rsi,16 
    call printf_serial
    pop rsi
    pop rdi

    push rdi
    push rsi
    mov rdi, str_star_rsp
    mov rsi, [rsp]
    call printf_serial
    pop rsi
    pop rdi


    push rdi
    push rsi
    mov rdi, str_rsp_next
    mov rsi, [rsi + 8]
    call printf_serial
    pop rsi
    pop rdi

    push rdi
    push rsi
    push rax
    mov rdi, str_star_rsp_next
    mov rax, [rsi + 8]
    mov rsi, [rax]
    call printf_serial
    pop rax
    pop rsi
    pop rdi

    ; Save callee-saved registers
    mov [rdi + 0x20], rbx
    mov [rdi + 0x28], rbp
    mov [rdi + 0x30], r12
    mov [rdi + 0x38], r13
    mov [rdi + 0x40], r14
    ;mov [rdi + 0x08], rsp   ; this is done in the timer handler now 
    ; used to be: Save current task's rsp

    ; Load next task's registers
    mov rbx, [rsi + 0x20]
    mov rbp, [rsi + 0x28]
    mov r12, [rsi + 0x30]
    mov r13, [rsi + 0x38]
    mov r14, [rsi + 0x40]
    mov rsp, [rsi + 0x08]

    iretq

; void switch_to_first_task(uint64_t rsp)
switch_to_first_task:

    push rdi
    push rsi
    mov rsi, [rdi + 8]
    mov rdi, str_rsp_next
    
    call printf_serial
    pop rsi
    pop rdi

    push rdi
    push rsi
    push rax
    mov rax, [rdi + 8]
    mov rdi, str_star_rsp_next
    mov rsi, [rax]
    call printf_serial
    pop rax
    pop rsi
    pop rdi


    push rdi
    push rsi
    push rax
    mov rax, [rdi + 8]
    mov rdi, str_task_cs
    mov rsi, [rax+8]
    call printf_serial
    pop rax
    pop rsi
    pop rdi


    push rdi
    push rsi
    push rax
    mov rax, [rdi + 8]
    mov rdi, str_task_rflags
    mov rsi, [rax+16]
    call printf_serial
    pop rax
    pop rsi
    pop rdi

    mov rsp, [rdi + 0x08]     ; Load saved rsp (points to iretq frame)

    mov rbx, [rdi + 0x10]     ; Restore RBX
    mov rbp, [rdi + 0x18]     ; Restore RBP
    mov r12, [rdi + 0x20]     ; Restore R12
    mov r13, [rdi + 0x28]     ; Restore R13
    mov r14, [rdi + 0x30]     ; Restore R14

    iretq


section .data
    str_ebx db 0xa,"in task switcher ",0xa, 0


    str_rsp db "Current RSP: %x. This will be saved in the current task->rsp.", 0xa, 0
    str_star_rsp db "Current *RSP: %x. This is the RIP for the current task.", 0xa, 0

    str_rsp_next db "Next Task RSP: 0x%x. Will load this to rsp", 0xa, 0
    str_star_rsp_next db "Next Task *RSP: 0x%x. This is the next RIP after ret.", 0xa, 0

    str_task_cs db "Task CS: 0x%x.", 0xa, 0

    str_task_rflags db "Task RFLAGS: 0x%x.", 0xa, 0