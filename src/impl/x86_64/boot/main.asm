global start, longmode_message

extern long_mode_start

extern framebuffer_addr,framebuffer_addr2,framebuffer_width,framebuffer_height
extern framebuffer_depth

extern serial_write, serial_write_char
extern serial_init
extern print_char_serial, print_string_serial, print_hex_serial


section .text
bits 32
start:
    mov esp, stack_top
    push dword 0
    popf    ; clear flags 
    
    call serial_init

    call check_multiboot
    
    call check_long_mode
    
    call setup_page_tables
    call enable_paging


    ; Print " Entering 64bit mode"
    mov esi, longmode_message          
    push esi
    call print_string_serial
    add esp, 4

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start

    ;mov dword [0xb8000], 0x2f4b2f4f
    hlt

check_multiboot:
    
    push eax         
    call print_hex_serial       ; Print tag type as hex
    add esp, 4 

    cmp eax, 0x36d76289
    jne .no_multiboot

    mov al,0x0a
    call print_char_serial      ; Print a new line
    

    lea eax, [str_ebx]  ; Load the address of the string into EAX
    push eax              ; Pass the string address as an argument
    call print_string_serial     
    add esp, 4 

    push ebx           ; Push the value onto the stack
    call print_hex_serial       ; Print tag type as hex
    add esp, 4 
    
    mov al,0x0a
    call print_char_serial      ; Print a new line
    
    mov ecx, 474
.l1

    lea eax, [str_star_ebx]  ; Load the address of the string into EAX
    push eax              ; Pass the string address as an argument
    call print_string_serial     
    add esp,4

    mov eax, ecx   ; Load the value from memory into EAX
    push eax           ; Push the value onto the stack
    call print_hex_serial       ; Print tag type as hex
    add esp, 4 
    
    mov al, ' '
    call print_char_serial


    mov eax, [ebx+ecx*4]   ; Load the value from memory into EAX
    push eax           ; Push the value onto the stack
    call print_hex_serial       ; Print tag type as hex
    add esp, 4 
    
    mov al,0x0a
    call print_char_serial      ; Print a new line

    sub ecx, 1
    jns .l1

    ; EBX holds the multiboot info structure address
    mov edi, ebx       ; Get the multiboot info address from the stack
    add edi, 8           ; Skip total size and reserved field

.tag_scan:
    ; Load the tag->type (assuming 2-byte value at offset 0)
    movzx ecx, word [edi]         ; Load tag->type into ECX
    cmp ecx, 0 ; [multiboot_end_type] ; Compare tag->type with MULTIBOOT_TAG_TYPE_END (0)
    je .loop_done                 ; If it's the end tag, break out of the loop

    ; Print "Tag 0x"
    mov esi, format_tag           ; Load address of "Tag 0x" string
    push esi
    call print_string_serial
    add esp, 4


    ; Print tag->type (as hex)
    movzx eax, word [edi]         ; Load tag->type into EAX
    push eax
    call print_hex_serial                ; Print the hex value of tag->type
    add esp, 4

    cmp eax, 8           ; Check if framebuffer tag (type 8)
    je .read_fb_info

    movzx eax, word [edi]         ; Load tag->type into EAX
    cmp eax, 8           ; Check if framebuffer tag (type 8)
    je .loop_done

    ; Print " Size 0x"
    mov esi, format_size          ; Load address of " Size 0x" string
    push esi
    call print_string_serial
    add esp, 4


    ; Print tag->size (as hex)
    movzx eax, word [edi + 4]     ; Load tag->size into EAX (assuming size is 2 bytes at offset 2)
    push eax
    call print_hex_serial                ; Print the hex value of tag->size
    add esp, 4
    
    ; Print newline (to move to the next line)
    mov esi, newline              ; Load address of newline character
    push esi
    call print_string_serial
    add esp, 4

    ; Move to the next tag
    ; tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7))
    movzx edx, word [edi + 4]     ; Load tag->size into EDX
    add edx, 7                    ; tag->size + 7
    and edx, ~7                   ; Round tag->size up to nearest multiple of 8
    add edi, edx                  ; Update the tag pointer by the rounded size

    jmp .tag_scan               ; Repeat the loop

.loop_done:
    ret                          
    

.read_fb_info:
    ; Framebuffer address is at offset 8 within the framebuffer tag
    mov eax, [edi + 8]   ; Load framebuffer physical address
    mov dword [framebuffer_addr], eax


    ; Print newline (to move to the next line)
    mov esi, newline              ; Load address of newline character
    push esi
    call print_string_serial
    add esp, 4

    mov eax, [edi + 8]   ; Load framebuffer physical address
    push eax
    call print_hex_serial       ; Print frame buffer address
    add esp, 4


    ; Print newline (to move to the next line)
    mov esi, newline            ; Load address of newline character
    push esi
    call print_string_serial
    add esp, 4


    mov eax, [edi + 12]         ; Load framebuffer physical address, lower 32 bits
    mov dword [framebuffer_addr2], eax

    push eax
    call print_hex_serial       ; Print frame buffer address
    add esp, 4

    ; Print newline (to move to the next line)
    mov esi, newline            ; Load address of newline character
    push esi
    call print_string_serial
    add esp, 4


    mov eax, [edi + 16]         ; Load framebuffer physical address upper 32 bits
    mov dword [framebuffer_addr2], eax

    push eax
    call print_hex_serial       ; Print framebuffer pitch
    add esp, 4

    ; Print newline (to move to the next line)
    mov esi, newline            ; Load address of newline character
    push esi
    call print_string_serial
    add esp, 4

    mov eax, [edi + 20]   ; Load framebuffer width
    mov dword [framebuffer_width], eax
    push eax
    call print_hex_serial       ; Print 
    add esp, 4

    ; Print newline (to move to the next line)
    mov esi, newline            ; Load address of newline character
    push esi
    call print_string_serial
    add esp, 4


    mov eax, [edi + 24]         ; Load framebuffer height
    mov dword [framebuffer_height], eax
    push eax
    call print_hex_serial       ; Print 
    add esp, 4

    ; Print newline (to move to the next line)
    mov esi, newline            ; Load address of newline character
    push esi
    call print_string_serial
    add esp, 4

    xor eax,eax

    mov byte al, [edi + 28]         ; Load framebuffer depth
    mov dword [framebuffer_depth], eax
    push eax
    call print_hex_serial       ; Print 
    add esp, 4

    ; Print newline (to move to the next line)
    mov esi, newline            ; Load address of newline character
    push esi
    call print_string_serial
    add esp, 4


