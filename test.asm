.org 0

main:
	movq	si, str_putstr_test
	call	putstr
	call	test_math
	call	test_mov
	call	test_strops
exit:
	halt
error:
	movw	si, str_error
	call	putstr
	halt
str_error:
	db "Error!\n", 0
str_putstr_test:
	db "putstr test\n", 0
test_mov:
	push	si
	push	di
	push	r0
	push	r1
	movq	si, .str_mov_test
	call	putstr
	movw	si, .str_loadmem
	call	putstr

	movq	rax, 128
	call	malloc
	
	add	di, 64	; start in the middle so we can test negatives

	movb	rax, 100
	movq	[di], rax
	movq	rbx, [di]
	sub	rbx, 100
	jnz rbx, error
	
	movb	rax, 66
	movd	r1, 0
	movb	[di + 22], rax
	movb	r1, [di + 22]
	sub	r1, 66
	jnz	r1, error
	
	movb	[di - 30], rax
	sub di, 30
	movb	r1, [di]
	sub	r1, 66
	jnz	r1, error
	
	movw	si, .str_mov_pass
	call	putstr
	
	pop r1
	pop r0
	pop di
	pop si
	ret
	
.str_mov_test:
	db "Mov test:\n", 0
.str_loadmem:
	db "Testing mov[s] r1i, [r2i + **] and mov[s] [r2i + **], r1i\n > Note: this test uses heap.  If that fails this will too.\n", 0
.str_mov_pass:
	db " $ MOV PASS OVERALL - NO ERRORS\n", 0


test_math:
	push	si
	push	di
	push	r0
	push	r1
	movq	si, .str_math_test
	call	putstr
	movw	si, .str_inc_dec
	call	putstr
	
	; test that inc and dec work
	
	movd	r0, 1
	dec	r0
	jnz	r0, error
	
	movq	rax, -1
	inc	rax
	cmpq rax, 0
	jnz	error
	
	movd	r1, 0
	inc	r1
	jz	r1, error
	
	movw	si, .str_regop
	call	putstr
	
	; test various operations on registers only
	
	movd	r0, 9
	movq	di, 10
	sub	r0, di
	jz	r0, error
	cmp r0, -1
	jnz	error
	
	movd	r0, -10
	movq	di, 10
	add	r0, di
	jnz	r0, error
	
	movd	r0, -10
	movq	di, 0
	mul	di, r0
	jnz	di, error
	
	movd	r0, 0x55
	movd	r1, 0xAA
	and	r0, r1
	jnz	r0, error
	
	movd	r0, 0xAA
	movd	r1, 0xAA
	xor	r0, r1
	jnz	r0, error
	
	movd	r0, -150
	movd	r1, 1
	ras	r0, r1
	movd	r1, -75
	sub	r0, r1
	cmp r0, 0
	jg	error
	jnz	r0, error
	
	movw	si, .str_constop
	call	putstr
	
	; test various operations on registers and constants
	
	movd	r0, 9
	sub	r0, 10
	jz	r0, error
	inc	r0
	jnz	r0, error
	
	movd	r0, -10
	add	r0, 10
	jnz	r0, error
	
	movq	di, 0
	mul	di, -10
	jnz	di, error
	
	movd	r0, 0x55
	and	r0, 0xAB
	dec	r0
	jnz	r0, error
	
	movd	r0, 0x67
	xor	r0, 0x67
	jnz	r0, error
	
	movd	r0, -300
	ras	r0, 2
	sub	r0, -75
	jnz	r0, error
	
	movw	si, .str_math_pass
	call	putstr
	
	pop r1
	pop r0
	pop di
	pop si
	ret
.str_math_test:
	db "Math test:\n", 0
.str_inc_dec:
	db "Testing Inc/Dec...\n", 0
.str_regop:
	db "Testing Regops...\n", 0
.str_constop:
	db "Testing Constops...\n", 0
.str_math_pass:
	db " $ MATH PASS OVERALL - NO ERRORS\n", 0
	
test_strops:
	push si
	push di
	
	movq si, .str_strop_test
	call putstr
	
	xor r1, r1
	xor rax, rax
	movq si, .str_strfi_test
	movb rax, '.'
	strfi si, rax, .strfi_test_f
	
	movq di, .str_strcat_0
	movq si, .str_strcat_1
	strcat di, si
	movq si, .str_strcat_0
	call putstr
	
	movq si, .str_test_strcmp
	call putstr
	movq di, .str_strcmp_0
	movq si, .str_strcmp_1
	strcmp di, si
	sub rax, -13
	jnz rax, error
	
	movq si, .str_test_strcpy
	call putstr
	movq di, .str_strcmp_0
	movq si, .str_strcmp_1
	strcpy di, si
	strcmp di, si
	jnz rax, error
	
	movq si, .str_test_strlen
	call putstr
	movq si, .str_strcmp_0
	strlen si
	sub rax, 6
	jnz rax, error
	
	movq si, .str_pass
	call putstr
	
	pop di
	pop si
	ret
.str_strop_test:
	db "String ops test:\n", 0
.str_strfi_test:
	db ".s.trfi. is .work.ing.!.\n.", 0
.str_strcat_0:
	db "strcat ", 0
.str_strcat_1:
	db "is working!\n", 0
.str_strcmp_0:
	db "Hello!", 0
.str_strcmp_1:
	db "Hello.", 0
.str_test_strcmp:
	db "Testing strcmp...\n", 0
.str_test_strcpy:
	db "Testing strcpy...\n", 0
.str_test_strlen:
	db "Testing strlen...\n", 0
.str_pass:
	db "String tests passed!\n", 0
.strfi_test_f:
	lodb
	movd r1, rax
	sub r1, '.'
	jz r1, .strfi_ret
	jz rax, .strfi_ret
	syscall 2
	jmp .strfi_test_f
.strfi_ret:
	ret


putstr:
	syscall 4
	ret
malloc:
	push rax
	syscall 0
	movq di, rax
	pop rax
	ret
free:
	syscall 1
	ret
