extrn MessageBoxA: proc ; You can call MessageBoxA
extrn printf: proc ; You can call printf

.data
text     db 'Hello x64!', 0 ; You can lea rdx,text
caption  db 'My First x64 Application', 0

.CODE  ;

vt_Add PROC  ;
    add rcx,rdx		;
    mov rax,rcx		;
    ret				;
vt_Add ENDP  ;º¯Êý½áÊø


asm_msg_box_x64 PROC  ;
    
    push rbp
    sub rsp,30h ; params is rcx,rdx,r8,r9,rsp+20,rsp+28..

    mov         qword ptr [rsp+20h],r9
    mov         qword ptr [rsp+18h],r8
    mov         qword ptr [rsp+10h],rdx
    mov         qword ptr [rsp+8],rcx

    mov r9,0
    mov r8, [rsp+8]
    mov rdx, [rsp+10h]
    mov rcx,0
    mov rax,[rsp+18h]
    call rax

    mov rcx,[rsp+20h]
    mov rdx,rax
    call printf
    
    add rsp,30h
    pop rbp
    ret

asm_msg_box_x64 ENDP  ;

asm_msg_box_x64_bak PROC  a:DWORD,b:DWORD,c:DWORD,d:DWORD;

    sub rsp,30h ;
    mov rax,r8
    mov r9,0
    mov r8, rcx
    mov rdx, rdx
    mov rcx,0
    call rax
    add rsp,30h
    ret

asm_msg_box_x64_bak ENDP  ;

END     ;