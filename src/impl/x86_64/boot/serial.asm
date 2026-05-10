bits 32

section .text
global serial_init, serial_write, serial_write_char, print_string_serial, print_char_serial
global print_hex_serial

%if 1
; Initialize the serial port (COM1) for 9600-8N1N
serial_init:
    pusha
    pushf

    mov dx, 0x3F8             ; COM1 base port

    ; Disable interrupts
    mov al, 0x00
    out dx, al                ; Interrupt Enable Register (IER)

    ; Set baud rate divisor for 9600
    mov dx, 0x3FB             ; Line Control Register (LCR)
    mov al, 0x80              ; Enable DLAB
    out dx, al

    mov dx, 0x3F8             ; Divisor Latch Low Byte
    mov al, 0x0C              ; Low byte of 12
    out dx, al
    inc dx                    ; 0x3F9: Divisor Latch High Byte
    mov al, 0x00              ; High byte of 12
    out dx, al

    mov dx, 0x3FB             ; Line Control Register
    mov al, 0x03              ; 8 data bits, no parity, 1 stop bit => 8N1
    out dx, al

    ; Enable FIFO, clear them, 14-byte threshold
    mov dx, 0x3FA             ; FIFO Control Register (FCR)
    mov al, 0xC7
    out dx, al

    ; Modem Control Register (MCR): DTR, RTS, OUT2
    mov dx, 0x3FC
    mov al, 0x0B
    out dx, al

    popf
    popa
    ret
%endif

%if 0
; Initialize the serial port (COM1)
serial_init:
    pusha                    ; Save all registers
    pushf

    mov dx, 0x3F8             ; COM1 base port

    ; Disable interrupts
    mov al, 0x00
    out dx, al                ; Interrupt Enable Register (IER)

    ; Set baud rate divisor (115200 baud rate)
    mov al, 0x80              ; Enable DLAB (Divisor Latch Access Bit)
    out dx, al                ; Line Control Register (LCR)

    ; Set divisor to 1 (115200 baud)
    mov dx, 0x3F8
    mov al, 0x01              ; Divisor low byte
    out dx, al
    inc dx
    mov al, 0x00              ; Divisor high byte
    out dx, al

    ; 8 bits, no parity, one stop bit
    mov dx, 0x3F8
    mov al, 0x03              ; 8N1 (8 data bits, no parity, 1 stop bit)
    out dx, al

    ; Enable FIFO, clear them, with 14-byte threshold
    mov dx, 0x3FA
    mov al, 0xC7              ; Enable FIFO, clear RX/TX queues
    out dx, al

    ; Mark data terminal ready, signal request to send, and enable the UART
    mov dx, 0x3FC
    mov al, 0x0B              ; Set DTR, RTS, and OUT2
    out dx, al

    popf
    popa                      ; Restore all registers
    ret
%endif

; Send a single character (in CL) to the serial port (COM1)
serial_write:
    pusha                     ; Save all registers

    ; Wait for the transmitter to be empty
.wait:
    mov dx, 0x3FD              ; Line Status Register (LSR)
    in al, dx
    test al, 0x20              ; Check if THRE (bit 5) is set
    jz .wait                   ; Loop until ready

    ; Send the character in AL
    mov al, cl
    mov dx, 0x3F8              ; Data Register (THR)
    out dx, al

    popa                       ; Restore all registers
    ret

; Send a single character (in CL) to the serial port (COM1)
serial_write_char:
    ; Prologue
    push ebp              ; Save the base pointer
    mov ebp, esp          ; Establish a new stack frame
    pushf                 ; Save EFLAGS
    pusha                 ; Save all general-purpose registers

    ; Access the argument 'a' from the stack (for demonstration)
    mov ecx, [ebp + 8]     ; Load the argument 'a' into ECX

    ; Wait for the transmitter to be empty
.wait:
    mov dx, 0x3FD              ; Line Status Register (LSR)
    in al, dx
    test al, 0x20              ; Check if THRE (bit 5) is set
    jz .wait                   ; Loop until ready

    ; Send the character in CL
    mov al, cl
    mov dx, 0x3F8              ; Data Register (THR)
    out dx, al

    ; Epilogue
    popa                  ; Restore all general-purpose registers
    popf                   ; Restore EFLAGS
    mov esp, ebp           ; Restore the original stack pointer
    pop ebp                ; Restore the base pointer
    ret                    ; Return to the caller


print_string_serial:
    ; Preamble
    push ebp              ; Save base pointer
    mov ebp, esp          ; Set up stack frame
    pushf                 ; Save flags
    pusha                 ; Save general-purpose registers

    mov esi, [ebp + 8]    ; Load the address of the string from the argument

.print_char:
    mov al, [esi]         ; Load a character from the string
    test al, al           ; Check if it's the null terminator
    jz .done              ; Jump if zero (end of string)

    call print_char_serial ; Print the character

    inc esi               ; Move to the next character
    jmp .print_char        ; Repeat

.done:
    ; Cleanup
    popa                  ; Restore general-purpose registers
    popf                  ; Restore flags
    mov esp, ebp           ; Restore stack pointer
    pop ebp                ; Restore base pointer
    ret                    ; Return to caller


; Print a character from AL to the serial port
print_char_serial:
    pusha
    push eax
    call serial_write_char
    add esp,4
    popa
    ret


print_hex_serial:
    pusha                    ; Save registers

    mov ecx, 28              ; Start with the highest nibble (7 * 4 bits)

.hex_loop:
    mov ebx, eax             ; Copy EAX to EBX
    shr ebx, cl              ; Shift to extract the current nibble
    and bl, 0x0F             ; Isolate the lowest nibble

    cmp bl, 10
    jl .digit

    add bl, 55               ; Convert to ASCII A-F
    jmp .output

.digit:
    add bl, 48               ; Convert to ASCII 0-9

.output:
    push ebx
    call serial_write_char   ; Print the hex digit to serial port
    add esp, 4

    sub cl, 4                ; Move to the next nibble (4 bits down)
    jns .hex_loop            ; Jump if CL is non-negative

    popa                     ; Restore registers
    ret
