#include "../../config.h"

	.text
ptr_call_handler:

  stp x29, x30, [sp, #-0x50]!
	stp	x6, x7, [sp, #0x40]
	stp	x4, x5, [sp, #0x30]
	stp	x2, x3, [sp, #0x20]
	stp	x0, x1, [sp, #0x10]

	add	x0, sp, #0x10
  mov x1, x9
	bl	handle_ptr_call
  ldp x29, x30, [sp], #0x50
	ret

	.macro	entry_point number

	.global	entry_point_\number
entry_point_\number:
	stp 	x29, x30, [sp, #-0x20]!
  str 	x9, [sp]
  mov 	x29, sp
	ldr 	x9, #=entry_point_\number
	bl   	ptr_call_handler
  ldr   x9, [sp]
  ldp   x20, x30, [sp], #0x20
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
