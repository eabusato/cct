	.intel_syntax noprefix
	.file	"asm_gas_assemble_16c3.cgen.c"
	.text
	.p2align	4                               # -- Begin function cct_fn_main
	.type	cct_fn_main,@function
cct_fn_main:                            # @cct_fn_main
# %bb.0:
	push	ebp
	mov	ebp, esp
	and	esp, -8
	sub	esp, 24
	mov	eax, esp
	mov	dword ptr [eax + 4], 0
	mov	dword ptr [eax], 8
	call	cct_fn_dobra
	mov	dword ptr [esp + 12], edx
	mov	dword ptr [esp + 8], eax
	call	cct_rt_fractum_is_active
	cmp	eax, 0
	je	.LBB0_2
# %bb.1:
	mov	dword ptr [esp + 20], 0
	mov	dword ptr [esp + 16], 0
	jmp	.LBB0_3
.LBB0_2:
	movsd	xmm0, qword ptr [esp + 8]       # xmm0 = mem[0],zero
	movsd	qword ptr [esp + 16], xmm0
.LBB0_3:
	mov	eax, dword ptr [esp + 16]
	mov	edx, dword ptr [esp + 20]
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end0:
	.size	cct_fn_main, .Lfunc_end0-cct_fn_main
                                        # -- End function
	.p2align	4                               # -- Begin function cct_fn_dobra
	.type	cct_fn_dobra,@function
cct_fn_dobra:                           # @cct_fn_dobra
# %bb.0:
	push	ebp
	mov	ebp, esp
	and	esp, -8
	sub	esp, 8
	mov	ecx, dword ptr [ebp + 8]
	mov	eax, dword ptr [ebp + 12]
	mov	dword ptr [esp], ecx
	mov	dword ptr [esp + 4], eax
	mov	eax, dword ptr [esp]
	mov	edx, dword ptr [esp + 4]
	add	eax, eax
	adc	edx, edx
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end1:
	.size	cct_fn_dobra, .Lfunc_end1-cct_fn_dobra
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
	.p2align	4                               # -- Begin function cct_rt_fractum_uncaught_abort
	.type	cct_rt_fractum_uncaught_abort,@function
cct_rt_fractum_uncaught_abort:          # @cct_rt_fractum_uncaught_abort
# %bb.0:
	push	ebp
	mov	ebp, esp
	call	cct_fs_panic
.Lfunc_end4:
	.size	cct_rt_fractum_uncaught_abort, .Lfunc_end4-cct_rt_fractum_uncaught_abort
                                        # -- End function
	.ident	"clang version 21.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym cct_fn_main
	.addrsig_sym cct_fn_dobra
	.addrsig_sym cct_rt_fractum_is_active
	.addrsig_sym cct_rt_fractum_uncaught_abort
	.addrsig_sym cct_fs_panic
