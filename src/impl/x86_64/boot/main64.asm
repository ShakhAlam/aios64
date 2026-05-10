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

        ; Print " Entering 64bit mode"
    mov al, 'A'
    mov dx, 0x3F8              ; Data Register (THR)
    out dx, al



%if 0
; Draw a red pixel at (200, 150)
    mov rax, [framebuffer_addr]    ; Load the framebuffer address
    add rax, (150 * 1024 + 200) * 4 ; Calculate pixel offset (BGRA format)
    mov dword [rax], 0xFFFFFFFF    ; Write a red pixel (BGRA: 00 FF 00 00)
    mov dword [rax+8], 0xFFFFFFFF    ; Write a red pixel (BGRA: 00 FF 00 00)
    mov dword [rax+16], 0xFFFFFFFF    ; Write a red pixel (BGRA: 00 FF 00 00)
%endif

%if 0
mov ecx, 0x5FFFF
.l2:
    mov  [0xFD000000 + ecx * 4], ecx
    loop .l2
%endif
    

    call kernel_main
    hlt
section .data
    longmode_message2 db 0xA,"Entered 64bit Mode!",0xA