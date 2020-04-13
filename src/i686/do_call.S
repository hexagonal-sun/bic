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

	push	ebp
	mov	  ebp, esp
  push  0
  mov   edx, [ebp + 0xc]
1:test  edx, edx
  je    call_fn
  push  [edx + 4]
  mov   edx, [edx]
  add   DWORD PTR [ebp - 4], 4
  jmp   1b

call_fn:
  call  [ebp + 8]
  add   esp, [ebp - 4]
  add   esp, 4
  pop   ebp
	ret
