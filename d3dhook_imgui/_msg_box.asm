extrn MessageBoxA: proc ; You can call MessageBoxA

.data
text     db 'Hello x64!', 0 ; You can lea rdx,text
caption  db 'My First x64 Application', 0

.CODE  ;

vt_Add PROC  ;
    add rcx,rdx		;
    mov rax,rcx		;
    ret				;
vt_Add ENDP  ;º¯Êý½áÊø


asm_msg_box_x64 PROC  a:DWORD,b:DWORD,c:DWORD,d:DWORD;

    sub rsp,30h ; params is rcx,rdx,r8,r9...
    mov rax,r8
    mov r9,0
    mov r8, rcx
    mov rdx, rdx
    mov rcx,0
    call rax
    add rsp,30h
    ret

asm_msg_box_x64 ENDP  ;

END     ;