	.file	"kernel.cgen.c"
	.intel_syntax noprefix
	.text
	.type	cct_svc_halt, @function
cct_svc_halt:
	push	ebp
	mov	ebp, esp
#APP
# 42 "/home/admin/dev/cct/src/runtime/cct_freestanding_rt.h" 1
	cli
	hlt
# 0 "" 2
#NO_APP
	.size	cct_svc_halt, .-cct_svc_halt
	.type	cct_svc_outb, @function
cct_svc_outb:
	push	ebp
	mov	ebp, esp
	sub	esp, 32
	mov	eax, DWORD PTR [ebp+8]
	mov	DWORD PTR [ebp-24], eax
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-20], eax
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-32], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-28], eax
	mov	eax, DWORD PTR [ebp-24]
	mov	WORD PTR [ebp-2], ax
	mov	eax, DWORD PTR [ebp-32]
	mov	BYTE PTR [ebp-3], al
	movzx	eax, BYTE PTR [ebp-3]
	movzx	edx, WORD PTR [ebp-2]
#APP
# 53 "/home/admin/dev/cct/src/runtime/cct_freestanding_rt.h" 1
	out dx, al
# 0 "" 2
# 54 "/home/admin/dev/cct/src/runtime/cct_freestanding_rt.h" 1
	clc
# 0 "" 2
#NO_APP
	nop
	leave
	ret
	.size	cct_svc_outb, .-cct_svc_outb
	.type	cct_svc_inb, @function
cct_svc_inb:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, DWORD PTR [ebp+8]
	mov	DWORD PTR [ebp-24], eax
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-20], eax
	mov	BYTE PTR [ebp-1], 0
	mov	eax, DWORD PTR [ebp-24]
	mov	WORD PTR [ebp-4], ax
	movzx	eax, WORD PTR [ebp-4]
	mov	edx, eax
#APP
# 65 "/home/admin/dev/cct/src/runtime/cct_freestanding_rt.h" 1
	in al, dx
# 0 "" 2
#NO_APP
	mov	BYTE PTR [ebp-1], al
#APP
# 66 "/home/admin/dev/cct/src/runtime/cct_freestanding_rt.h" 1
	clc
# 0 "" 2
#NO_APP
	movzx	eax, BYTE PTR [ebp-1]
	mov	edx, 0
	leave
	ret
	.size	cct_svc_inb, .-cct_svc_inb
	.type	cct_svc_memcpy, @function
cct_svc_memcpy:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-16], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-12], eax
	cmp	DWORD PTR [ebp+8], 0
	je	.L6
	cmp	DWORD PTR [ebp+12], 0
	je	.L6
	cmp	DWORD PTR [ebp-12], 0
	jns	.L7
.L6:
	call	cct_svc_halt
.L7:
	mov	eax, DWORD PTR [ebp-16]
	sub	esp, 4
	push	eax
	push	DWORD PTR [ebp+12]
	push	DWORD PTR [ebp+8]
	call	cct_fs_memcpy
	add	esp, 16
#APP
# 125 "/home/admin/dev/cct/src/runtime/cct_freestanding_rt.h" 1
	clc
# 0 "" 2
#NO_APP
	nop
	leave
	ret
	.size	cct_svc_memcpy, .-cct_svc_memcpy
	.type	cct_svc_memset, @function
cct_svc_memset:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-16], eax
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-12], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-24], eax
	mov	eax, DWORD PTR [ebp+24]
	mov	DWORD PTR [ebp-20], eax
	cmp	DWORD PTR [ebp+8], 0
	je	.L9
	cmp	DWORD PTR [ebp-20], 0
	jns	.L10
.L9:
	call	cct_svc_halt
.L10:
	mov	edx, DWORD PTR [ebp-24]
	mov	eax, DWORD PTR [ebp-16]
	movzx	eax, al
	sub	esp, 4
	push	edx
	push	eax
	push	DWORD PTR [ebp+8]
	call	cct_fs_memset
	add	esp, 16
#APP
# 133 "/home/admin/dev/cct/src/runtime/cct_freestanding_rt.h" 1
	clc
# 0 "" 2
#NO_APP
	nop
	leave
	ret
	.size	cct_svc_memset, .-cct_svc_memset
	.type	cct_fn_kernel_memset, @function
cct_fn_kernel_memset:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-16], eax
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-12], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-24], eax
	mov	eax, DWORD PTR [ebp+24]
	mov	DWORD PTR [ebp-20], eax
	sub	esp, 12
	push	DWORD PTR [ebp-20]
	push	DWORD PTR [ebp-24]
	push	DWORD PTR [ebp-12]
	push	DWORD PTR [ebp-16]
	push	DWORD PTR [ebp+8]
	call	cct_svc_memset
	add	esp, 32
	mov	eax, 0
	mov	edx, 0
	leave
	ret
	.size	cct_fn_kernel_memset, .-cct_fn_kernel_memset
	.type	cct_fn_kernel_memcpy, @function
