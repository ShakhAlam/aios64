global start

extern long_mode_start

section .text
bits 32
start:
    mov esp, stack_top

    call check_multiboot
    call check_long_mode
    
    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start

    ;mov dword [0xb8000], 0x2f4b2f4f
    hlt

check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, "M"
    jmp error

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
.no_cpuid:
    mov al, "C"
    jmp error

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode

    ret

.no_long_mode:
    mov al, "L"
    jmp error

setup_page_tables:
    mov eax, page_table_l3  ; load ptr to pt l3
    or eax, 0b11            ; set flags:present, writable
    mov [page_table_l4], eax

    mov eax, page_table_l2  ; ptr to pt l2
    or eax, 0b11            ; present, writable
    mov [page_table_l3], eax

    mov ecx, 0              ; counter
.loop:
    mov eax, 0x200000       ; 2MiB
    mul ecx
    or eax, 0b10000011       ; present, writable, huge page
    mov [page_table_l2 + ecx * 8], eax

    inc ecx                 ; counter++
    cmp ecx, 512            ; if( counter != 512 )
    jne .loop               ;   goto loop

    ret

enable_paging:
    mov eax, page_table_l4  ; page table location
    mov cr3, eax            ; send to cpu via cr3

    mov eax, cr4            ; enable PAE
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080     ; enable long-mode
    rdmsr                   ; read model specific register
    or eax, 1 << 8          ; set long mode bit       
    wrmsr                   ; write model spec reg

    mov eax, cr0            ; enable paging  
    or eax, 1 << 31         ; set paging bit
    mov cr0, eax            ; update cr0

    ret


error:
	; display "ERR: X" where X is the error code
	mov dword [0xb8000], 0x4f524f45
	mov dword [0xb8004], 0x4f3a4f52
	mov dword [0xb8008], 0x4f204f20
	mov byte  [0xb800a], al
	hlt



section .bss
align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096
stack_bottom:
    resb 4096 * 4
stack_top:


section .rodata
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq ( 1 << 43 ) |  (1 << 44) | ( 1 << 47) | (1<<53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64