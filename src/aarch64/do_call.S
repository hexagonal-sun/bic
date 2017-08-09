#include "../../config.h"

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
	.macro param_2_ireg destreg paramptr tmp
	cmp	\paramptr, 0
	beq	call_fn
	ldr	\tmp, [\paramptr, 16]
	cmp	\tmp, 1
	beq	vec_args
	ldr	\destreg, [\paramptr, 8]
	ldr	\paramptr, [\paramptr]
	.endm

	.macro param_2_vreg destreg paramptr
	cmp	\paramptr, 0
	beq	call_fn
	ldr	\destreg, [\paramptr, 8]
	ldr	\paramptr, [\paramptr]
	.endm

	stp	x29, x30, [sp, -0x10]!
	mov	x29, sp

	// Save the function call address.
	mov	x9, x0

	// Save the argument pointer.
	mov	x10, x1

	// Marshal integer parameters into registers.
	param_2_ireg	x0, x10, w11
	param_2_ireg	x1, x10, w11
	param_2_ireg	x2, x10, w11
	param_2_ireg	x3, x10, w11
	param_2_ireg	x4, x10, w11
	param_2_ireg	x5, x10, w11
	param_2_ireg	x6, x10, w11
	param_2_ireg	x7, x10, w11

	// Marshal vector (float) parameters into registers.
vec_args:
	param_2_vreg	d0, x10
	param_2_vreg	d1, x10
	param_2_vreg	d2, x10
	param_2_vreg	d3, x10
	param_2_vreg	d4, x10
	param_2_vreg	d5, x10
	param_2_vreg	d6, x10
	param_2_vreg	d7, x10
call_fn:
	blr	x9
	ldp	x29, x30, [sp], 0x10
	ret
