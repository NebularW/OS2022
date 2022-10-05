section .bss
input:      RESB    255
x:          RESB    255
y:          RESB    255
operator:   RESB    255
result:     RESB    255  


section .data
msg     db  "Invalid",0h

section .text
global _start
;------------------main函数----------------
_start:
    mov ebp, esp; for correct debugging
    
.loop:
    ;获取一行输入
    mov eax, input
    mov ebx, 255
    call getline
    ;检查是否'q'
    mov ecx, input
    call check_quit
    cmp eax, 1
    jz .quit_loop
    ;存参并计算
    mov eax, x
    mov ebx, y
    mov edx, operator
    call parse_input
    call cal
    ;输出结果
    ;mov eax, result
    ;call print_str
    
    ;清空x,y,operator,input
    mov eax, x
    call del_str
    mov eax, y
    call del_str
    ;mov eax, operator
;    call del_str
    mov eax, input
    call del_str
    
    jmp .loop

.quit_loop:
    call quit 


;int check_quit(char*)
;检查输入的是否'q'
;输入
;ecx 输入字符串
;输出
;eax 是'q',1;不是'q',0
check_quit:
    mov eax, ecx
    call slen
    cmp eax, 2
    jnz .quit_check
    cmp BYTE[ecx], 113
    jnz .quit_check
    inc ecx
    cmp BYTE[ecx], 10
    mov eax, 1
    ret
   
.quit_check:
    mov eax, 0
    ret 


;解析输入语句
;输入
;eax: x地址
;ebx: y地址
;ecx: 语句
;edx: operator地址
parse_input:
    push ebx
    push edx
.parse_x:
    cmp BYTE[ecx], 45
    jl .parse_operator
    mov dl, BYTE[ecx]
    mov BYTE[eax], dl
    inc eax
    inc ecx
    jmp .parse_x

.parse_operator:
    pop eax ;弹出的是operator的弟子
    cmp BYTE[ecx], 42
    jl .invalid
    cmp BYTE[ecx], 43
    jg  .invalid
    mov dl, BYTE[ecx]
    mov BYTE[eax], dl
    inc eax
    inc ecx
    pop eax ;弹出的是y的地址
    jmp .parse_y

.parse_y:
    cmp BYTE[ecx], 10
    jz .ret_parse
    mov dl, BYTE[ecx]
    mov BYTE[eax], dl
    inc eax
    inc ecx
    jmp .parse_y

.invalid:
    mov eax, msg
    call print_str

.ret_parse:
    inc ecx
    ret




 

;----------------工具函数------------------
;int slen(char*)
;计算字符串长度
;输入
;eax 字符串
;输出
;eax 长度
slen:
    push ebx
    mov ebx, eax
nextchar:
    cmp BYTE[eax], 0
    jz finished
    inc eax
    jmp nextchar
finished:
    sub eax, ebx
    pop ebx
    ret

;----------------------------------
;void del_str(char*)
;清空地址
;输入
;eax 变量地址
del_str:
    push ebx
    push eax
    
    mov ebx, eax
    call slen
    jmp .del_loop
    
.del_loop:
    and BYTE[ebx],0
    inc ebx
    dec eax
    cmp eax, 0
    jnz .del_loop
    
    pop ebx
    pop eax
    ret
    
    


;----------------------------------
;void print_str(char*)
;打印字符串
print_str:
    push edx
    push ecx
    push ebx
    push eax
    
    call slen
    mov edx, eax
    pop eax
    mov ecx, eax
    mov ebx, 1
    mov eax, 4
    int 80h
    
    pop ebx
    pop ecx
    pop edx
    
    ret

;----------------------------------
;void print_char(char*)
;打印字符
print_char:
    push edx
    push ecx
    push ebx
    push eax
    
    mov edx, 1
    mov ecx, esp
    mov ebx, 1
    mov eax, 4
    int 80h
     
    pop eax
    pop ebx
    pop ecx
    pop edx
    
    ret
    
;----------------------------------
;void printLF()
;打印换行符
printLF:
    push eax
    mov eax, 0ah
    push eax
    mov eax, esp
    call print_str
    pop eax
    pop eax
    ret


;----------------------------------
;void print_digit(int*)
;打印数字
print_digit:
    push eax
    push ecx
    push edx
    push esi
    mov ecx, 0

divideLoop:
    inc ecx
    mov edx, 0
    mov esi, 10
    idiv esi      ;商在eax，余数在edx
    add edx, 48
    push edx
    cmp eax, 0
    jnz divideLoop

printLoop:
    dec ecx
    mov eax, esp
    call print_char
    pop edx
    cmp ecx, 0
    jnz printLoop

    pop esi
    pop edx
    pop ecx
    pop eax
    ret


;----------------------------------
;void getline()
;获取一行输入
getline:
    push edx
    push ecx
    push ebx
    push eax

    mov edx, ebx
    mov ecx, eax
    mov ebx, 1
    mov eax, 3
    int 80h
    
    pop eax
    pop ebx
    pop ecx
    pop edx

    ret


;----------------------------------
;void exit()
;退出程序
quit:
    mov ebx, 0
    mov eax, 1
    int 80h
    ret
    

