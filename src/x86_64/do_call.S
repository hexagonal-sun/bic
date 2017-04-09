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
	.macro param_2_ireg destreg paramptr
	test	\paramptr, \paramptr
	jz	call_fn
	mov	r12, [\paramptr + 16]
	cmp	r12, 1
	je	vec_args
	mov	\destreg, [\paramptr + 8]
	mov	\paramptr, [\paramptr]
	.endm

	.macro param_2_vreg destreg paramptr
	test	\paramptr, \paramptr
	jz	call_fn
	movsd	\destreg, [\paramptr + 8]
	inc	rax
	mov	\paramptr, [\paramptr]
	.endm

	push	rbp
	mov	rbp, rsp

	// We don't use this stack space, but we need to ensure that
	// it remains 16-byte aligned.
	sub	rsp, 8
	push	r10
	push	r11
	push	r12

	// Save the function call address.
	mov	r11, rdi

	// Save the argument pointer.
	mov	r10, rsi

	// Clear rax so it can count the number of vector registers.
	mov	rax, 0

	// Marshal integer parameters into registers.
	param_2_ireg	rdi, r10
	param_2_ireg	rsi, r10
	param_2_ireg	rdx, r10
	param_2_ireg	rcx, r10
	param_2_ireg	r8,  r10
	param_2_ireg	r9,  r10

	// Marshal vector (float) parameters into registers.
vec_args:
	param_2_vreg	xmm0, r10
	param_2_vreg	xmm1, r10
	param_2_vreg	xmm2, r10
	param_2_vreg	xmm3, r10
	param_2_vreg	xmm4, r10
	param_2_vreg	xmm5, r10
	param_2_vreg	xmm6, r10
	param_2_vreg	xmm7, r10
call_fn:
	call	r11
	pop	r12
	pop	r11
	pop	r10
	add	rsp, 8
	leave
	ret
