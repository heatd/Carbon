.section .text

.global _start
_start:
	sub $16, %rsp
	mov $1, %eax
	mov $2, %rdi
	mov $0xDEADBEEF, %rsi
	syscall
	mov $1, %eax
	mov $0, %rdi
	mov $0, %rsi
	syscall
	mov (%rsp), %r11
	jmp .