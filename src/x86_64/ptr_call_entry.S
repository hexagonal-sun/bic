#include "../../config.h"

	.intel_syntax noprefix
	.text
ptr_call_handler:

	push	r11
	push	r9
	push	r8
	push	rcx
	push	rdx
	push	rsi
	push	rdi

	mov	rdi, rsp
	call	handle_ptr_call

	leave
	ret

	.macro	entry_point number
#ifdef SYMBOL_USCORE
	.global	_entry_point_\number
_entry_point_\number:
#else
	.global	entry_point_\number
entry_point_\number:
#endif
	push	rbp
	mov	rbp, rsp
#ifdef SYMBOL_USCORE
	lea	r11, [rip + _entry_point_\number]
#else
	lea	r11, [rip + entry_point_\number]
#endif
	call	ptr_call_handler
	ret
	.endm

	entry_point	0
	entry_point	1
	entry_point	2
	entry_point	3
	entry_point	4
	entry_point	5
	entry_point	6
	entry_point	7
	entry_point	8
	entry_point	9
	entry_point	10
	entry_point	11
	entry_point	12
	entry_point	13
	entry_point	14
	entry_point	15
	entry_point	16
	entry_point	17
	entry_point	18
	entry_point	19
	entry_point	20
	entry_point	21
	entry_point	22
	entry_point	23
	entry_point	24
	entry_point	25
	entry_point	26
	entry_point	27
	entry_point	28
	entry_point	29
	entry_point	30
	entry_point	31
	entry_point	end
