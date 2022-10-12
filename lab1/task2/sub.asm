%define MULTI   42
%define P       43
%define N       45
%define ZERO    48


section .bss
x:          RESB    255
y:          RESB    255
input:      RESB    255
operator:   RESB    255
result:     RESB    255  
sign:       RESB    255
state       RESB    255

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
    ;解析参数
    mov BYTE[state], 0
    call parse_input
    cmp BYTE[state], 1
    je .main_loop
    ; 计算并输出结果
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
    mov eax, edx
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
    mov eax, x
    cmp BYTE[ecx], N
    jg .add_sign_x
    jmp .parse_x

    .add_sign_x:
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
        mov eax, operator
        mov dl, BYTE[ecx]
        mov BYTE[eax], dl
        inc eax
        inc ecx

        mov eax, y
        cmp BYTE[ecx], N
        jg .add_sign_y
        jmp .parse_y

    .add_sign_y:
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
        mov BYTE[state], 1
        ret
        
    .ret_parse:
        cmp BYTE[operator], MULTI   ; 判断符号是否合法
        jl .invalid
        cmp BYTE[operator], P
        jg .invalid
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
    ;结果地址存在edx
    mov edx, result
    ;结果默认为正数
    mov BYTE[sign], P
    ;判断是否乘法
    cmp BYTE[operator], MULTI
    jz domul
    ;加法
    mov al, BYTE[x]
    add al, BYTE[y]
    cmp al, 88 ; 43+45=88,说明是异号
    jnz doadd ;同号加法
    jmp dosub


;void doadd()
;同号整数相加
doadd:
    inc edx
    mov ecx, 0
    mov eax, x
    mov ebx, y
    cmp BYTE[eax], P ;判断结果符号,负号就加上'-'
    je  .addloop
    mov BYTE[sign], N

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
        mov BYTE[edx], al
        jmp .addloop


    .rest_y:
        cmp edi, y
        jle .end_add
        mov eax, 0
        add al, BYTE[edi]
        add al, cl
        mov ecx, 0
        dec edi
        dec edx
        mov BYTE[edx], al
        cmp al, 58
        jl .rest_y
        call carry_digit
        mov BYTE[edx], al
        jmp .rest_y

    .rest_x:
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
        jl .rest_x
        call carry_digit
        mov BYTE[edx], al
        jmp .rest_x

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

;esi被减数，edi减数
dosub:
    mov BYTE[sign], P   ; 结果默认为正
    mov edx, 0          ; 计数器
    call get_sub_operand; 获取操作数，esi指向被减数，edi指向减数，（减去'0'）
    cmp BYTE[x], N
    jz  .switch ; x是负数
    jmp .subloop

    .switch:  ; 交换传入顺序
        mov eax, esi
        mov esi, edi
        mov edi, eax


    .subloop:
        inc esi
        inc edi
        mov bl, 0
        inc edx
        cmp edx, 22
        je .modify      ; 每一位都减完就进入下一环节


        mov bl, BYTE[esi]
        mov cl, BYTE[edi]
        sub bl, cl

        mov BYTE[result+edx], bl
        jmp .subloop

    .modify:
        mov edx, 0
    .modifyloop:
        inc edx
        cmp edx, 21
        je  .backwave
        mov al, 0
    .inner:
        push eax
        push edx
        mov edx, 0
        mov dl, 10
        mul dl
        pop edx
        add al, BYTE[result+edx]
        cmp al, 0
        jl .continue

        mov BYTE[result+edx], al
        pop eax
        sub BYTE[result+edx+1], al
        jmp .modifyloop
    .continue:        
        pop eax
        inc al
        jmp .inner
    .backwave:
        ;如果首位是正，说明结果为正，直接返回
        ;首位为负，需要不断向后（高位到低位）修正结果，方法名也源于此
        cmp BYTE[result+22-1], 0
        jge .done   ; 减法完成
        mov BYTE[sign], N
        mov edx, 21
    .negative_proc:
        ;把高位的-1往低位推
        cmp edx, 1
        je .first_proc
        cmp BYTE[result+edx-1], 0
        jl .abs
        sub BYTE[result+edx-1], 10
        add BYTE[result+edx], 1
    .abs:
        ;化负为正
        mov al, BYTE[result+edx]
        mov BYTE[result+edx], 0
        sub BYTE[result+edx], al
        dec edx
        jmp .negative_proc
    .first_proc:
        ;上述过程可能会把第一位（个位）修正为10,需要手动处理
        mov al, BYTE[result+1]
        mov BYTE[result+1], 0
        sub BYTE[result+1], al
        mov ebx, 1
    .last_modify:
        cmp BYTE[result+ebx], 10
        jl .done
        mov BYTE[result+ebx], 0
        add BYTE[result+ebx+1], 1
        inc ebx
        jmp .last_modify
    .done:
        mov edx, result
        call sub_format
        call output_result
        ret

