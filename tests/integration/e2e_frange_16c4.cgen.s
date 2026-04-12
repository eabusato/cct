	.file	"e2e_frange_16c4.cgen.c"
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
	push	edi
	push	esi
	sub	esp, 16
	mov	DWORD PTR [ebp-16], 0
	mov	DWORD PTR [ebp-12], 0
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L5
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L5:
	mov	DWORD PTR [ebp-24], 0
	mov	DWORD PTR [ebp-20], 0
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L8
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L12:
	mov	eax, DWORD PTR [ebp-16]
	xor	eax, 4
	mov	esi, eax
	mov	eax, DWORD PTR [ebp-12]
	xor	ah, 0
	mov	edi, eax
	mov	eax, edi
	or	eax, esi
	test	eax, eax
	je	.L13
	mov	eax, DWORD PTR [ebp-16]
	mov	edx, DWORD PTR [ebp-12]
	add	DWORD PTR [ebp-24], eax
	adc	DWORD PTR [ebp-20], edx
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L11
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L11:
	add	DWORD PTR [ebp-16], 1
	adc	DWORD PTR [ebp-12], 0
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L8
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L8:
	mov	edx, 9
	mov	eax, 0
	cmp	edx, DWORD PTR [ebp-16]
	sbb	eax, DWORD PTR [ebp-12]
	jge	.L12
	jmp	.L10
.L13:
	nop
.L10:
	mov	eax, DWORD PTR [ebp-24]
	mov	edx, DWORD PTR [ebp-20]
.L6:
	add	esp, 16
	pop	esi
	pop	edi
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
	je	.L15
	call	cct_rt_fractum_uncaught_abort
	mov	eax, 1
	jmp	.L16
.L15:
	mov	eax, DWORD PTR [esp+8]
.L16:
	leave
	ret
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
