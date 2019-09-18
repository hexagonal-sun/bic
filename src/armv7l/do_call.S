#include "../../config.h"

	.text
	.arm
	.global __do_call
__do_call:
	// This macro will traverse the argument linked list and move
	// the appropriate argument into destreg.  If the linked list
	// pointer is equal to zero, the argument parsing is complete.
	.macro param_2_ireg destreg paramptr tmp
	cmp	\paramptr, #0
	beq	call_fn
	ldr	\tmp, [\paramptr, #0xc]
	cmp	\tmp, #1
	beq	vec_args
  cmp \tmp, #2
  beq stack_args
	ldr	\destreg, [\paramptr, #4]
	ldr	\paramptr, [\paramptr]
	.endm

	.macro param_2_dreg destreg paramptr tmp
	cmp	\paramptr, #0
	beq	call_fn
  ldr \tmp, [\paramptr, #0xc]
  cmp \tmp, #2
  beq stack_args
	vldr.64	\destreg, [\paramptr, #4]
	ldr	\paramptr, [\paramptr]
	.endm

  push {r4, r5, r6, r7, fp, lr}
	// Save the function call address.
	mov	r4, r0

	// Save the argument pointer.
	mov	r5, r1

  // Reset the number of elements pushed on the stack.
	mov   r7, #0

	// Marshal integer parameters into registers.
	param_2_ireg	r0, r5, r6
	param_2_ireg	r1, r5, r6
	param_2_ireg	r2, r5, r6
	param_2_ireg	r3, r5, r6

	// Marshal vector (float) parameters into registers.
vec_args:
	param_2_dreg  d0, r5, r6
	param_2_dreg  d1, r5, r6
  param_2_dreg  d2, r5, r6
	param_2_dreg  d3, r5, r6


  // Handle stack params
stack_args:
  cmp   r5, #0
  beq   call_fn
  ldr   r6, [r5, #4]
  push  {r6}
  add   r7, r7, #4
  ldr   r5, [r5]
  b     stack_args

call_fn:
	blx r4
  add sp, sp, r7
	pop {r4, r5, r6, r7, fp, pc}
