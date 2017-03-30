#include "../../config.h"

	.intel_syntax noprefix
	.text
#ifdef SYMBOL_USCORE
	.global ___do_call
___do_call:
#else
	.global __do_call
__do_call:
#endif
	// This macro will traverse the argument linked list and move
	// the appropriate argument into destreg.  If the linked list
	// pointer is equal to zero, the argument parsing is complete.
	.macro param_2_reg destreg paramptr
	test	\paramptr, \paramptr
	jz	call_fn
	mov	\destreg, [\paramptr + 8]
	mov	\paramptr, [\paramptr]
	.endm

	push	rbp
	mov	rbp, rsp
	push	r10
	push	r11

	// Save the function call address.
	mov	r11, rdi

	// Save the argument pointer.
	mov	r10, rsi

	// Marshal integer parameters into registers.
	param_2_reg	rdi, r10
	param_2_reg	rsi, r10
	param_2_reg	rdx, r10
	param_2_reg	rcx, r10
	param_2_reg	r8,  r10
	param_2_reg	r9,  r10
call_fn:
	call	r11
	pop	r11
	pop	r10
	leave
	ret
