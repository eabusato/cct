	.text
	.intel_syntax noprefix
	.file	"kernel.cgen.c"
	.p2align	4, 0x90                         # -- Begin function cct_fn_kernel_memset
	.type	cct_fn_kernel_memset,@function
cct_fn_kernel_memset:                   # @cct_fn_kernel_memset
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	ebx
	push	edi
	push	esi
	and	esp, -8
	sub	esp, 48
	mov	eax, dword ptr [ebp + 20]
	mov	ecx, dword ptr [ebp + 24]
	mov	esi, dword ptr [ebp + 12]
	mov	edx, dword ptr [ebp + 16]
	mov	edi, dword ptr [ebp + 8]
	mov	dword ptr [esp + 32], esi
	mov	dword ptr [esp + 36], edx
	mov	dword ptr [esp + 28], ecx
	mov	dword ptr [esp + 24], eax
	mov	ecx, dword ptr [ebp + 8]
	mov	edx, dword ptr [esp + 32]
	mov	esi, dword ptr [esp + 36]
	mov	edi, dword ptr [esp + 24]
	mov	ebx, dword ptr [esp + 28]
	mov	eax, esp
	mov	dword ptr [eax + 16], ebx
	mov	dword ptr [eax + 12], edi
	mov	dword ptr [eax + 8], esi
	mov	dword ptr [eax + 4], edx
	mov	dword ptr [eax], ecx
	call	cct_svc_memset
	xor	edx, edx
	mov	eax, edx
	lea	esp, [ebp - 12]
	pop	esi
	pop	edi
	pop	ebx
	pop	ebp
	ret
.Lfunc_end0:
	.size	cct_fn_kernel_memset, .Lfunc_end0-cct_fn_kernel_memset
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_svc_memset
	.type	cct_svc_memset,@function
cct_svc_memset:                         # @cct_svc_memset
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	edi
	push	esi
	and	esp, -8
	sub	esp, 32
	mov	eax, dword ptr [ebp + 20]
	mov	ecx, dword ptr [ebp + 24]
	mov	esi, dword ptr [ebp + 12]
	mov	edx, dword ptr [ebp + 16]
	mov	edi, dword ptr [ebp + 8]
	mov	dword ptr [esp + 24], esi
	mov	dword ptr [esp + 28], edx
	mov	dword ptr [esp + 20], ecx
	mov	dword ptr [esp + 16], eax
	cmp	dword ptr [ebp + 8], 0
	je	.LBB1_2
# %bb.1:
	mov	eax, dword ptr [esp + 20]
	test	eax, eax
	jns	.LBB1_3
	jmp	.LBB1_2
.LBB1_2:
	call	cct_svc_halt
.LBB1_3:
	mov	edx, dword ptr [ebp + 8]
	movzx	ecx, byte ptr [esp + 24]
	mov	eax, dword ptr [esp + 16]
	mov	dword ptr [esp], edx
	mov	dword ptr [esp + 4], ecx
	mov	dword ptr [esp + 8], eax
	call	cct_fs_memset
	#APP

	clc

	#NO_APP
	lea	esp, [ebp - 8]
	pop	esi
	pop	edi
	pop	ebp
	ret
.Lfunc_end1:
	.size	cct_svc_memset, .Lfunc_end1-cct_svc_memset
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_fn_kernel_memcpy
	.type	cct_fn_kernel_memcpy,@function
cct_fn_kernel_memcpy:                   # @cct_fn_kernel_memcpy
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	edi
	push	esi
	and	esp, -8
	sub	esp, 24
	mov	ecx, dword ptr [ebp + 16]
	mov	eax, dword ptr [ebp + 20]
	mov	edx, dword ptr [ebp + 12]
	mov	edx, dword ptr [ebp + 8]
	mov	dword ptr [esp + 16], ecx
	mov	dword ptr [esp + 20], eax
	mov	ecx, dword ptr [ebp + 8]
	mov	edx, dword ptr [ebp + 12]
	mov	esi, dword ptr [esp + 16]
	mov	edi, dword ptr [esp + 20]
	mov	eax, esp
	mov	dword ptr [eax + 12], edi
	mov	dword ptr [eax + 8], esi
	mov	dword ptr [eax + 4], edx
	mov	dword ptr [eax], ecx
	call	cct_svc_memcpy
	xor	edx, edx
	mov	eax, edx
	lea	esp, [ebp - 8]
	pop	esi
	pop	edi
	pop	ebp
	ret
