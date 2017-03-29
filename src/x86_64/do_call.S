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
	push	rbp
	mov	rbp, rsp
	push	r10
	push	r11

	// Save the function call address.
	mov	r11, rdi

	mov	rdi, [rsi+8]
	call	r11
	pop	r11
	pop	r10
	leave
	ret
