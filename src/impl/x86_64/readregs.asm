global getCR3
global getRIP
;return the cr3 register

section .text

getCR3:
    mov rax,cr3
    ret

getRIP:
    lea rax, [rel rip_label]
    ret
rip_label