.Lfunc_end2:
	.size	cct_fn_kernel_memcpy, .Lfunc_end2-cct_fn_kernel_memcpy
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_svc_memcpy
	.type	cct_svc_memcpy,@function
cct_svc_memcpy:                         # @cct_svc_memcpy
# %bb.0:
	push	ebp
	mov	ebp, esp
	and	esp, -8
	sub	esp, 24
	mov	ecx, dword ptr [ebp + 16]
	mov	eax, dword ptr [ebp + 20]
	mov	edx, dword ptr [ebp + 12]
	mov	edx, dword ptr [ebp + 8]
	mov	dword ptr [esp + 16], ecx
	mov	dword ptr [esp + 20], eax
	cmp	dword ptr [ebp + 8], 0
	je	.LBB3_3
# %bb.1:
	cmp	dword ptr [ebp + 12], 0
	je	.LBB3_3
# %bb.2:
	mov	eax, dword ptr [esp + 20]
	test	eax, eax
	jns	.LBB3_4
	jmp	.LBB3_3
.LBB3_3:
	call	cct_svc_halt
.LBB3_4:
	mov	edx, dword ptr [ebp + 8]
	mov	ecx, dword ptr [ebp + 12]
	mov	eax, dword ptr [esp + 16]
	mov	dword ptr [esp], edx
	mov	dword ptr [esp + 4], ecx
	mov	dword ptr [esp + 8], eax
	call	cct_fs_memcpy
	#APP

	clc

	#NO_APP
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end3:
	.size	cct_svc_memcpy, .Lfunc_end3-cct_svc_memcpy
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_fn_kernel_inb
	.type	cct_fn_kernel_inb,@function
cct_fn_kernel_inb:                      # @cct_fn_kernel_inb
# %bb.0:
	push	ebp
	mov	ebp, esp
	and	esp, -8
	sub	esp, 16
	mov	ecx, dword ptr [ebp + 8]
	mov	eax, dword ptr [ebp + 12]
	mov	dword ptr [esp + 8], ecx
	mov	dword ptr [esp + 12], eax
	mov	ecx, dword ptr [esp + 8]
	mov	edx, dword ptr [esp + 12]
	mov	eax, esp
	mov	dword ptr [eax + 4], edx
	mov	dword ptr [eax], ecx
	call	cct_svc_inb
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end4:
	.size	cct_fn_kernel_inb, .Lfunc_end4-cct_fn_kernel_inb
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_svc_inb
	.type	cct_svc_inb,@function
cct_svc_inb:                            # @cct_svc_inb
# %bb.0:
	push	ebp
	mov	ebp, esp
	and	esp, -8
	sub	esp, 16
	mov	ecx, dword ptr [ebp + 8]
	mov	eax, dword ptr [ebp + 12]
	mov	dword ptr [esp + 8], ecx
	mov	dword ptr [esp + 12], eax
	mov	byte ptr [esp + 7], 0
	mov	ax, word ptr [esp + 8]
	mov	word ptr [esp + 4], ax
	mov	dx, word ptr [esp + 4]
	#APP

	in	al, dx

	#NO_APP
	mov	byte ptr [esp + 7], al
	#APP

	clc

	#NO_APP
	movzx	eax, byte ptr [esp + 7]
	xor	edx, edx
	mov	esp, ebp
	pop	ebp
	ret
.Lfunc_end5:
	.size	cct_svc_inb, .Lfunc_end5-cct_svc_inb
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_fn_kernel_outb
	.type	cct_fn_kernel_outb,@function