%if 1
    mov ecx,0x10ffff
.l2
    mov eax, ecx
    and eax,0xFFFF
    ;mov dword [0xfd000000+ecx*4], eax
    loop .l2
%endif 

    ret


.no_framebuffer:
    mov al, "Z"
    call print_char_serial
    
    jmp error

.no_multiboot:
    push dword  "M"

    call serial_write_char
   
    mov al, "M"
    jmp error


print_char:
    ; Print a character from AL to the screen (text mode)
    pusha
    mov ah, 0x07         ; White foreground, black background
    mov [edx], ax         ; Character and attribute byte
    popa
    add edx, 2            ; Move to next character position
    ret



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

%if 1

setup_page_tables:
    ; Set up L4 page table to point to L3 page table
    mov eax, page_table_l3
    or eax, 0b11                ; Present, Writable
    mov [page_table_l4], eax    ; Store L4 entry pointing to L3

    ; Set up L3 page table to point to L2 page table
    mov eax, page_table_l2
    or eax, 0b11                ; Present, Writable
    mov [page_table_l3], eax    ; Store L3 entry pointing to L2

    ; ================================
    ; Add the entry for page_table_l2_fb in page_table_l3
    ; ================================
    mov eax, page_table_l2_fb   ; Address of page_table_l2_fb
    or eax, 0b11                ; Present, Writable
    
    
    mov edx, [framebuffer_addr] ; load framebuffer address 
    shr edx, 30                 ; shift left to remove lower 30 bits
    mov [page_table_l3 + edx * 8], eax  ; L3 entry for framebuffer

    ; example
    ; mov [page_table_l3 + 3 * 8], eax  ; L3 entry for framebuffer
    ; the index 3 is chosen because the 9 bits for l3 from fd00 0000 are 0000 0001 1 = 3

    ; ================================
    ; Identity Mapping using Huge Pages (2MB)
    ; ================================
    xor ecx, ecx                ; Reset counter for identity mapping
.low_memory_loop:
    mov eax, ecx                ; Identity map: map 0x00000000 to 0x00000000
    or eax, 0b10000011          ; Present, Writable, Huge Page (2MB)
    mov [page_table_l2 + ecx * 8], eax
    inc ecx
    cmp ecx, 512                ; 512 entries = 1GiB
    jne .low_memory_loop        ; Repeat for 1GiB of low memory


    ; ================================
    ; Map framebuffer in L2 using huge pages (2MB)
    ; ================================
  
    mov edx, [framebuffer_addr] ; load framebuffer address 
    and edx, 0x3fe00000         ; keep bit 22 to 31
    shr edx, 21                 ; shift left to remove lower 21 bits
    mov eax, [framebuffer_addr] ; 0xFD000000         ; Load framebuffer address
    or eax, 0b10000011          ; Present, Writable, Huge Page (2MB)
    mov [page_table_l2_fb+(edx*8)], eax ; Store in L2 entry for framebuffer

    ;mov [page_table_l2_fb+(488*8)], eax ; Store in L2 entry for framebuffer
    ;mov [page_table_l2_fb+(320*8)], eax ; Store in L2 entry for framebuffer
    ;the index 488 = 1 1110 1000 is chosen because
    ; the 9 bits from bit 21 going left for fd00 0000 address = 488 

    ret
%endif 

%if 0
; The original mapping
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
%endif

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


; original  1GB memory layout
%if 0
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
%endif 

%if 1
section .bss
align 4096
page_table_l4: resb 4096           ; Level 4 page table (PML4)
page_table_l3: resb 4096           ; Level 3 page table (PDPT)
page_table_l2: resb 4096           ; Level 2 page table (PDT)
page_table_l2_fb: resb 4096        ; Level 2 page table for framebuffer (2MB page)
stack_bottom:
    resb 4096 * 4
stack_top:
%endif




section .rodata
gdt64:
    dq 0
.code_segment: equ $ - gdt64
    dq ( 1 << 43 ) |  (1 << 44) | ( 1 << 47) | (1<<53)
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .data
    str_ebx db "EBX Register: ", 0
    str_star_ebx db "*EBX Register: ", 0
    multiboot_end_type dw 0             ; MULTIBOOT_TAG_TYPE_END value (0)
    format_tag db "Tag 0x", 0           ; String for "Tag 0x"
    format_size db " Size 0x", 0        ; String for " Size 0x"
    newline db 0xA, 0                  ; Newline character (line feed)
    longmode_message db 0xA,"Entering 64bit Mode!",0xA