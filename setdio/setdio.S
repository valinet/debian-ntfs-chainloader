bits 64
org 0x10000

db 0x7F, "ELF"          ; Magic
db 2, 1, 1, 0           ; 64-bit, little endian, ELF v1, SYSV
times 8 db 0            ; Padding

dw 2                    ; Executable
dw 0x3E                 ; x86_64
dd 1                    ; ELF v1
dq _start               ; Entry point
dq 0x40                 ; Program header offset
dq 0                    ; Section header offset
dd 0                    ; Flags
dw 64                   ; ELF header size
dw 56                   ; Program header size
dw 1                    ; 1 program header
dw 0                    ; Section header size
dw 0                    ; 0 section headers
dw 0                    ; Section name index

; Program header
dd 1                    ; PT_LOAD
dd 5                    ; R-X
dq 0                    ; File offset
dq $$                   ; Virtual address
dq $$                   ; Physical address
dq _fin - $$            ; File size
dq _fin - $$            ; Memory size
dq 0x1000               ; Alignment

_start:
    mov rdi, [rsp]
    lea rsi, [rsp + 8]
    sub rsp, 8
    xor rax, rax
    dec rax
    cmp rdi, 2
    jnz _end
    mov rdi, [rsi + 8]
    mov rsi, 2
    xor rdx, rdx
    call sys_open
    push rax
    mov rdi, rax
    mov rsi, 0x4C08
    xor rdx, rdx
    inc rdx
    call sys_ioctl
    pop rdi
    call sys_close
    xor rax, rax
_end:
    mov rdi, rax
    mov rax, 60
    syscall

sys_exit:
    mov rax, 60
    syscall
    ret

sys_open:
    mov rax, 2
    syscall
    ret

sys_ioctl:
    mov rax, 16
    syscall
    ret

sys_close:
    mov rax, 3
    syscall
    ret

_fin:
