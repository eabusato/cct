	.text
	.intel_syntax noprefix
	.file	"e2e_minimal_16c4.cgen.c"
	.p2align	4, 0x90                         # -- Begin function cct_fn_main
	.type	cct_fn_main,@function
cct_fn_main:                            # @cct_fn_main
# %bb.0:
	push	ebp
	mov	ebp, esp
	xor	edx, edx
	mov	eax, edx
	pop	ebp
	ret
.Lfunc_end0:
	.size	cct_fn_main, .Lfunc_end0-cct_fn_main
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
	je	.LBB1_2
# %bb.1:
	call	cct_rt_fractum_uncaught_abort
	mov	dword ptr [esp + 12], 1
	jmp	.LBB1_3
.LBB1_2:
	mov	eax, dword ptr [esp]
	mov	dword ptr [esp + 12], eax
.LBB1_3:
	mov	eax, dword ptr [esp + 12]
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end1:
	.size	main, .Lfunc_end1-main
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
	.p2align	4, 0x90                         # -- Begin function cct_rt_fractum_uncaught_abort
	.type	cct_rt_fractum_uncaught_abort,@function
cct_rt_fractum_uncaught_abort:          # @cct_rt_fractum_uncaught_abort
# %bb.0:
	push	ebp
	mov	ebp, esp
	call	cct_fs_panic
.Lfunc_end3:
	.size	cct_rt_fractum_uncaught_abort, .Lfunc_end3-cct_rt_fractum_uncaught_abort
                                        # -- End function
	.ident	"Apple clang version 17.0.0 (clang-1700.6.4.2)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym cct_fn_main
	.addrsig_sym cct_rt_fractum_is_active
	.addrsig_sym cct_rt_fractum_uncaught_abort
	.addrsig_sym cct_fs_panic
