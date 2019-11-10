main:
	movq si, .msg_start
	call putstr
	
	movq rcx, 99023
	movq rbx, -83764
	movd cnt, 0x10000000
.loop:
	add rcx, rbx
	mul rbx, 13
	xor rbx, 0xC9025621FE21
	dec cnt
	jnz cnt, .loop
.end:
	
	movw si, .msg_end
	call putstr
	halt
.msg_start:
	db "Starting CPU-intensive test...\n", 0
.msg_end:
	db "Done!\n", 0
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
