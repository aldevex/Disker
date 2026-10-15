[BITS 16]
[ORG 0x7C00]

    jmp 0x0:START ; Set code segment to 0
START:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov si, message
.PRINT_LOOP:
    lodsb ; al = [si], si++
    test al, al
    jz .PRINT_END
    ; Interrupt 10-0E : video teletype output
    ; https://www.delorie.com/djgpp/doc/rbinter/id/11/1.html
    mov bh, 0 ; Page number 0
    mov bl, 7 ; Foreground color light grey
    ;   al    ; Character to write
    mov ah, 0x0E
    int 0x10
    jmp .PRINT_LOOP
.PRINT_END:

    cli
.HALT_LOOP:
    hlt
    jmp .HALT_LOOP



message: db 'Disker MBR Bootloader: No proper bootloader code has been set. Halting...', 13, 10, 0

db "This MBR has been written by ***Disker*** :)", 0

; Bootloader limit before other MBR data
times 440 - ($ - $$) db 0
; Unique disk signature (used mostly by windows I assume)
dd 0
; Legacy (DOS maybe?) write protection ({0x0 for writable, 0x5A5A for write-protected)
;   source: "https://en.wikipedia.org/wiki/Master_boot_record"
;   and the UEFI spec literally just says "Unknown" 💀
dw 0
; Partition table
times 16*4 db 0
; MBR signature
dw 0xAA55
