
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_s_disp_str      equ 1
_NR_s_delay         equ 2
_NR_p		     equ 3
_NR_v		     equ 4

INT_VECTOR_SYS_CALL equ 0x90

extern  disp_str

; 导出符号
global	get_ticks
global	s_disp_str
global  s_delay
global	sys_disp_str
global  p
global  v

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret


s_disp_str:
	mov	eax,_NR_s_disp_str
	push	ebx
	mov	ebx,[esp+8]
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret
	
sys_disp_str:
	pusha
	push ebx
	call disp_str
	pop ebx
	popa
	ret
	
	
s_delay:
	mov	eax,_NR_s_delay
	push	ebx
	mov	ebx,[esp+8]
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret
	
p:
	mov	eax,_NR_p
	push	ebx
	mov	ebx,[esp+8]
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret
	
v:
	mov	eax,_NR_v
	push	ebx
	mov	ebx,[esp+8]
	int	INT_VECTOR_SYS_CALL
	pop	ebx
	ret