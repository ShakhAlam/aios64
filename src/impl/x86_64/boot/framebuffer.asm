global  framebuffer_addr, framebuffer_addr2 
global framebuffer_width, framebuffer_height
global framebuffer_depth

section .bss
align 8
framebuffer_addr:   resq 1
framebuffer_addr2:   resq 1
framebuffer_width: resd 1
framebuffer_height: resd 1
framebuffer_depth: resd 1
