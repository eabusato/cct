	.text
	.intel_syntax noprefix
	.file	"asm_validate_call_16c2.cgen.c"
	.p2align	4, 0x90                         # -- Begin function cct_fn_main
	.type	cct_fn_main,@function
cct_fn_main:                            # @cct_fn_main
# %bb.0:
	push	ebp
	mov	ebp, esp
	and	esp, -8
	sub	esp, 32
	mov	eax, esp
	mov	dword ptr [eax + 12], 0
	mov	dword ptr [eax + 8], 4
	mov	dword ptr [eax + 4], 0
	mov	dword ptr [eax], 3
	call	cct_fn_soma
	mov	dword ptr [esp + 20], edx
	mov	dword ptr [esp + 16], eax
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB0_2
# %bb.1:
	mov	dword ptr [esp + 28], 0
	mov	dword ptr [esp + 24], 0
	jmp	.LBB0_3
.LBB0_2:
	movsd	xmm0, qword ptr [esp + 16]      # xmm0 = mem[0],zero
	movsd	qword ptr [esp + 24], xmm0
.LBB0_3:
	mov	eax, dword ptr [esp + 24]
	mov	edx, dword ptr [esp + 28]
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end0:
	.size	cct_fn_main, .Lfunc_end0-cct_fn_main
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_fn_soma
	.type	cct_fn_soma,@function
cct_fn_soma:                            # @cct_fn_soma
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	esi
	and	esp, -8
	sub	esp, 24
	mov	eax, dword ptr [ebp + 16]
	mov	ecx, dword ptr [ebp + 20]
	mov	esi, dword ptr [ebp + 8]
	mov	edx, dword ptr [ebp + 12]
	mov	dword ptr [esp + 8], esi
	mov	dword ptr [esp + 12], edx
	mov	dword ptr [esp + 4], ecx
	mov	dword ptr [esp], eax
	mov	eax, dword ptr [esp + 8]
	mov	edx, dword ptr [esp + 12]
	mov	esi, dword ptr [esp]
	mov	ecx, dword ptr [esp + 4]
	add	eax, esi
	adc	edx, ecx
	lea	esp, [ebp - 4]
	pop	esi
	pop	ebp
	ret
.Lfunc_end1:
	.size	cct_fn_soma, .Lfunc_end1-cct_fn_soma
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_rt_fractum_is_active
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
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
# %bb.0:
	push	ebp
	mov	ebp, esp
	and	esp, -8
	sub	esp, 16
	mov	eax, dword ptr [ebp + 12]
	mov	eax, dword ptr [ebp + 8]
	call	cct_fn_main
	mov	dword ptr [esp + 4], edx
	mov	dword ptr [esp], eax
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB3_2
# %bb.1:
	call	cct_rt_fractum_uncaught_abort
	mov	dword ptr [esp + 12], 1
	jmp	.LBB3_3
.LBB3_2:
	mov	eax, dword ptr [esp]
	mov	dword ptr [esp + 12], eax
.LBB3_3:
	mov	eax, dword ptr [esp + 12]
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end3:
	.size	main, .Lfunc_end3-main
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_rt_fractum_uncaught_abort
	.type	cct_rt_fractum_uncaught_abort,@function
cct_rt_fractum_uncaught_abort:          # @cct_rt_fractum_uncaught_abort
# %bb.0:
	push	ebp
	mov	ebp, esp
	call	cct_fs_panic
.Lfunc_end4:
	.size	cct_rt_fractum_uncaught_abort, .Lfunc_end4-cct_rt_fractum_uncaught_abort
                                        # -- End function
	.ident	"Apple clang version 17.0.0 (clang-1700.6.4.2)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym cct_fn_main
	.addrsig_sym cct_fn_soma
	.addrsig_sym cct_rt_fractum_is_active
	.addrsig_sym cct_rt_fractum_uncaught_abort
	.addrsig_sym cct_fs_panic
