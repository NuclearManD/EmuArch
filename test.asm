.org 0
test_sys_putchar:
        movb rax, 'F'
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
