	.file	"asm_validate_loop_16c2.cgen.c"
	.intel_syntax noprefix
	.text
	.type	cct_rt_fractum_is_active, @function
cct_rt_fractum_is_active:
	push	ebp
	mov	ebp, esp
	mov	eax, 0
	pop	ebp
	ret
	.size	cct_rt_fractum_is_active, .-cct_rt_fractum_is_active
	.type	cct_rt_fractum_uncaught_abort, @function
cct_rt_fractum_uncaught_abort:
	push	ebp
	mov	ebp, esp
	sub	esp, 8
	call	cct_fs_panic
	.size	cct_rt_fractum_uncaught_abort, .-cct_rt_fractum_uncaught_abort
	.type	cct_fn_main, @function
cct_fn_main:
	push	ebp
	mov	ebp, esp
	sub	esp, 16
	mov	DWORD PTR [ebp-8], 0
	mov	DWORD PTR [ebp-4], 0
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L5
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L5:
	mov	DWORD PTR [ebp-16], 0
	mov	DWORD PTR [ebp-12], 0
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L8
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L10:
	mov	eax, DWORD PTR [ebp-8]
	mov	edx, DWORD PTR [ebp-4]
	add	DWORD PTR [ebp-16], eax
	adc	DWORD PTR [ebp-12], edx
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L9
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L9:
	add	DWORD PTR [ebp-8], 1
	adc	DWORD PTR [ebp-4], 0
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L8
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L8:
	mov	edx, 3
	mov	eax, 0
	cmp	edx, DWORD PTR [ebp-8]
	sbb	eax, DWORD PTR [ebp-4]
	jge	.L10
	mov	eax, DWORD PTR [ebp-16]
	mov	edx, DWORD PTR [ebp-12]
.L6:
	leave
	ret
	.size	cct_fn_main, .-cct_fn_main
	.globl	main
	.type	main, @function
main:
	push	ebp
	mov	ebp, esp
	and	esp, -16
	sub	esp, 16
	call	cct_fn_main
	mov	DWORD PTR [esp+8], eax
	mov	DWORD PTR [esp+12], edx
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L12
	call	cct_rt_fractum_uncaught_abort
	mov	eax, 1
	jmp	.L13
.L12:
	mov	eax, DWORD PTR [esp+8]
.L13:
	leave
	ret
	.size	main, .-main
	.ident	"GCC: (GNU) 15.2.1 20260209"
	.section	.note.GNU-stack,"",@progbits
