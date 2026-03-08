	.intel_syntax noprefix
	.file	"asm_validate_loop_16c2.cgen.c"
	.text
	.p2align	4                               # -- Begin function cct_fn_main
	.type	cct_fn_main,@function
cct_fn_main:                            # @cct_fn_main
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	esi
	and	esp, -8
	sub	esp, 32
	mov	dword ptr [esp + 12], 0
	mov	dword ptr [esp + 8], 0
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB0_2
# %bb.1:
	mov	dword ptr [esp + 24], 0
	mov	dword ptr [esp + 20], 0
	jmp	.LBB0_13
.LBB0_2:
	mov	dword ptr [esp + 4], 0
	mov	dword ptr [esp], 0
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB0_4
# %bb.3:
	mov	dword ptr [esp + 24], 0
	mov	dword ptr [esp + 20], 0
	jmp	.LBB0_13
.LBB0_4:
	jmp	.LBB0_5
.LBB0_5:                                # =>This Inner Loop Header: Depth=1
	mov	ecx, dword ptr [esp + 8]
	mov	eax, dword ptr [esp + 12]
	sub	ecx, 4
	sbb	eax, 0
	setl	al
	movzx	eax, al
	cmp	eax, 0
	je	.LBB0_11
# %bb.6:                                #   in Loop: Header=BB0_5 Depth=1
	mov	ecx, dword ptr [esp]
	mov	eax, dword ptr [esp + 4]
	mov	esi, dword ptr [esp + 8]
	mov	edx, dword ptr [esp + 12]
	add	ecx, esi
	adc	eax, edx
	mov	dword ptr [esp], ecx
	mov	dword ptr [esp + 4], eax
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB0_8
# %bb.7:
	mov	dword ptr [esp + 24], 0
	mov	dword ptr [esp + 20], 0
	jmp	.LBB0_13
.LBB0_8:                                #   in Loop: Header=BB0_5 Depth=1
	mov	eax, dword ptr [esp + 12]
	add	dword ptr [esp + 8], 1
	adc	eax, 0
	mov	dword ptr [esp + 12], eax
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB0_10
# %bb.9:
	mov	dword ptr [esp + 24], 0
	mov	dword ptr [esp + 20], 0
	jmp	.LBB0_13
.LBB0_10:                               #   in Loop: Header=BB0_5 Depth=1
	jmp	.LBB0_5
.LBB0_11:
	jmp	.LBB0_12
.LBB0_12:
	movsd	xmm0, qword ptr [esp]           # xmm0 = mem[0],zero
	movsd	qword ptr [esp + 20], xmm0
.LBB0_13:
	mov	eax, dword ptr [esp + 20]
	mov	edx, dword ptr [esp + 24]
	lea	esp, [ebp - 4]
	pop	esi
	pop	ebp
	ret
.Lfunc_end0:
	.size	cct_fn_main, .Lfunc_end0-cct_fn_main
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
.Lfunc_end1:
	.size	cct_rt_fractum_is_active, .Lfunc_end1-cct_rt_fractum_is_active
                                        # -- End function
	.globl	main                            # -- Begin function main
	.p2align	4
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
	je	.LBB2_2
# %bb.1:
	call	cct_rt_fractum_uncaught_abort
	mov	dword ptr [esp + 12], 1
	jmp	.LBB2_3
.LBB2_2:
	mov	eax, dword ptr [esp]
	mov	dword ptr [esp + 12], eax
.LBB2_3:
	mov	eax, dword ptr [esp + 12]
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end2:
	.size	main, .Lfunc_end2-main
                                        # -- End function
	.p2align	4                               # -- Begin function cct_rt_fractum_uncaught_abort
	.type	cct_rt_fractum_uncaught_abort,@function
cct_rt_fractum_uncaught_abort:          # @cct_rt_fractum_uncaught_abort
# %bb.0:
	push	ebp
	mov	ebp, esp
	call	cct_fs_panic
.Lfunc_end3:
	.size	cct_rt_fractum_uncaught_abort, .Lfunc_end3-cct_rt_fractum_uncaught_abort
                                        # -- End function
	.ident	"clang version 21.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym cct_fn_main
	.addrsig_sym cct_rt_fractum_is_active
	.addrsig_sym cct_rt_fractum_uncaught_abort
	.addrsig_sym cct_fs_panic
