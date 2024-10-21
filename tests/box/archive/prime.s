	.text
	.file	"main.gsc"
	.globl	main
	.p2align	4, 0x90
	.type	main,@function
main:
	.cfi_startproc
	pushq	%rbx
	.cfi_def_cfa_offset 16
	subq	$16, %rsp
	.cfi_def_cfa_offset 32
	.cfi_offset %rbx, -16
	movl	$64, %edi
	callq	"std::mem::alloc"@PLT
	movq	%rax, (%rsp)
	movq	%rax, 8(%rsp)
	movq	%rsp, %rbx
	movq	%rbx, %rdi
	movl	$32, %esi
	callq	"std::mem::realloc"@PLT
	movq	96e28277@GOTPCREL(%rip), %rdi
	movl	$13, %esi
	callq	"std::io::print"@PLT
	movq	%rbx, %rdi
	callq	"std::mem::free"@PLT
	addq	$16, %rsp
	.cfi_def_cfa_offset 16
	popq	%rbx
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc

	.type	96e28277,@object
	.section	.rodata.str1.1,"aMS",@progbits,1
	.globl	96e28277
96e28277:
	.asciz	"Hello, World\n"
	.size	96e28277, 14

	.section	".note.GNU-stack","",@progbits
