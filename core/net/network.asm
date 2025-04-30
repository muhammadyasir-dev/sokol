[bits 16]
[org 0x7c00]

; Define the Ethernet frame
ethernet_frame:
    db 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  ; Destination MAC address (broadcast)
    db 0x00, 0x11, 0x22, 0x33, 0x44, 0x55  ; Source MAC address (example)
    dw 0x0800                               ; EtherType (IP)
    times 42 db 0                           ; Padding for minimum Ethernet frame size

; Function to send Ethernet frame
send_ethernet_frame:
    ; Assuming NIC is at I/O port 0x300
    mov dx, 0x300                          ; NIC I/O port
    mov si, ethernet_frame                 ; Address of the Ethernet frame
    mov cx, 60                             ; Number of bytes to send (60 for this example)

send_loop:
    lodsb                                  ; Load byte from SI to AL
    out dx, al                             ; Output byte to NIC
    loop send_loop                         ; Repeat for all bytes

    ret

start:
    call send_ethernet_frame
    jmp $                                  ; Infinite loop (for demonstration purposes)

times 510-($-$$) db 0
dw 0xaa55                                  ; Boot signature