cct_fn_kernel_outb:                     # @cct_fn_kernel_outb
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	edi
	push	esi
	and	esp, -8
	sub	esp, 32
	mov	eax, dword ptr [ebp + 16]
	mov	ecx, dword ptr [ebp + 20]
	mov	esi, dword ptr [ebp + 8]
	mov	edx, dword ptr [ebp + 12]
	mov	dword ptr [esp + 24], esi
	mov	dword ptr [esp + 28], edx
	mov	dword ptr [esp + 20], ecx
	mov	dword ptr [esp + 16], eax
	mov	ecx, dword ptr [esp + 24]
	mov	edx, dword ptr [esp + 28]
	mov	esi, dword ptr [esp + 16]
	mov	edi, dword ptr [esp + 20]
	mov	eax, esp
	mov	dword ptr [eax + 12], edi
	mov	dword ptr [eax + 8], esi
	mov	dword ptr [eax + 4], edx
	mov	dword ptr [eax], ecx
	call	cct_svc_outb
	xor	edx, edx
	mov	eax, edx
	lea	esp, [ebp - 8]
	pop	esi
	pop	edi
	pop	ebp
	ret
.Lfunc_end6:
	.size	cct_fn_kernel_outb, .Lfunc_end6-cct_fn_kernel_outb
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_svc_outb
	.type	cct_svc_outb,@function
cct_svc_outb:                           # @cct_svc_outb
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	esi
	and	esp, -8
	sub	esp, 32
	mov	eax, dword ptr [ebp + 16]
	mov	ecx, dword ptr [ebp + 20]
	mov	esi, dword ptr [ebp + 8]
	mov	edx, dword ptr [ebp + 12]
	mov	dword ptr [esp + 16], esi
	mov	dword ptr [esp + 20], edx
	mov	dword ptr [esp + 12], ecx
	mov	dword ptr [esp + 8], eax
	mov	ax, word ptr [esp + 16]
	mov	word ptr [esp + 6], ax
	mov	al, byte ptr [esp + 8]
	mov	byte ptr [esp + 5], al
	mov	al, byte ptr [esp + 5]
	mov	dx, word ptr [esp + 6]
	#APP

	out	dx, al

	#NO_APP
	#APP

	clc

	#NO_APP
	lea	esp, [ebp - 4]
	pop	esi
	pop	ebp
	ret
.Lfunc_end7:
	.size	cct_svc_outb, .Lfunc_end7-cct_svc_outb
                                        # -- End function
	.globl	cct_fn_kernel_kernel_halt       # -- Begin function cct_fn_kernel_kernel_halt
	.p2align	4, 0x90
	.type	cct_fn_kernel_kernel_halt,@function
cct_fn_kernel_kernel_halt:              # @cct_fn_kernel_kernel_halt
# %bb.0:
	push	ebp
	mov	ebp, esp
	call	cct_svc_halt
.Lfunc_end8:
	.size	cct_fn_kernel_kernel_halt, .Lfunc_end8-cct_fn_kernel_kernel_halt
                                        # -- End function
	.p2align	4, 0x90                         # -- Begin function cct_svc_halt
	.type	cct_svc_halt,@function
cct_svc_halt:                           # @cct_svc_halt
# %bb.0:
	push	ebp
	mov	ebp, esp
	#APP

	cli
	hlt

	#NO_APP
.Lfunc_end9:
	.size	cct_svc_halt, .Lfunc_end9-cct_svc_halt
                                        # -- End function
	.ident	"Apple clang version 17.0.0 (clang-1700.6.4.2)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym cct_fn_kernel_memset
	.addrsig_sym cct_svc_memset
	.addrsig_sym cct_fn_kernel_memcpy
	.addrsig_sym cct_svc_memcpy
	.addrsig_sym cct_fn_kernel_inb
	.addrsig_sym cct_svc_inb
	.addrsig_sym cct_fn_kernel_outb
	.addrsig_sym cct_svc_outb
	.addrsig_sym cct_svc_halt
	.addrsig_sym cct_fs_memset
	.addrsig_sym cct_fs_memcpy
