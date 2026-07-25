.intel_syntax noprefix

.global main
.global sys_write
.global sys_read
.global sys_open
.global sys_close
.global sys_mkdir
.global sys_reboot
.global sys_kexec_file_load
.global sys_mmap
.global sys_munmap
.global sys_fstat
.global sys_fsync
.global _start

.text

_start:
    mov rdi, [rsp]
    lea rsi, [rsp + 8]
    sub rsp, 8
    call main
    mov rdi, rax
    mov rax, 60
    syscall

sys_write:
    mov rax, 1
    syscall
    ret

sys_read:
    mov rax, 0
    syscall
    ret

sys_open:
    mov rax, 2
    syscall
    ret

sys_close:
    mov rax, 3
    syscall
    ret

sys_stat:
    mov rax, 4
    syscall
    ret

sys_fstat:
    mov rax, 5
    syscall
    ret

sys_mknod:
    mov rax, 133
    syscall
    ret

sys_mmap:
    mov rax, 9
    mov r10, rcx
    syscall
    ret

sys_munmap:
    mov rax, 11
    syscall
    ret

sys_mkdir:
    mov rax, 83
    syscall
    ret

sys_reboot:
    mov rax, 169
    mov r10, rcx
    syscall
    ret

sys_kexec_file_load:
    mov rax, 320
    mov r10, rcx
    syscall
    ret

sys_fsync:
    mov rax, 74
    syscall
    ret
