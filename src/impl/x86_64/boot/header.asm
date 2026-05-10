section .multiboot_header
    align 8                  ; The header must be 64-bit aligned

    ; Multiboot header magic number
    dd 0xE85250D6             ; magic
    dd 0                      ; architecture (0 for 32-bit)
    dd header_end - header_start ; header_length
    dd -(0xE85250D6 + 0 + (header_end - header_start)) ; checksum

align 8
header_start:
%if 1
align 8
header_framebuffer_tag:
    dw 5                                    ; framebuffer settings
    dw 0
    dd header_framebuffer_tag_end - header_framebuffer_tag
    dd 1920
    dd 1080
    dd 32
header_framebuffer_tag_end:
%endif 
align 8
end_tag:
    ; End tag
    dw 0                      ; type = 0 (end tag)
    dw 0
    dd 8                      ; size
align 8
header_end:
