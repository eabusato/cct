	.intel_syntax noprefix
	.text
	.globl cct_fn_bad_libc
cct_fn_bad_libc:
	push ebp
	mov ebp, esp
	call printf
	leave
	ret
