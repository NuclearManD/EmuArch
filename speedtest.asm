main:
	movq si, .msg_start
	call putstr
	
	syscall 5 ; millis() -> rax

	movq rcx, 99023
	movq rbx, -83764
	movd cnt, 0x10000000
	movq rdx, 0xC9025621FE21
	movd r2, 13
.loop:
	add rcx, rbx
	mul rbx, r2
	xor rbx, rdx
	dec cnt
	jnz cnt, .loop
.end:
	exx rbx, rax
	syscall 5
	sub rax, rbx

	movw si, .msg_end
	push rax
	push rcx
	call printf
	halt
.msg_start:
	db "Running CPU-intensive test...\n", 0
.msg_end:
	db "Done!\nResult: %i\nTime taken: %i ms\n\n", 0
putstr:
	syscall 4
	ret
printf:
	pop rdx
	syscall 3
	push rdx
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
