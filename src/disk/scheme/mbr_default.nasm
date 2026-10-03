[BITS 16]
[ORG 0x7C00]

START:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov si, message
.PLOOP:
    lodsb ; "mov al, [si]" then "si++"
    or al, al
    jz .HSTART
    ; Interrupt int=10 ah=0E (VIDEO - TELETYPE OUTPUT)
    mov ah, 0x0E
    ; al = character to write
    mov bh, 0 ; Page number 0
    mov bl, 7 ; Foreground color light grey
    int 0x10
    jmp .PLOOP

.HSTART:
    cli
.HLOOP:
    hlt
    jmp .HLOOP



message db 'MBR Bootloader: No proper bootloader code has been set. Halting...', 13, 10, 0

; Bootloader limit before other MBR data
times 440 - ($ - $$) db 0
; Unique disk signature (used mostly by windows)
dd 0
; Write protection ({0x0 for writable, 0x5A5A for write-protected)
;   source: "en.wikipedia.org/wiki/Master_boot_record"
;   and the UEFI spec literally just says "Unknown"
dw 0
; Partition table
times 16*4 db 0
; MBR signature
dw 0xAA55
