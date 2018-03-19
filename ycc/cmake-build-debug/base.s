	.text
.global f
f:
	nop
	push %rbp
	mov %rsp, %rbp
	sub $8, %rsp
	.file 1 "../snip/base.c"
	.loc 1 3 0
	# int a=2+3;
	mov $2, %rax
	push %rax
	mov $3, %rax
	mov %rax, %rcx
	pop %rax
	add %rcx, %rax
	mov %eax, -8(%rbp)
	leave
	ret
	.data
	.global bo
	.lcomm bo, 4
	.data 0
.global aq
aq:
	.long 1
	.text
.global g
g:
	nop
	push %rbp
	mov %rsp, %rbp
	push %rdi
	.loc 1 9 0
	# return a+2;
	.loc 1 3 0
	# int a=2+3;
	movslq -8(%rbp), %rax
	push %rax
	.loc 1 9 0
	# return a+2;
	mov $2, %rax
	mov %rax, %rcx
	pop %rax
	add %rcx, %rax
	cltq
	leave
	ret
	leave
	ret
	.text
.global main
main:
	nop
	push %rbp
	mov %rsp, %rbp
	sub $16, %rsp