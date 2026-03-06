	.file	"asm_validate_call_16c2.cgen.c"
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
	sub	esp, 24
	push	0
	push	4
	push	0
	push	3
	call	cct_fn_soma
	add	esp, 16
	mov	DWORD PTR [ebp-16], eax
	mov	DWORD PTR [ebp-12], edx
	call	cct_rt_fractum_is_active
	test	eax, eax
	je	.L5
	mov	eax, 0
	mov	edx, 0
	jmp	.L6
.L5:
	mov	eax, DWORD PTR [ebp-16]
	mov	edx, DWORD PTR [ebp-12]
.L6:
	leave
	ret
	.size	cct_fn_main, .-cct_fn_main
	.type	cct_fn_soma, @function
cct_fn_soma:
	push	ebp
	mov	ebp, esp
	push	ebx
	sub	esp, 20
	mov	eax, DWORD PTR [ebp+8]
	mov	edx, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-16], eax
	mov	DWORD PTR [ebp-12], edx
	mov	eax, DWORD PTR [ebp+16]
	mov	edx, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-24], eax
	mov	DWORD PTR [ebp-20], edx
	mov	ecx, DWORD PTR [ebp-16]
	mov	ebx, DWORD PTR [ebp-12]
	mov	eax, DWORD PTR [ebp-24]
	mov	edx, DWORD PTR [ebp-20]
	add	eax, ecx
	adc	edx, ebx
	mov	ebx, DWORD PTR [ebp-4]
	leave
	ret
	.size	cct_fn_soma, .-cct_fn_soma
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
	je	.L10
	call	cct_rt_fractum_uncaught_abort
	mov	eax, 1
	jmp	.L11
.L10:
	mov	eax, DWORD PTR [esp+8]
.L11:
	leave
	ret
	.size	main, .-main
	.ident	"GCC: (GNU) 15.2.1 20260209"
	.section	.note.GNU-stack,"",@progbits
