	.file	"e2e_arithmetic_16c4.cgen.c"
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
	push	ebx
	sub	esp, 20
	mov	DWORD PTR [ebp-16], 7
	mov	DWORD PTR [ebp-12], 0
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L5
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L5:
	mov	DWORD PTR [ebp-24], 3
	mov	DWORD PTR [ebp-20], 0
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L7
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L7:
	mov	eax, DWORD PTR [ebp-12]
	imul	eax, DWORD PTR [ebp-24]
	mov	edx, eax
	mov	eax, DWORD PTR [ebp-20]
	imul	eax, DWORD PTR [ebp-16]
	lea	ecx, [edx+eax]
	mov	eax, DWORD PTR [ebp-16]
	mul	DWORD PTR [ebp-24]
	add	ecx, edx
	mov	edx, ecx
	mov	ecx, DWORD PTR [ebp-16]
	mov	ebx, DWORD PTR [ebp-12]
	sub	ecx, DWORD PTR [ebp-24]
	sbb	ebx, DWORD PTR [ebp-20]
	add	eax, ecx
	adc	edx, ebx
.L6:
	mov	ebx, DWORD PTR [ebp-4]
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
	je	.L9
	call	cct_rt_fractum_uncaught_abort
	mov	eax, 1
	jmp	.L10
.L9:
	mov	eax, DWORD PTR [esp+8]
.L10:
	leave
	ret
	.size	main, .-main
	.ident	"GCC: (GNU) 15.2.1 20260209"
	.section	.note.GNU-stack,"",@progbits
