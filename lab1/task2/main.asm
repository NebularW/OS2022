%define MULTI   42
%define P       43
%define N       45
%define ZERO    48



section .bss
;input:      RESB    255
x:          RESB    255
y:          RESB    255
input:      RESB    255
operator:   RESB    255
result:     RESB    255  
signal:     RESB    255

section .data
msg_hello       db  "Please input an expression: ",0h
msg_invalid     db  "Invalid",0h
msg_result      db  "The result is: ",0h


section .text
global _start
;------------------main函数----------------
_start:
    
.main_loop:
    ;输出提示语
    mov eax, msg_hello
    call print_str
    call printLF
    ;获取一行输入
    mov eax, input
    mov ebx, 255
    call getline
    ;检查是否'q'
    mov ecx, input
    call check_quit
    cmp eax, 1
    jz .quit_loop
    ;解析参数，计算并输出结果
    mov eax, x
    mov ebx, y
    mov edx, operator
    call parse_input
    call cal
    ;清空x,y,operator,input
    mov eax, x
    call memset
    mov eax, y
    call memset
    mov eax, operator
    call memset
    mov eax, input
    call memset
    ;继续接收输入
    jmp .main_loop

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


;void parse_input(char*)
;解析输入语句
;输入
;eax: x地址
;ebx: y地址
;ecx: 语句
;edx: operator地址
parse_input:
    push ebx
    push edx
    
    cmp BYTE[ecx], N
    jg .add_signal_x
    jmp .parse_x

    .add_signal_x:
        mov dl, P
        mov BYTE[eax], dl
        inc eax
        jmp .parse_x

    .parse_x:
        cmp BYTE[ecx], N
        jl .parse_operator
        mov dl, BYTE[ecx]
        mov BYTE[eax], dl
        inc eax
        inc ecx
        jmp .parse_x

    .parse_operator:
        pop eax ;弹出的是operator的地址
        cmp BYTE[ecx], MULTI
        jl .invalid
        cmp BYTE[ecx], P
        jg  .invalid
        mov dl, BYTE[ecx]
        mov BYTE[eax], dl
        inc eax
        inc ecx

        pop eax ;弹出的是y的地址
        cmp BYTE[ecx], N
        jg .add_signal_y
        jmp .parse_y

    .add_signal_y:
        mov dl, P   
        mov BYTE[eax], dl
        inc eax
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
        mov eax, msg_invalid
        call print_str
        call printLF

    .ret_parse:
        inc ecx
        ret




;void cal()
;运算得出结果
cal:
    ;esi,edi分别指向x,y的尾部
    mov eax, x
    call slen
    mov esi, x
    add esi, eax
    sub esi, 1
    mov eax, y
    call slen
    mov edi, y
    add edi, eax
    sub edi, 1
    ;结果地址存在edx(后续循环会先dec再存入，所以这里先inc)
    mov edx, result
    inc edx 
    ;结果默认为正数
    mov BYTE[signal], P
    ;判断是否乘法
    cmp BYTE[operator], MULTI
    jz domul
    ;加法
    mov al, BYTE[x]
    add al, BYTE[y]
    cmp al, 88 ; 43+45=88,说明是异号
    jnz doadd ;同号加法
    cmp BYTE[x], N
    jz  .switch ; x是正数
    ; jmp dosub
    .switch:  ; 交换传入顺序
        mov eax, esi
        mov esi, edi
        mov edi, eax
        ;jmp dosub


;void doadd()
;同号整数相加
doadd:
    mov ecx, 0
    mov eax, x
    mov ebx, y
    cmp BYTE[eax], P ;判断结果符号,负号就加上'-'
    je  .addloop
    mov BYTE[signal], N

    .addloop:
        cmp esi, x          ;判断x是否到第一位（符号位）
        jle .rest_y
        cmp edi, y
        jle .rest_x
        mov eax, 0
        add al, BYTE[esi]
        sub al, ZERO        ;减去'0':48
        add al, BYTE[edi]   ;按位加
        add al, cl          ;加上进位
        mov ecx, 0          ;清空进位
        dec esi             ;移动x的指针
        dec edi             ;移动y的指针
        dec edx             ;移动结果的指针
        cmp al, 58          ;判断是否越界（58=48+10）
        mov BYTE[edx], al
        jl  .addloop        ;没有越界则继续循环
        call carry_digit
        mov BYTE[eax],dl
        jmp .addloop


    .rest_x:
        cmp edi, y
        jle .end_add
        mov eax, 0
        add al, BYTE[edi]
        add al, cl
        mov ecx, 0
        dec esi
        dec edx
        mov BYTE[edx], al
        cmp al, 58
        jl .rest_x
        call carry_digit
        mov BYTE[edx], al
        jmp .rest_x

    .rest_y:
        cmp esi, x
        jle .end_add
        mov eax, 0
        add al, BYTE[esi]
        add al, cl
        mov ecx, 0
        dec esi
        dec edx
        mov BYTE[edx], al
        cmp al, 58
        jl .rest_y
        call carry_digit
        mov BYTE[edx], al
        jmp .rest_y

    .end_add:
        cmp ecx, 1
        jz .add_carry
        call output_result
        ret


    .add_carry:
        mov al, 49
        dec edx
        mov BYTE[edx], al
        call output_result
        ret


;dosub:
    



domul:
    mov ecx, 0
    mov eax, 0

    .mulloop:
        cmp edi, y
        jle output_result ;可能有bug
        

    


carry_digit:
    sub al,  10
    mov ecx, 1
    ret


output_result:
    mov eax, msg_result
    call print_str
    cmp BYTE[signal], N
    jz .output_signal
    jmp .output_num


    .output_signal:
        mov eax, signal
        call print_str
        jmp .output_num


    .output_num:
        mov eax, edx
        call print_str
        call printLF
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
    .nextchar:
        cmp BYTE[eax], 0
        jz .finished
        inc eax
        jmp .nextchar
    .finished:
        sub eax, ebx
        pop ebx
        ret

;----------------------------------
;void memset(char*)
;清空地址
;输入
;eax 变量地址
memset:
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
    


