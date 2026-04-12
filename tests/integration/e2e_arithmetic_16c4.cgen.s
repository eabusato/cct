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
	push	esi
	push	ebx
	sub	esp, 16
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
	lea	esi, [edx+eax]
	mov	ebx, DWORD PTR [ebp-24]
	mov	eax, ebx
	mul	DWORD PTR [ebp-16]
	mov	ecx, eax
	mov	ebx, edx
	lea	eax, [esi+ebx]
	mov	ebx, eax
	mov	eax, DWORD PTR [ebp-16]
	mov	edx, DWORD PTR [ebp-12]
	sub	eax, DWORD PTR [ebp-24]
	sbb	edx, DWORD PTR [ebp-20]
	add	eax, ecx
	adc	edx, ebx
.L6:
	add	esp, 16
	pop	ebx
	pop	esi
	pop	ebp
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
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
