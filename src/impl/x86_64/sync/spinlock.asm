; spinlock_optimized.asm
global enter_critical_section
global leave_critical_section

section .text

; void enter_critical_section(uint64_t *lock);
enter_critical_section:
    ; rdi = pointer to lock
.try_lock:
    mov rax, [rdi]         ; Load the value of lock
    test rax, rax          ; Check if it's 0 (unlocked)
    jnz .wait              ; If not zero, wait

    ; Try to acquire it
    mov rax, 0             ; Expected: 0 (unlocked)
    mov rcx, 1             ; Set: 1 (locked)
    lock cmpxchg [rdi], rcx
    jz .got_lock           ; Success if ZF=1

.wait:
    pause
    jmp .try_lock

.got_lock:
    ret

; void leave_critical_section(uint64_t *lock);
leave_critical_section:
    ; rdi = pointer to lock
    mov qword [rdi], 0
    ret
