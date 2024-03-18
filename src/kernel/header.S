header:
    length dd end - start
    mov ax,0x1234
    start_addr dd start


start:
    mov ax,0x1234
    jmp $
end
