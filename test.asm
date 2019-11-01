.org 0
test_sys_putchar:
        movb rbx, 'F'
	movd s rbx, 0x99023344
	movd rcx, rbx
	movq rax, 42
	exx  rax, rcx
        syscall 2
        movb rax, 'O'
        syscall 2
        movb rax, 'O'
        syscall 2
        movb rax, 'F'
        syscall 2
	movb rax, '!'
	syscall 2
	halt
