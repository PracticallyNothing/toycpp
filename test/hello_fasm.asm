format ELF64 executable

_start:
    mov rax, 1                  ; sys_write(rdi, rsi, rdx)
    mov rdi, 1                  ; 0 = stdin, 1 = stdout, 2 = stderr
    mov rsi, hello              ; pointer to string
    mov rdx, 417                ; size of string
    syscall

    mov rax, 60                 ; sys_exit(fd)
    mov rdi, 71                 ; return code: 71
    syscall

hello:
    file "hello_fasm.asm"
