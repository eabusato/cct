	.intel_syntax noprefix
	.file	"emit_asm_call_16c1.cgen.c"
	.text
	.p2align	4                               # -- Begin function cct_fn_main
	.type	cct_fn_main,@function
cct_fn_main:                            # @cct_fn_main
# %bb.0:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, esp
	mov	dword ptr [eax + 4], 0
	mov	dword ptr [eax], 4
	call	cct_fn_soma1
	mov	dword ptr [ebp - 12], edx
	mov	dword ptr [ebp - 16], eax
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB0_2
# %bb.1:
	mov	dword ptr [ebp - 4], 0
	mov	dword ptr [ebp - 8], 0
	jmp	.LBB0_3
.LBB0_2:
	movsd	xmm0, qword ptr [ebp - 16]      # xmm0 = mem[0],zero
	movsd	qword ptr [ebp - 8], xmm0
.LBB0_3:
	mov	eax, dword ptr [ebp - 8]
	mov	edx, dword ptr [ebp - 4]
	add	esp, 24
	pop	ebp
	ret
.Lfunc_end0:
	.size	cct_fn_main, .Lfunc_end0-cct_fn_main
                                        # -- End function
	.p2align	4                               # -- Begin function cct_fn_soma1
	.type	cct_fn_soma1,@function
cct_fn_soma1:                           # @cct_fn_soma1
# %bb.0:
	push	ebp
	mov	ebp, esp
	mov	eax, dword ptr [ebp + 8]
	mov	eax, dword ptr [ebp + 12]
	mov	eax, dword ptr [ebp + 8]
	mov	edx, dword ptr [ebp + 12]
	add	eax, 1
	adc	edx, 0
	pop	ebp
	ret
.Lfunc_end1:
	.size	cct_fn_soma1, .Lfunc_end1-cct_fn_soma1
                                        # -- End function
	.p2align	4                               # -- Begin function cct_rt_fractum_is_active
	.type	cct_rt_fractum_is_active,@function
cct_rt_fractum_is_active:               # @cct_rt_fractum_is_active
# %bb.0:
	push	ebp
	mov	ebp, esp
	xor	eax, eax
	pop	ebp
	ret
.Lfunc_end2:
	.size	cct_rt_fractum_is_active, .Lfunc_end2-cct_rt_fractum_is_active
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
	.type	main,@function
main:                                   # @main
# %bb.0:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, dword ptr [ebp + 12]
	mov	eax, dword ptr [ebp + 8]
	call	cct_fn_main
	mov	dword ptr [ebp - 12], edx
	mov	dword ptr [ebp - 16], eax
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB3_2
# %bb.1:
	call	cct_rt_fractum_uncaught_abort
	mov	dword ptr [ebp - 4], 1
	jmp	.LBB3_3
.LBB3_2:
	mov	eax, dword ptr [ebp - 16]
	mov	dword ptr [ebp - 4], eax
.LBB3_3:
	mov	eax, dword ptr [ebp - 4]
	add	esp, 24
	pop	ebp
	ret
.Lfunc_end3:
	.size	main, .Lfunc_end3-main
                                        # -- End function
	.p2align	4                               # -- Begin function cct_rt_fractum_uncaught_abort
	.type	cct_rt_fractum_uncaught_abort,@function
cct_rt_fractum_uncaught_abort:          # @cct_rt_fractum_uncaught_abort
# %bb.0:
	push	ebp
	mov	ebp, esp
	sub	esp, 8
	call	cct_fs_panic
.Lfunc_end4:
	.size	cct_rt_fractum_uncaught_abort, .Lfunc_end4-cct_rt_fractum_uncaught_abort
                                        # -- End function
	.ident	"clang version 21.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym cct_fn_main
	.addrsig_sym cct_fn_soma1
	.addrsig_sym cct_rt_fractum_is_active
	.addrsig_sym cct_rt_fractum_uncaught_abort
	.addrsig_sym cct_fs_panic
