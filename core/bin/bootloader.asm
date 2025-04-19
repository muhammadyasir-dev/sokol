section .text
    global _start

_start:
    ; Set up the video mode
    mov ax, 0x03
    int 0x10

    ; Print "Hello, World!"
    mov si, hello_message
    call print_string

    ; Hang the system
hang:
    jmp hang

print_string:
    ; Print string pointed to by SI
    mov ah, 0x0E
.next_char:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .next_char
.done:
    ret

hello_message db 'Hello, World!', 0

section .bss