cct_fn_kernel_memcpy:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-16], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-12], eax
	push	DWORD PTR [ebp-12]
	push	DWORD PTR [ebp-16]
	push	DWORD PTR [ebp+12]
	push	DWORD PTR [ebp+8]
	call	cct_svc_memcpy
	add	esp, 16
	mov	eax, 0
	mov	edx, 0
	leave
	ret
	.size	cct_fn_kernel_memcpy, .-cct_fn_kernel_memcpy
	.type	cct_fn_kernel_inb, @function
cct_fn_kernel_inb:
	push	ebp
	mov	ebp, esp
	sub	esp, 8
	mov	eax, DWORD PTR [ebp+8]
	mov	DWORD PTR [ebp-8], eax
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-4], eax
	push	DWORD PTR [ebp-4]
	push	DWORD PTR [ebp-8]
	call	cct_svc_inb
	add	esp, 8
	leave
	ret
	.size	cct_fn_kernel_inb, .-cct_fn_kernel_inb
	.type	cct_fn_kernel_outb, @function
cct_fn_kernel_outb:
	push	ebp
	mov	ebp, esp
	sub	esp, 16
	mov	eax, DWORD PTR [ebp+8]
	mov	DWORD PTR [ebp-8], eax
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-4], eax
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-16], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-12], eax
	push	DWORD PTR [ebp-12]
	push	DWORD PTR [ebp-16]
	push	DWORD PTR [ebp-4]
	push	DWORD PTR [ebp-8]
	call	cct_svc_outb
	add	esp, 16
	mov	eax, 0
	mov	edx, 0
	leave
	ret
	.size	cct_fn_kernel_outb, .-cct_fn_kernel_outb
	.globl	cct_fn_kernel_kernel_halt
	.type	cct_fn_kernel_kernel_halt, @function
cct_fn_kernel_kernel_halt:
	push	ebp
	mov	ebp, esp
	call	cct_svc_halt
	.size	cct_fn_kernel_kernel_halt, .-cct_fn_kernel_kernel_halt
	.globl	kernel_memset
	.type	kernel_memset, @function
kernel_memset:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-16], eax
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-12], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-24], eax
	mov	eax, DWORD PTR [ebp+24]
	mov	DWORD PTR [ebp-20], eax
	sub	esp, 12
	push	DWORD PTR [ebp-20]
	push	DWORD PTR [ebp-24]
	push	DWORD PTR [ebp-12]
	push	DWORD PTR [ebp-16]
	push	DWORD PTR [ebp+8]
	call	cct_fn_kernel_memset
	add	esp, 32
	nop
	leave
	ret
	.size	kernel_memset, .-kernel_memset
	.globl	kernel_memcpy
	.type	kernel_memcpy, @function
kernel_memcpy:
	push	ebp
	mov	ebp, esp
	sub	esp, 24
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-16], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-12], eax
	push	DWORD PTR [ebp-12]
	push	DWORD PTR [ebp-16]
	push	DWORD PTR [ebp+12]
	push	DWORD PTR [ebp+8]
	call	cct_fn_kernel_memcpy
	add	esp, 16
	nop
	leave
	ret
	.size	kernel_memcpy, .-kernel_memcpy
	.globl	kernel_inb
	.type	kernel_inb, @function
kernel_inb:
	push	ebp
	mov	ebp, esp
	sub	esp, 8
	mov	eax, DWORD PTR [ebp+8]
	mov	DWORD PTR [ebp-8], eax
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-4], eax
	push	DWORD PTR [ebp-4]
	push	DWORD PTR [ebp-8]
	call	cct_fn_kernel_inb
	add	esp, 8
	leave
	ret
	.size	kernel_inb, .-kernel_inb
	.globl	kernel_outb
	.type	kernel_outb, @function
kernel_outb:
	push	ebp
	mov	ebp, esp
	sub	esp, 16
	mov	eax, DWORD PTR [ebp+8]
	mov	DWORD PTR [ebp-8], eax
	mov	eax, DWORD PTR [ebp+12]
	mov	DWORD PTR [ebp-4], eax
	mov	eax, DWORD PTR [ebp+16]
	mov	DWORD PTR [ebp-16], eax
	mov	eax, DWORD PTR [ebp+20]
	mov	DWORD PTR [ebp-12], eax
	push	DWORD PTR [ebp-12]
	push	DWORD PTR [ebp-16]
	push	DWORD PTR [ebp-4]
	push	DWORD PTR [ebp-8]
	call	cct_fn_kernel_outb
	add	esp, 16
	nop
	leave
	ret
	.size	kernel_outb, .-kernel_outb
	.globl	kernel_halt
	.type	kernel_halt, @function
kernel_halt:
	push	ebp
	mov	ebp, esp
	call	cct_fn_kernel_kernel_halt
	nop
	pop	ebp
	ret
	.size	kernel_halt, .-kernel_halt
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
