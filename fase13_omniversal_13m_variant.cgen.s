	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 26, 0	sdk_version 26, 2
	.syntax unified
	.globl	_main                           @ -- Begin function main
	.p2align	2
	.code	32                              @ @main
_main:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #16
	bic	sp, sp, #7
	mov	r0, #0
	str	r0, [sp, #12]
	bl	_cct_fn_main
	str	r1, [sp, #4]
	str	r0, [sp]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB0_2
	b	LBB0_1
LBB0_1:
	bl	_cct_rt_fractum_uncaught_abort
	mov	r0, #1
	str	r0, [sp, #12]
	b	LBB0_3
LBB0_2:
	ldr	r0, [sp]
	str	r0, [sp, #12]
	b	LBB0_3
LBB0_3:
	ldr	r0, [sp, #12]
	mov	sp, r7
	pop	{r7, lr}
	bx	lr
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_fractum_is_active
	.code	32                              @ @cct_rt_fractum_is_active
_cct_rt_fractum_is_active:
@ %bb.0:
	ldr	r0, LCPI1_0
LPC1_0:
	ldr	r0, [pc, r0]
	bx	lr
	.p2align	2
@ %bb.1:
	.data_region
LCPI1_0:
	.long	_cct_rt_fractum_active-(LPC1_0+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_fractum_uncaught_abort
	.code	32                              @ @cct_rt_fractum_uncaught_abort
_cct_rt_fractum_uncaught_abort:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #4
	ldr	r0, LCPI2_0
LPC2_0:
	ldr	r0, [pc, r0]
	ldr	r0, [r0]
	str	r0, [sp]                        @ 4-byte Spill
	bl	_cct_rt_fractum_peek
	mov	r2, r0
	ldr	r0, [sp]                        @ 4-byte Reload
	ldr	r1, LCPI2_1
LPC2_1:
	add	r1, pc, r1
	bl	_fprintf
	bl	_cct_rt_fractum_clear
	mov	r0, #1
	bl	_exit
	.p2align	2
@ %bb.1:
	.data_region
LCPI2_0:
	.long	L___stderrp$non_lazy_ptr-(LPC2_0+8)
LCPI2_1:
	.long	L_.str.9-(LPC2_1+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_fn_main
	.code	32                              @ @cct_fn_main
_cct_fn_main:
@ %bb.0:
	push	{r4, r5, r7, lr}
	add	r7, sp, #8
	sub	sp, sp, #288
	bic	sp, sp, #7
	ldr	r0, LCPI3_0
LPC3_0:
	add	r0, pc, r0
	ldr	r0, [r0]
	ldr	r0, [r0]
	str	r0, [sp, #284]
	add	r0, sp, #232
	mov	r1, #0
	mov	r2, #48
	bl	_memset
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_2
	b	LBB3_1
LBB3_1:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_2:
	mov	r0, #0
	str	r0, [sp, #220]
	str	r0, [sp, #216]
	str	r0, [sp, #212]
	mov	r1, #5
	str	r1, [sp, #208]
	str	r0, [sp, #204]
	mov	r0, #1
	str	r0, [sp, #200]
	ldr	r0, [sp, #200]
	ldr	r1, [sp, #204]
	orr	r0, r0, r1
	cmp	r0, #0
	bne	LBB3_4
	b	LBB3_3
LBB3_3:
	ldr	r0, LCPI3_1
LPC3_1:
	add	r0, pc, r0
	bl	_cct_rt_fail
	b	LBB3_4
LBB3_4:
	ldr	r0, [sp, #216]
	ldr	r1, [sp, #220]
	str	r1, [sp, #196]
	str	r0, [sp, #192]
	b	LBB3_5
LBB3_5:                                 @ =>This Inner Loop Header: Depth=1
	ldr	r1, [sp, #200]
	ldr	r0, [sp, #204]
	subs	r1, r1, #1
	sbcs	r0, r0, #0
	blt	LBB3_7
	b	LBB3_6
LBB3_6:                                 @   in Loop: Header=BB3_5 Depth=1
	ldr	r3, [sp, #192]
	ldr	r1, [sp, #196]
	ldr	r2, [sp, #208]
	ldr	r0, [sp, #212]
	subs	r2, r2, r3
	sbcs	r0, r0, r1
	mov	r0, #0
	movge	r0, #1
	str	r0, [sp, #76]                   @ 4-byte Spill
	b	LBB3_8
LBB3_7:                                 @   in Loop: Header=BB3_5 Depth=1
	ldr	r2, [sp, #192]
	ldr	r0, [sp, #196]
	ldr	r3, [sp, #208]
	ldr	r1, [sp, #212]
	subs	r2, r2, r3
	sbcs	r0, r0, r1
	mov	r0, #0
	movge	r0, #1
	str	r0, [sp, #76]                   @ 4-byte Spill
	b	LBB3_8
LBB3_8:                                 @   in Loop: Header=BB3_5 Depth=1
	ldr	r0, [sp, #76]                   @ 4-byte Reload
	cmp	r0, #0
	beq	LBB3_15
	b	LBB3_9
LBB3_9:                                 @   in Loop: Header=BB3_5 Depth=1
	ldr	r1, [sp, #192]
	ldr	r0, [sp, #196]
	adds	r3, r1, #2
	adc	lr, r0, #0
	umull	r0, r2, r3, r3
	mla	r1, r3, lr, r2
	mla	r2, r3, lr, r1
	umull	r1, r4, r0, r3
	mla	r12, r0, lr, r4
	mla	r0, r2, r3, r12
	str	r1, [sp, #184]
	str	r0, [sp, #188]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_11
	b	LBB3_10
LBB3_10:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_11:                                @   in Loop: Header=BB3_5 Depth=1
	ldr	r0, [sp, #184]
	ldr	r1, [sp, #188]
	mov	r2, #5
	str	r2, [sp, #60]                   @ 4-byte Spill
	mov	r3, #0
	str	r3, [sp, #64]                   @ 4-byte Spill
	bl	_cct_floor_div_ll
	ldr	r2, [sp, #60]                   @ 4-byte Reload
	ldr	r3, [sp, #64]                   @ 4-byte Reload
	str	r0, [sp, #68]                   @ 4-byte Spill
	str	r1, [sp, #72]                   @ 4-byte Spill
	ldr	r0, [sp, #184]
	ldr	r1, [sp, #188]
	bl	_cct_euclid_mod_ll
	ldr	r2, [sp, #68]                   @ 4-byte Reload
	mov	r3, r0
	mov	r0, r1
	ldr	r1, [sp, #72]                   @ 4-byte Reload
	adds	r2, r2, r3
	adc	r0, r1, r0
	ldr	r12, [sp, #192]
	add	r3, sp, #232
	add	r1, r3, r12, lsl #3
	str	r2, [r3, r12, lsl #3]
	str	r0, [r1, #4]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_13
	b	LBB3_12
LBB3_12:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_13:                                @   in Loop: Header=BB3_5 Depth=1
	b	LBB3_14
LBB3_14:                                @   in Loop: Header=BB3_5 Depth=1
	ldr	r3, [sp, #200]
	ldr	r2, [sp, #204]
	ldr	r1, [sp, #192]
	ldr	r0, [sp, #196]
	adds	r1, r1, r3
	adc	r0, r0, r2
	str	r1, [sp, #192]
	str	r0, [sp, #196]
	b	LBB3_5
LBB3_15:
	mov	r0, #0
	str	r0, [sp, #180]
	str	r0, [sp, #176]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_17
	b	LBB3_16
LBB3_16:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_17:
	mov	r0, #0
	str	r0, [sp, #172]
	str	r0, [sp, #168]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_19
	b	LBB3_18
LBB3_18:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_19:
	b	LBB3_20
LBB3_20:                                @ =>This Inner Loop Header: Depth=1
	ldr	r1, [sp, #168]
	ldr	r0, [sp, #172]
	subs	r1, r1, #6
	sbcs	r0, r0, #0
	mov	r0, #0
	movlt	r0, #1
	cmp	r0, #0
	beq	LBB3_26
	b	LBB3_21
LBB3_21:                                @   in Loop: Header=BB3_20 Depth=1
	ldr	r1, [sp, #176]
	ldr	r0, [sp, #180]
	ldr	r12, [sp, #168]
	add	r3, sp, #232
	add	r2, r3, r12, lsl #3
	ldr	r2, [r2, #4]
	ldr	r3, [r3, r12, lsl #3]
	adds	r1, r1, r3
	adc	r0, r0, r2
	str	r1, [sp, #176]
	str	r0, [sp, #180]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_23
	b	LBB3_22
LBB3_22:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_23:                                @   in Loop: Header=BB3_20 Depth=1
	ldr	r1, [sp, #168]
	ldr	r0, [sp, #172]
	adds	r1, r1, #1
	adc	r0, r0, #0
	str	r1, [sp, #168]
	str	r0, [sp, #172]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_25
	b	LBB3_24
LBB3_24:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_25:                                @   in Loop: Header=BB3_20 Depth=1
	b	LBB3_20
LBB3_26:
	add	r0, sp, #132
	mov	r1, #0
	mov	r2, #36
	bl	_memset
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_28
	b	LBB3_27
LBB3_27:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_28:
	ldr	r0, LCPI3_2
LPC3_2:
	add	r0, pc, r0
	str	r0, [sp, #132]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_30
	b	LBB3_29
LBB3_29:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_30:
	mov	r1, sp
	mov	r3, #0
	str	r3, [r1, #4]
	mov	r0, #2
	str	r0, [r1]
	mov	r0, #7
	mov	r2, #4
	mov	r1, r3
	bl	_cct_fn_energia_orbital
	str	r1, [sp, #124]
	str	r0, [sp, #120]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_32
	b	LBB3_31
LBB3_31:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_32:
	ldr	r0, [sp, #120]
	ldr	r1, [sp, #124]
	str	r1, [sp, #140]
	str	r0, [sp, #136]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_34
	b	LBB3_33
LBB3_33:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_34:
	ldr	r0, [sp, #176]
	ldr	r1, [sp, #180]
	mov	r2, #9
	mov	r3, #0
	bl	_cct_euclid_mod_ll
	str	r1, [sp, #148]
	str	r0, [sp, #144]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_36
	b	LBB3_35
LBB3_35:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_36:
	ldr	r0, [sp, #176]
	ldr	r1, [sp, #180]
	mov	r2, #3
	mov	r3, #0
	str	r3, [sp, #48]                   @ 4-byte Spill
	bl	_cct_floor_div_ll
	ldr	r3, [sp, #48]                   @ 4-byte Reload
	str	r0, [sp, #56]                   @ 4-byte Spill
	str	r1, [sp, #52]                   @ 4-byte Spill
	ldr	r0, [sp, #176]
	ldr	r1, [sp, #180]
	mov	r2, #7
	bl	_cct_euclid_mod_ll
	mov	r3, r0
	ldr	r0, [sp, #52]                   @ 4-byte Reload
	mov	r2, r1
	ldr	r1, [sp, #56]                   @ 4-byte Reload
	adds	r1, r1, r3
	adc	r0, r0, r2
	str	r1, [sp, #152]
	str	r0, [sp, #156]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_38
	b	LBB3_37
LBB3_37:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_38:
	mov	r0, #0
	str	r0, [sp, #164]
	str	r0, [sp, #160]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_40
	b	LBB3_39
LBB3_39:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_40:
	add	r0, sp, #132
	mov	r1, #4
	mov	r2, #0
	bl	_cct_fn_auditar_registro
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_42
	b	LBB3_41
LBB3_41:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_42:
	ldr	r0, [sp, #152]
	ldr	r1, [sp, #156]
	bl	_cct_fn_avaliar_limite
	str	r1, [sp, #116]
	str	r0, [sp, #112]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_44
	b	LBB3_43
LBB3_43:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_44:
	ldr	r2, [sp, #136]
	ldr	r3, [sp, #140]
	umull	r0, r1, r2, r2
	mla	r12, r2, r3, r1
	mla	r1, r2, r3, r12
	mov	r2, #13
	mov	r3, #0
	str	r3, [sp, #36]                   @ 4-byte Spill
	bl	_cct_floor_div_ll
	ldr	r3, [sp, #36]                   @ 4-byte Reload
	str	r0, [sp, #44]                   @ 4-byte Spill
	str	r1, [sp, #40]                   @ 4-byte Spill
	ldr	r0, [sp, #152]
	ldr	r1, [sp, #156]
	mov	r2, #10
	bl	_cct_euclid_mod_ll
	mov	r3, r0
	ldr	r0, [sp, #40]                   @ 4-byte Reload
	mov	r2, r1
	ldr	r1, [sp, #44]                   @ 4-byte Reload
	adds	r1, r1, r3
	adc	r0, r0, r2
	ldr	r3, [sp, #160]
	ldr	r2, [sp, #164]
	adds	r1, r1, r3
	adc	r0, r0, r2
	str	r1, [sp, #104]
	str	r0, [sp, #108]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_46
	b	LBB3_45
LBB3_45:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_46:
	mov	r1, #524288
	orr	r1, r1, #1073741824
	mov	r2, #0
	str	r2, [sp, #28]                   @ 4-byte Spill
	mov	r3, #1073741824
	str	r3, [sp, #32]                   @ 4-byte Spill
	mov	r0, r2
	bl	_cct_pow
	mov	r2, r0
	ldr	r0, [sp, #28]                   @ 4-byte Reload
	mov	r3, r1
	ldr	r1, [sp, #32]                   @ 4-byte Reload
	bl	_cct_pow
	bl	___fixdfdi
	str	r1, [sp, #100]
	str	r0, [sp, #96]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_48
	b	LBB3_47
LBB3_47:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_48:
	ldr	r0, [sp, #96]
	ldr	r1, [sp, #100]
	mov	r2, #128
	mov	r3, #0
	str	r3, [sp, #16]                   @ 4-byte Spill
	bl	_cct_floor_div_ll
	ldr	r3, [sp, #16]                   @ 4-byte Reload
	str	r0, [sp, #24]                   @ 4-byte Spill
	str	r1, [sp, #20]                   @ 4-byte Spill
	ldr	r0, [sp, #96]
	ldr	r1, [sp, #100]
	mov	r2, #7
	bl	_cct_euclid_mod_ll
	mov	r3, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	mov	r2, r1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	adds	r1, r1, r3
	adc	r0, r0, r2
	str	r1, [sp, #88]
	str	r0, [sp, #92]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_50
	b	LBB3_49
LBB3_49:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_50:
	ldr	r1, [sp, #104]
	ldr	r0, [sp, #108]
	ldr	r3, [sp, #112]
	ldr	r2, [sp, #116]
	adds	r1, r1, r3
	adc	r0, r0, r2
	ldr	r3, [sp, #144]
	ldr	r2, [sp, #148]
	adds	r1, r1, r3
	adc	r0, r0, r2
	ldr	r3, [sp, #88]
	ldr	r2, [sp, #92]
	adds	r1, r1, r3
	adc	r0, r0, r2
	str	r1, [sp, #80]
	str	r0, [sp, #84]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB3_52
	b	LBB3_51
LBB3_51:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_52:
	ldr	r0, LCPI3_3
LPC3_3:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp, #132]
	bl	_cct_rt_scribe_str
	ldr	r0, LCPI3_4
LPC3_4:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp, #176]
	ldr	r1, [sp, #180]
	bl	_cct_rt_scribe_int
	ldr	r0, LCPI3_5
LPC3_5:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp, #136]
	ldr	r1, [sp, #140]
	bl	_cct_rt_scribe_int
	ldr	r0, LCPI3_6
LPC3_6:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp, #144]
	ldr	r1, [sp, #148]
	bl	_cct_rt_scribe_int
	ldr	r0, LCPI3_7
LPC3_7:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp, #152]
	ldr	r1, [sp, #156]
	bl	_cct_rt_scribe_int
	ldr	r0, LCPI3_8
LPC3_8:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp, #160]
	ldr	r1, [sp, #164]
	bl	_cct_rt_scribe_int
	ldr	r0, LCPI3_9
LPC3_9:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp, #104]
	ldr	r1, [sp, #108]
	bl	_cct_rt_scribe_int
	ldr	r0, LCPI3_10
LPC3_10:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp, #80]
	ldr	r1, [sp, #84]
	bl	_cct_rt_scribe_int
	ldr	r0, LCPI3_11
LPC3_11:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r1, [sp, #80]
	ldr	r0, [sp, #84]
	subs	r1, r1, #1
	sbcs	r0, r0, #0
	blt	LBB3_54
	b	LBB3_53
LBB3_53:
	mov	r0, #1
	cmp	r0, #0
	bne	LBB3_55
	b	LBB3_56
LBB3_54:
	mov	r0, #1
	cmp	r0, #0
	bne	LBB3_56
	b	LBB3_55
LBB3_55:
	mov	r0, #0
	str	r0, [sp, #228]
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_56:
	mov	r0, #0
	str	r0, [sp, #228]
	mov	r0, #1
	str	r0, [sp, #224]
	b	LBB3_57
LBB3_57:
	ldr	r0, [sp, #224]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #228]
	str	r0, [sp, #12]                   @ 4-byte Spill
	ldr	r0, LCPI3_12
LPC3_12:
	add	r0, pc, r0
	ldr	r0, [r0]
	ldr	r0, [r0]
	ldr	r1, [sp, #284]
	cmp	r0, r1
	bne	LBB3_59
	b	LBB3_58
LBB3_58:
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	sub	sp, r7, #8
	pop	{r4, r5, r7, lr}
	bx	lr
LBB3_59:
	bl	___stack_chk_fail
	.p2align	2
@ %bb.60:
	.data_region
LCPI3_0:
	.long	L___stack_chk_guard$non_lazy_ptr-(LPC3_0+8)
LCPI3_1:
	.long	L_.str-(LPC3_1+8)
LCPI3_2:
	.long	_cct_str_4-(LPC3_2+8)
LCPI3_3:
	.long	_cct_str_5-(LPC3_3+8)
LCPI3_4:
	.long	_cct_str_6-(LPC3_4+8)
LCPI3_5:
	.long	_cct_str_7-(LPC3_5+8)
LCPI3_6:
	.long	_cct_str_8-(LPC3_6+8)
LCPI3_7:
	.long	_cct_str_9-(LPC3_7+8)
LCPI3_8:
	.long	_cct_str_10-(LPC3_8+8)
LCPI3_9:
	.long	_cct_str_11-(LPC3_9+8)
LCPI3_10:
	.long	_cct_str_12-(LPC3_10+8)
LCPI3_11:
	.long	_cct_str_2-(LPC3_11+8)
LCPI3_12:
	.long	L___stack_chk_guard$non_lazy_ptr-(LPC3_12+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_fail
	.code	32                              @ @cct_rt_fail
_cct_rt_fail:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #12
	str	r0, [r7, #-4]
	ldr	r0, LCPI4_0
LPC4_0:
	ldr	r0, [pc, r0]
	ldr	r0, [r0]
	str	r0, [sp, #4]                    @ 4-byte Spill
	ldr	r0, [r7, #-4]
	cmp	r0, #0
	beq	LBB4_3
	b	LBB4_1
LBB4_1:
	ldr	r0, [r7, #-4]
	ldrsb	r0, [r0]
	cmp	r0, #0
	beq	LBB4_3
	b	LBB4_2
LBB4_2:
	ldr	r0, [r7, #-4]
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB4_4
LBB4_3:
	ldr	r0, LCPI4_1
LPC4_1:
	add	r0, pc, r0
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB4_4
LBB4_4:
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	ldr	r2, [sp]                        @ 4-byte Reload
	ldr	r1, LCPI4_2
LPC4_2:
	add	r1, pc, r1
	bl	_fprintf
	mov	r0, #1
	bl	_exit
	.p2align	2
@ %bb.5:
	.data_region
LCPI4_0:
	.long	L___stderrp$non_lazy_ptr-(LPC4_0+8)
LCPI4_1:
	.long	L_.str.2-(LPC4_1+8)
LCPI4_2:
	.long	L_.str.1-(LPC4_2+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_floor_div_ll
	.code	32                              @ @cct_floor_div_ll
_cct_floor_div_ll:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #40
	bic	sp, sp, #7
                                        @ kill: def $r12 killed $r3
                                        @ kill: def $r12 killed $r2
                                        @ kill: def $r12 killed $r1
                                        @ kill: def $r12 killed $r0
	str	r1, [sp, #28]
	str	r0, [sp, #24]
	str	r3, [sp, #20]
	str	r2, [sp, #16]
	ldr	r0, [sp, #16]
	ldr	r1, [sp, #20]
	orr	r0, r0, r1
	cmp	r0, #0
	bne	LBB5_2
	b	LBB5_1
LBB5_1:
	ldr	r0, LCPI5_0
LPC5_0:
	add	r0, pc, r0
	bl	_cct_rt_fail
	mov	r0, #0
	str	r0, [sp, #36]
	str	r0, [sp, #32]
	b	LBB5_6
LBB5_2:
	ldr	r0, [sp, #24]
	ldr	r1, [sp, #28]
	ldr	r2, [sp, #16]
	ldr	r3, [sp, #20]
	bl	___divdi3
	str	r1, [sp, #12]
	str	r0, [sp, #8]
	ldr	r0, [sp, #24]
	ldr	r1, [sp, #28]
	ldr	r2, [sp, #16]
	ldr	r3, [sp, #20]
	bl	___moddi3
	str	r1, [sp, #4]
	str	r0, [sp]
	ldr	r0, [sp]
	ldr	r1, [sp, #4]
	orr	r0, r0, r1
	cmp	r0, #0
	beq	LBB5_5
	b	LBB5_3
LBB5_3:
	ldr	r1, [sp]
	ldr	r0, [sp, #4]
	rsbs	r1, r1, #0
	rscs	r0, r0, #0
	mov	r1, #0
	mov	r0, r1
	movlt	r0, #1
	ldr	r3, [sp, #16]
	ldr	r2, [sp, #20]
	rsbs	r3, r3, #0
	rscs	r2, r2, #0
	movlt	r1, #1
	cmp	r0, r1
	beq	LBB5_5
	b	LBB5_4
LBB5_4:
	ldr	r1, [sp, #8]
	ldr	r0, [sp, #12]
	subs	r1, r1, #1
	sbc	r0, r0, #0
	str	r1, [sp, #8]
	str	r0, [sp, #12]
	b	LBB5_5
LBB5_5:
	ldr	r0, [sp, #8]
	ldr	r1, [sp, #12]
	str	r1, [sp, #36]
	str	r0, [sp, #32]
	b	LBB5_6
LBB5_6:
	ldr	r0, [sp, #32]
	ldr	r1, [sp, #36]
	mov	sp, r7
	pop	{r7, lr}
	bx	lr
	.p2align	2
@ %bb.7:
	.data_region
LCPI5_0:
	.long	L_.str.3-(LPC5_0+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_euclid_mod_ll
	.code	32                              @ @cct_euclid_mod_ll
_cct_euclid_mod_ll:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #48
	bic	sp, sp, #7
                                        @ kill: def $r12 killed $r3
                                        @ kill: def $r12 killed $r2
                                        @ kill: def $r12 killed $r1
                                        @ kill: def $r12 killed $r0
	str	r1, [sp, #36]
	str	r0, [sp, #32]
	str	r3, [sp, #28]
	str	r2, [sp, #24]
	ldr	r0, [sp, #24]
	ldr	r1, [sp, #28]
	orr	r0, r0, r1
	cmp	r0, #0
	bne	LBB6_2
	b	LBB6_1
LBB6_1:
	ldr	r0, LCPI6_0
LPC6_0:
	add	r0, pc, r0
	bl	_cct_rt_fail
	mov	r0, #0
	str	r0, [sp, #44]
	str	r0, [sp, #40]
	b	LBB6_8
LBB6_2:
	ldr	r0, [sp, #32]
	ldr	r1, [sp, #36]
	ldr	r2, [sp, #24]
	ldr	r3, [sp, #28]
	bl	___moddi3
	str	r1, [sp, #20]
	str	r0, [sp, #16]
	ldr	r0, [sp, #28]
	cmn	r0, #1
	bgt	LBB6_4
	b	LBB6_3
LBB6_3:
	ldr	r1, [sp, #24]
	ldr	r0, [sp, #28]
	rsbs	r1, r1, #0
	rsc	r0, r0, #0
	str	r1, [sp]                        @ 4-byte Spill
	str	r0, [sp, #4]                    @ 4-byte Spill
	b	LBB6_5
LBB6_4:
	ldr	r1, [sp, #24]
	ldr	r0, [sp, #28]
	str	r1, [sp]                        @ 4-byte Spill
	str	r0, [sp, #4]                    @ 4-byte Spill
	b	LBB6_5
LBB6_5:
	ldr	r1, [sp]                        @ 4-byte Reload
	ldr	r0, [sp, #4]                    @ 4-byte Reload
	str	r1, [sp, #8]
	str	r0, [sp, #12]
	ldr	r0, [sp, #20]
	cmn	r0, #1
	bgt	LBB6_7
	b	LBB6_6
LBB6_6:
	ldr	r3, [sp, #8]
	ldr	r2, [sp, #12]
	ldr	r1, [sp, #16]
	ldr	r0, [sp, #20]
	adds	r1, r1, r3
	adc	r0, r0, r2
	str	r1, [sp, #16]
	str	r0, [sp, #20]
	b	LBB6_7
LBB6_7:
	ldr	r0, [sp, #16]
	ldr	r1, [sp, #20]
	str	r1, [sp, #44]
	str	r0, [sp, #40]
	b	LBB6_8
LBB6_8:
	ldr	r0, [sp, #40]
	ldr	r1, [sp, #44]
	mov	sp, r7
	pop	{r7, lr}
	bx	lr
	.p2align	2
@ %bb.9:
	.data_region
LCPI6_0:
	.long	L_.str.4-(LPC6_0+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_fn_energia_orbital
	.code	32                              @ @cct_fn_energia_orbital
_cct_fn_energia_orbital:
@ %bb.0:
	push	{r4, r7, lr}
	add	r7, sp, #4
	sub	sp, sp, #60
	bic	sp, sp, #7
	mov	lr, r1
	mov	r12, r0
	ldr	r1, [r7, #12]
	ldr	r0, [r7, #8]
                                        @ kill: def $r4 killed $r3
                                        @ kill: def $r4 killed $r2
                                        @ kill: def $r4 killed $lr
                                        @ kill: def $r4 killed $r12
	str	lr, [sp, #44]
	str	r12, [sp, #40]
	str	r3, [sp, #36]
	str	r2, [sp, #32]
	str	r1, [sp, #28]
	str	r0, [sp, #24]
	ldr	r0, [sp, #40]
	ldr	r1, [sp, #44]
	ldr	r3, [sp, #32]
	ldr	r2, [sp, #36]
	adds	r0, r0, r3
	adc	r1, r1, r2
	bl	___floatdidf
	mov	r2, #0
	mov	r3, #1073741824
	bl	_cct_pow
	bl	___fixdfdi
	str	r1, [sp, #20]
	str	r0, [sp, #16]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB7_2
	b	LBB7_1
LBB7_1:
	mov	r0, #0
	str	r0, [sp, #56]
	str	r0, [sp, #52]
	b	LBB7_7
LBB7_2:
	ldr	r0, [sp, #16]
	ldr	r1, [sp, #20]
	ldr	r2, [sp, #24]
	ldr	r3, [sp, #28]
	adds	r2, r2, #2
	adc	r3, r3, #0
	bl	_cct_floor_div_ll
	str	r1, [sp, #12]
	str	r0, [sp, #8]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB7_4
	b	LBB7_3
LBB7_3:
	mov	r0, #0
	str	r0, [sp, #56]
	str	r0, [sp, #52]
	b	LBB7_7
LBB7_4:
	ldr	r0, [sp, #16]
	ldr	r1, [sp, #20]
	ldr	r2, [sp, #24]
	ldr	r3, [sp, #28]
	adds	r2, r2, #3
	adc	r3, r3, #0
	bl	_cct_euclid_mod_ll
	str	r1, [sp, #4]
	str	r0, [sp]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB7_6
	b	LBB7_5
LBB7_5:
	mov	r0, #0
	str	r0, [sp, #56]
	str	r0, [sp, #52]
	b	LBB7_7
LBB7_6:
	ldr	r1, [sp, #8]
	ldr	r0, [sp, #12]
	ldr	r3, [sp]
	ldr	r2, [sp, #4]
	adds	r1, r1, r3
	adc	r0, r0, r2
	str	r1, [sp, #52]
	str	r0, [sp, #56]
	b	LBB7_7
LBB7_7:
	ldr	r0, [sp, #52]
	ldr	r1, [sp, #56]
	sub	sp, r7, #4
	pop	{r4, r7, lr}
	bx	lr
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_fn_auditar_registro
	.code	32                              @ @cct_fn_auditar_registro
_cct_fn_auditar_registro:
@ %bb.0:
	push	{r4, r5, r7, lr}
	add	r7, sp, #8
	sub	sp, sp, #80
	bic	sp, sp, #7
                                        @ kill: def $r3 killed $r2
                                        @ kill: def $r3 killed $r1
	str	r0, [sp, #68]
	str	r2, [sp, #60]
	str	r1, [sp, #56]
	ldr	r0, [sp, #68]
	ldr	r1, LCPI8_0
LPC8_0:
	add	r1, pc, r1
	str	r1, [sp, #36]                   @ 4-byte Spill
	bl	_cct_rt_check_not_null
	ldr	r1, [sp, #36]                   @ 4-byte Reload
	ldr	r3, [r0, #4]
	ldr	r0, [r0, #8]
	ldr	lr, [sp, #56]
	ldr	r4, [sp, #60]
	umull	r12, r2, lr, lr
	mla	r5, lr, r4, r2
	mla	r2, lr, r4, r5
	adds	r3, r3, r12
	str	r3, [sp, #40]                   @ 4-byte Spill
	adc	r0, r0, r2
	str	r0, [sp, #44]                   @ 4-byte Spill
	ldr	r0, [sp, #68]
	bl	_cct_rt_check_not_null
	ldr	r2, [sp, #40]                   @ 4-byte Reload
	mov	r1, r0
	ldr	r0, [sp, #44]                   @ 4-byte Reload
	str	r2, [r1, #4]
	str	r0, [r1, #8]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB8_2
	b	LBB8_1
LBB8_1:
	mov	r0, #0
	str	r0, [sp, #76]
	str	r0, [sp, #72]
	b	LBB8_18
LBB8_2:
	ldr	r0, [sp, #68]
	ldr	r1, LCPI8_1
LPC8_1:
	add	r1, pc, r1
	str	r1, [sp, #24]                   @ 4-byte Spill
	bl	_cct_rt_check_not_null
	mov	r1, r0
	ldr	r0, [r1, #12]
	ldr	r1, [r1, #16]
	ldr	r3, [sp, #56]
	ldr	r2, [sp, #60]
	adds	r0, r0, r3
	adc	r1, r1, r2
	mov	r2, #11
	mov	r3, #0
	bl	_cct_euclid_mod_ll
	str	r0, [sp, #32]                   @ 4-byte Spill
	mov	r0, r1
	ldr	r1, [sp, #24]                   @ 4-byte Reload
	str	r0, [sp, #28]                   @ 4-byte Spill
	ldr	r0, [sp, #68]
	bl	_cct_rt_check_not_null
	ldr	r2, [sp, #28]                   @ 4-byte Reload
	mov	r1, r0
	ldr	r0, [sp, #32]                   @ 4-byte Reload
	str	r2, [r1, #16]
	str	r0, [r1, #12]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB8_4
	b	LBB8_3
LBB8_3:
	mov	r0, #0
	str	r0, [sp, #76]
	str	r0, [sp, #72]
	b	LBB8_18
LBB8_4:
	ldr	r0, [sp, #68]
	ldr	r1, LCPI8_2
LPC8_2:
	add	r1, pc, r1
	str	r1, [sp, #12]                   @ 4-byte Spill
	bl	_cct_rt_check_not_null
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	ldr	r2, [r0, #20]
	str	r2, [sp, #4]                    @ 4-byte Spill
	ldr	r0, [r0, #24]
	str	r0, [sp, #8]                    @ 4-byte Spill
	ldr	r0, [sp, #68]
	bl	_cct_rt_check_not_null
	mov	r1, r0
	ldr	r0, [r1, #4]
	ldr	r1, [r1, #8]
	mov	r2, #5
	mov	r3, #0
	bl	_cct_floor_div_ll
	ldr	r3, [sp, #4]                    @ 4-byte Reload
	mov	r12, r0
	ldr	r0, [sp, #8]                    @ 4-byte Reload
	mov	r2, r1
	ldr	r1, [sp, #12]                   @ 4-byte Reload
	adds	r3, r3, r12
	str	r3, [sp, #20]                   @ 4-byte Spill
	adc	r0, r0, r2
	str	r0, [sp, #16]                   @ 4-byte Spill
	ldr	r0, [sp, #68]
	bl	_cct_rt_check_not_null
	ldr	r2, [sp, #16]                   @ 4-byte Reload
	mov	r1, r0
	ldr	r0, [sp, #20]                   @ 4-byte Reload
	str	r2, [r1, #24]
	str	r0, [r1, #20]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB8_6
	b	LBB8_5
LBB8_5:
	mov	r0, #0
	str	r0, [sp, #76]
	str	r0, [sp, #72]
	b	LBB8_18
LBB8_6:
	ldr	r0, [sp, #68]
	ldr	r1, LCPI8_3
LPC8_3:
	add	r1, pc, r1
	bl	_cct_rt_check_not_null
	mov	r1, r0
	ldr	r0, [r1, #20]
	ldr	r1, [r1, #24]
	str	r1, [sp, #52]
	str	r0, [sp, #48]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB8_8
	b	LBB8_7
LBB8_7:
	mov	r0, #0
	str	r0, [sp, #76]
	str	r0, [sp, #72]
	b	LBB8_18
LBB8_8:
	ldr	r1, [sp, #48]
	ldr	r0, [sp, #52]
	subs	r1, r1, #71
	sbcs	r0, r0, #0
	blt	LBB8_10
	b	LBB8_9
LBB8_9:
	mov	r0, #1
	cmp	r0, #0
	bne	LBB8_11
	b	LBB8_14
LBB8_10:
	mov	r0, #1
	cmp	r0, #0
	bne	LBB8_14
	b	LBB8_11
LBB8_11:
	ldr	r0, [sp, #68]
	ldr	r1, LCPI8_4
LPC8_4:
	add	r1, pc, r1
	bl	_cct_rt_check_not_null
	mov	r1, r0
	mov	r0, #0
	str	r0, [r1, #32]
	mov	r0, #9
	str	r0, [r1, #28]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB8_13
	b	LBB8_12
LBB8_12:
	mov	r0, #0
	str	r0, [sp, #76]
	str	r0, [sp, #72]
	b	LBB8_18
LBB8_13:
	b	LBB8_17
LBB8_14:
	ldr	r0, [sp, #68]
	ldr	r1, LCPI8_5
LPC8_5:
	add	r1, pc, r1
	bl	_cct_rt_check_not_null
	mov	r1, r0
	mov	r0, #0
	str	r0, [r1, #32]
	mov	r0, #3
	str	r0, [r1, #28]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB8_16
	b	LBB8_15
LBB8_15:
	mov	r0, #0
	str	r0, [sp, #76]
	str	r0, [sp, #72]
	b	LBB8_18
LBB8_16:
	b	LBB8_17
LBB8_17:
	mov	r0, #0
	str	r0, [sp, #76]
	str	r0, [sp, #72]
	b	LBB8_18
LBB8_18:
	ldr	r0, [sp, #72]
	ldr	r1, [sp, #76]
	sub	sp, r7, #8
	pop	{r4, r5, r7, lr}
	bx	lr
	.p2align	2
@ %bb.19:
	.data_region
LCPI8_0:
	.long	L_.str.5-(LPC8_0+8)
LCPI8_1:
	.long	L_.str.5-(LPC8_1+8)
LCPI8_2:
	.long	L_.str.5-(LPC8_2+8)
LCPI8_3:
	.long	L_.str.5-(LPC8_3+8)
LCPI8_4:
	.long	L_.str.5-(LPC8_4+8)
LCPI8_5:
	.long	L_.str.5-(LPC8_5+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_fn_avaliar_limite
	.code	32                              @ @cct_fn_avaliar_limite
_cct_fn_avaliar_limite:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #32
	bic	sp, sp, #7
                                        @ kill: def $r2 killed $r1
                                        @ kill: def $r2 killed $r0
	str	r1, [sp, #20]
	str	r0, [sp, #16]
	mov	r0, #1
	str	r0, [sp, #12]
	mov	r0, #0
	str	r0, [sp, #8]
	str	r0, [sp, #4]
	ldr	r1, [sp, #16]
	ldr	r0, [sp, #20]
	subs	r1, r1, #96
	sbcs	r0, r0, #0
	blt	LBB9_2
	b	LBB9_1
LBB9_1:
	mov	r0, #1
	cmp	r0, #0
	bne	LBB9_3
	b	LBB9_4
LBB9_2:
	mov	r0, #1
	cmp	r0, #0
	bne	LBB9_4
	b	LBB9_3
LBB9_3:
	ldr	r0, LCPI9_0
LPC9_0:
	add	r0, pc, r0
	bl	_cct_rt_fractum_throw_str
	b	LBB9_5
LBB9_4:
	ldr	r0, [sp, #16]
	ldr	r1, [sp, #20]
	str	r1, [sp, #28]
	str	r0, [sp, #24]
	b	LBB9_20
LBB9_5:
	ldr	r0, [sp, #12]
	cmp	r0, #1
	bne	LBB9_7
	b	LBB9_6
LBB9_6:
	b	LBB9_8
LBB9_7:
	mov	r0, #1
	str	r0, [sp, #8]
	b	LBB9_9
LBB9_8:
	mov	r0, #2
	str	r0, [sp, #12]
	bl	_cct_rt_fractum_peek
	str	r0, [sp]
	bl	_cct_rt_fractum_clear
	ldr	r0, LCPI9_1
LPC9_1:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	ldr	r0, [sp]
	bl	_cct_rt_scribe_str
	ldr	r0, LCPI9_2
LPC9_2:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	mov	r0, #0
	str	r0, [sp, #28]
	mov	r0, #66
	str	r0, [sp, #24]
	b	LBB9_20
LBB9_9:
	mov	r0, #3
	str	r0, [sp, #12]
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB9_11
	b	LBB9_10
LBB9_10:
	bl	_cct_rt_fractum_peek
	str	r0, [sp, #4]
	bl	_cct_rt_fractum_clear
	b	LBB9_11
LBB9_11:
	ldr	r0, LCPI9_3
LPC9_3:
	add	r0, pc, r0
	bl	_cct_rt_scribe_str
	b	LBB9_12
LBB9_12:
	ldr	r0, [sp, #8]
	cmp	r0, #0
	beq	LBB9_16
	b	LBB9_13
LBB9_13:
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	bne	LBB9_16
	b	LBB9_14
LBB9_14:
	ldr	r0, [sp, #4]
	cmp	r0, #0
	beq	LBB9_16
	b	LBB9_15
LBB9_15:
	ldr	r0, [sp, #4]
	bl	_cct_rt_fractum_throw_str
	b	LBB9_16
LBB9_16:
	ldr	r0, [sp, #8]
	cmp	r0, #0
	bne	LBB9_18
	b	LBB9_17
LBB9_17:
	bl	_cct_rt_fractum_is_active
	cmp	r0, #0
	beq	LBB9_19
	b	LBB9_18
LBB9_18:
	mov	r0, #0
	str	r0, [sp, #28]
	str	r0, [sp, #24]
	b	LBB9_20
LBB9_19:
	mov	r0, #0
	str	r0, [sp, #28]
	str	r0, [sp, #24]
	b	LBB9_20
LBB9_20:
	ldr	r0, [sp, #24]
	ldr	r1, [sp, #28]
	mov	sp, r7
	pop	{r7, lr}
	bx	lr
	.p2align	2
@ %bb.21:
	.data_region
LCPI9_0:
	.long	_cct_str_0-(LPC9_0+8)
LCPI9_1:
	.long	_cct_str_1-(LPC9_1+8)
LCPI9_2:
	.long	_cct_str_2-(LPC9_2+8)
LCPI9_3:
	.long	_cct_str_3-(LPC9_3+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_pow
	.code	32                              @ @cct_pow
_cct_pow:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #16
	bic	sp, sp, #7
                                        @ kill: def $r12 killed $r3
                                        @ kill: def $r12 killed $r2
                                        @ kill: def $r12 killed $r1
                                        @ kill: def $r12 killed $r0
	str	r1, [sp, #12]
	str	r0, [sp, #8]
	str	r3, [sp, #4]
	str	r2, [sp]
	ldr	r0, [sp, #8]
	ldr	r1, [sp, #12]
	ldr	r2, [sp]
	ldr	r3, [sp, #4]
	bl	_pow
	mov	sp, r7
	pop	{r7, lr}
	bx	lr
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_scribe_str
	.code	32                              @ @cct_rt_scribe_str
_cct_rt_scribe_str:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #8
	str	r0, [sp, #4]
	ldr	r0, [sp, #4]
	cmp	r0, #0
	beq	LBB11_2
	b	LBB11_1
LBB11_1:
	ldr	r0, [sp, #4]
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB11_3
LBB11_2:
	ldr	r0, LCPI11_0
LPC11_0:
	add	r0, pc, r0
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB11_3
LBB11_3:
	ldr	r0, [sp]                        @ 4-byte Reload
	ldr	r1, LCPI11_1
LPC11_1:
	ldr	r1, [pc, r1]
	ldr	r1, [r1]
	bl	_fputs
	mov	sp, r7
	pop	{r7, lr}
	bx	lr
	.p2align	2
@ %bb.4:
	.data_region
LCPI11_0:
	.long	L_.str.7-(LPC11_0+8)
LCPI11_1:
	.long	L___stdoutp$non_lazy_ptr-(LPC11_1+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_scribe_int
	.code	32                              @ @cct_rt_scribe_int
_cct_rt_scribe_int:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #8
	bic	sp, sp, #7
                                        @ kill: def $r2 killed $r1
                                        @ kill: def $r2 killed $r0
	str	r1, [sp, #4]
	str	r0, [sp]
	ldr	r1, [sp]
	ldr	r2, [sp, #4]
	ldr	r0, LCPI12_0
LPC12_0:
	add	r0, pc, r0
	bl	_printf
	mov	sp, r7
	pop	{r7, lr}
	bx	lr
	.p2align	2
@ %bb.1:
	.data_region
LCPI12_0:
	.long	L_.str.8-(LPC12_0+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_check_not_null
	.code	32                              @ @cct_rt_check_not_null
_cct_rt_check_not_null:
@ %bb.0:
	push	{r7, lr}
	mov	r7, sp
	sub	sp, sp, #12
	str	r0, [r7, #-4]
	str	r1, [sp, #4]
	ldr	r0, [r7, #-4]
	cmp	r0, #0
	bne	LBB13_6
	b	LBB13_1
LBB13_1:
	ldr	r0, [sp, #4]
	cmp	r0, #0
	beq	LBB13_4
	b	LBB13_2
LBB13_2:
	ldr	r0, [sp, #4]
	ldrsb	r0, [r0]
	cmp	r0, #0
	beq	LBB13_4
	b	LBB13_3
LBB13_3:
	ldr	r0, [sp, #4]
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB13_5
LBB13_4:
	ldr	r0, LCPI13_0
LPC13_0:
	add	r0, pc, r0
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB13_5
LBB13_5:
	ldr	r0, [sp]                        @ 4-byte Reload
	bl	_cct_rt_fail
	b	LBB13_6
LBB13_6:
	ldr	r0, [r7, #-4]
	mov	sp, r7
	pop	{r7, lr}
	bx	lr
	.p2align	2
@ %bb.7:
	.data_region
LCPI13_0:
	.long	L_.str.6-(LPC13_0+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_fractum_throw_str
	.code	32                              @ @cct_rt_fractum_throw_str
_cct_rt_fractum_throw_str:
@ %bb.0:
	sub	sp, sp, #8
	str	r0, [sp, #4]
	ldr	r1, LCPI14_0
LPC14_0:
	add	r1, pc, r1
	mov	r0, #1
	str	r0, [r1]
	ldr	r0, [sp, #4]
	cmp	r0, #0
	beq	LBB14_2
	b	LBB14_1
LBB14_1:
	ldr	r0, [sp, #4]
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB14_3
LBB14_2:
	ldr	r0, LCPI14_1
LPC14_1:
	add	r0, pc, r0
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB14_3
LBB14_3:
	ldr	r0, [sp]                        @ 4-byte Reload
	ldr	r1, LCPI14_2
LPC14_2:
	add	r1, pc, r1
	str	r0, [r1]
	add	sp, sp, #8
	bx	lr
	.p2align	2
@ %bb.4:
	.data_region
LCPI14_0:
	.long	_cct_rt_fractum_active-(LPC14_0+8)
LCPI14_1:
	.long	L_.str.7-(LPC14_1+8)
LCPI14_2:
	.long	_cct_rt_fractum_msg-(LPC14_2+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_fractum_peek
	.code	32                              @ @cct_rt_fractum_peek
_cct_rt_fractum_peek:
@ %bb.0:
	sub	sp, sp, #4
	ldr	r0, LCPI15_0
LPC15_0:
	ldr	r0, [pc, r0]
	cmp	r0, #0
	beq	LBB15_2
	b	LBB15_1
LBB15_1:
	ldr	r0, LCPI15_1
LPC15_1:
	ldr	r0, [pc, r0]
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB15_3
LBB15_2:
	ldr	r0, LCPI15_2
LPC15_2:
	add	r0, pc, r0
	str	r0, [sp]                        @ 4-byte Spill
	b	LBB15_3
LBB15_3:
	ldr	r0, [sp]                        @ 4-byte Reload
	add	sp, sp, #4
	bx	lr
	.p2align	2
@ %bb.4:
	.data_region
LCPI15_0:
	.long	_cct_rt_fractum_msg-(LPC15_0+8)
LCPI15_1:
	.long	_cct_rt_fractum_msg-(LPC15_1+8)
LCPI15_2:
	.long	L_.str.7-(LPC15_2+8)
	.end_data_region
                                        @ -- End function
	.p2align	2                               @ -- Begin function cct_rt_fractum_clear
	.code	32                              @ @cct_rt_fractum_clear
_cct_rt_fractum_clear:
@ %bb.0:
	ldr	r1, LCPI16_0
LPC16_0:
	add	r1, pc, r1
	mov	r0, #0
	str	r0, [r1]
	ldr	r1, LCPI16_1
LPC16_1:
	add	r1, pc, r1
	str	r0, [r1]
	bx	lr
	.p2align	2
@ %bb.1:
	.data_region
LCPI16_0:
	.long	_cct_rt_fractum_active-(LPC16_0+8)
LCPI16_1:
	.long	_cct_rt_fractum_msg-(LPC16_1+8)
	.end_data_region
                                        @ -- End function
	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 @ @.str
	.asciz	"REPETE GRADUS cannot be 0"

	.section	__TEXT,__const
_cct_str_4:                             @ @cct_str_4
	.asciz	"Astra-V2"

_cct_str_5:                             @ @cct_str_5
	.asciz	"nomen="

_cct_str_6:                             @ @cct_str_6
	.asciz	" soma="

_cct_str_7:                             @ @cct_str_7
	.asciz	" poder="

_cct_str_8:                             @ @cct_str_8
	.asciz	" resto="

_cct_str_9:                             @ @cct_str_9
	.asciz	" risco="

_cct_str_10:                            @ @cct_str_10
	.asciz	" bonus="

_cct_str_11:                            @ @cct_str_11
	.asciz	" assinatura="

_cct_str_12:                            @ @cct_str_12
	.asciz	" controle="

_cct_str_2:                             @ @cct_str_2
	.asciz	"\n"

	.section	__TEXT,__cstring,cstring_literals
L_.str.1:                               @ @.str.1
	.asciz	"cct runtime: %s\n"

L_.str.2:                               @ @.str.2
	.asciz	"failure"

L_.str.3:                               @ @.str.3
	.asciz	"integer division by zero"

L_.str.4:                               @ @.str.4
	.asciz	"euclidean modulo by zero"

L_.str.5:                               @ @.str.5
	.asciz	"runtime-fail (bridged): null pointer dereference"

L_.str.6:                               @ @.str.6
	.asciz	"null pointer"

	.section	__TEXT,__const
_cct_str_0:                             @ @cct_str_0
	.asciz	"zona rubra"

_cct_str_1:                             @ @cct_str_1
	.asciz	"[limite.cape] "

_cct_str_3:                             @ @cct_str_3
	.asciz	"[limite.semper]\n"

.zerofill __DATA,__bss,_cct_rt_fractum_active,4,2 @ @cct_rt_fractum_active
	.section	__TEXT,__cstring,cstring_literals
L_.str.7:                               @ @.str.7
	.space	1

.zerofill __DATA,__bss,_cct_rt_fractum_msg,4,2 @ @cct_rt_fractum_msg
L_.str.8:                               @ @.str.8
	.asciz	"%lld"

L_.str.9:                               @ @.str.9
	.asciz	"cct runtime: uncaught FRACTUM: %s\n"

	.section	__DATA,__nl_symbol_ptr,non_lazy_symbol_pointers
	.p2align	2, 0x0
L___stack_chk_guard$non_lazy_ptr:
	.indirect_symbol	___stack_chk_guard
	.long	0
L___stderrp$non_lazy_ptr:
	.indirect_symbol	___stderrp
	.long	0
L___stdoutp$non_lazy_ptr:
	.indirect_symbol	___stdoutp
	.long	0

.subsections_via_symbols