domul:
    mov al, BYTE[x]
    add al, BYTE[y]
    cmp al, 88 ; 43+45=88,说明是异号
    je .negative
    jmp .memset_mul

    .negative:
        mov BYTE[sign], N

    .memset_mul:
        mov eax, 0
        mov ecx, 0

    .outter_loop:           ; 乘法的外循环
        cmp edi, y
        jle .output_mul
        call inner_loop
        dec edi
        dec edx
        jmp .outter_loop
        

    .output_mul:
        mov edx, eax
        cmp BYTE[edx], 0
        je  .process_zero   ; 处理开头的'0'
        call format         ; 将结果加上'0'的ASCII码
        call output_result  ; 输出结果
        ret

    .process_zero:
        inc edx
        call format         ; 将结果加上'0'的ASCII码
        call output_result  ; 输出结果
        ret

inner_loop:
    push esi
    push ebx
    push edx

    .loop:
        cmp esi, x
        jle .finish
        mov eax, 0
        mov ebx, 0
        add al, BYTE[esi]
        sub al, ZERO
        add bl, BYTE[edi]
        sub bl, ZERO
        mul bl              ; 乘数在al和bl，结果在ax
        add BYTE[edx], al   ; edx所指内容可能有进位的数字
        mov al, BYTE[edx]
        mov ah, 0
        mov bl, 10
        div bl              ; al保存商，ah保存余数
        mov BYTE[edx], ah
        dec esi             ; 移动x的指针
        dec edx             ; 移动结果的指针
        add BYTE[edx], al
        jmp .loop        

    .finish:
        mov eax, edx
        pop edx
        pop ebx
        pop esi
        ret

carry_digit:
    sub al,  10
    mov ecx, 1
    ret

get_sub_operand:
    ;去除符号，减去'0'，esi和edi指向22位数字的开头
    push eax
    push ebx
    push edx
    .get_x:
        mov eax, x
        inc eax
    .x_loop:
        cmp BYTE[eax], 0
        je .x_finish
        sub BYTE[eax], ZERO
        mov bl, BYTE[eax]
        mov BYTE[eax+22], bl
        inc eax
        jmp .x_loop
    .x_finish:
        mov esi, eax
    .get_y:
        mov eax, y
        inc eax
    .y_loop:
        cmp BYTE[eax], 0
        je .y_finish
        sub BYTE[eax], ZERO
        mov bl, BYTE[eax]
        mov BYTE[eax+22], bl
        inc eax
        jmp .y_loop
    .y_finish:
        mov edi, eax
    pop edx
    pop ebx
    pop eax
    ret



sub_format:
    ; 用于减法结果加'0'
    push eax
    mov eax, 22
    ; 去除开头的0
    .process_zero:
        cmp BYTE[edx], 0
        push edx
        jnz .loop
        pop edx
        inc edx
        dec eax
        jmp .process_zero
    .loop:
        add BYTE[edx], ZERO
        inc edx
        dec eax
        cmp eax, 0
        jnz .loop
        pop edx
        pop eax
        ret

        



format:
    ; edx指向结果的开头（低地址）
    push edx
    push eax

    .loop:
        mov eax, result
        inc eax
        cmp edx, eax
        jge .finished
        add BYTE[edx], ZERO
        inc edx
        jmp .loop

    .finished:
        pop eax
        pop edx
        ret

output_result:
    mov eax, msg_result
    call print_str
    cmp BYTE[sign], N
    jz .output_sign
    jmp .output_num


    .output_sign:
        mov eax, sign
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
    
    mov ebx, 255
    
    .del_loop:
        and BYTE[eax], 0
        inc eax
        dec ebx
        cmp ebx, 0
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