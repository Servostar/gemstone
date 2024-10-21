	.text
	.file	"std.gsc"
	.globl	"std::mem::fill"
	.p2align	4, 0x90
	.type	"std::mem::fill",@function
"std::mem::fill":
	.cfi_startproc
	movl	$0, -4(%rsp)
	cmpl	%edx, -4(%rsp)
	jge	.LBB0_3
	.p2align	4, 0x90
.LBB0_2:
	movslq	-4(%rsp), %rax
	movb	%sil, (%rdi,%rax)
	incl	-4(%rsp)
	cmpl	%edx, -4(%rsp)
	jl	.LBB0_2
.LBB0_3:
	retq
.Lfunc_end0:
	.size	"std::mem::fill", .Lfunc_end0-"std::mem::fill"
	.cfi_endproc

	.globl	_start
	.p2align	4, 0x90
	.type	_start,@function
_start:
	.cfi_startproc
	pushq	%rax
	.cfi_def_cfa_offset 16
	callq	main@PLT
	xorl	%edi, %edi
	callq	"std::os::exit"@PLT
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end1:
	.size	_start, .Lfunc_end1-_start
	.cfi_endproc

	.globl	"std::io::print"
	.p2align	4, 0x90
	.type	"std::io::print",@function
"std::io::print":
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r14
	pushq	%rbx
	.cfi_offset %rbx, -32
	.cfi_offset %r14, -24
	movl	%esi, %r14d
	movq	%rdi, %rbx
	callq	"std::io::getStdoutHandle"@PLT
	movq	%rsp, %rcx
	leaq	-16(%rcx), %rsp
	movl	%eax, -16(%rcx)
	movl	%r14d, %edx
	movl	$1, %edi
	movq	%rbx, %rsi
	callq	write@PLT
	leaq	-16(%rbp), %rsp
	popq	%rbx
	popq	%r14
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	.cfi_restore %rbx
	.cfi_restore %r14
	.cfi_restore %rbp
	retq
.Lfunc_end2:
	.size	"std::io::print", .Lfunc_end2-"std::io::print"
	.cfi_endproc

	.globl	"std::mem::realloc"
	.p2align	4, 0x90
	.type	"std::mem::realloc",@function
"std::mem::realloc":
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	pushq	%rax
	.cfi_def_cfa_offset 32
	.cfi_offset %rbx, -24
	.cfi_offset %rbp, -16
	movl	%esi, %ebp
	movq	%rdi, %rbx
	movq	(%rdi), %rdi
	callq	realloc@PLT
	movq	%rax, (%rbx)
	movq	%rax, %rdi
	xorl	%esi, %esi
	movl	%ebp, %edx
	callq	"std::mem::fill"@PLT
	movq	(%rbx), %rax
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end3:
	.size	"std::mem::realloc", .Lfunc_end3-"std::mem::realloc"
	.cfi_endproc

	.globl	"std::io::getStdoutHandle"
	.p2align	4, 0x90
	.type	"std::io::getStdoutHandle",@function
"std::io::getStdoutHandle":
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	movq	%rsp, %rax
	leaq	-16(%rax), %rsp
	movl	$0, -16(%rax)
	xorl	%eax, %eax
	movq	%rbp, %rsp
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end4:
	.size	"std::io::getStdoutHandle", .Lfunc_end4-"std::io::getStdoutHandle"
	.cfi_endproc

	.globl	"std::os::exit"
	.p2align	4, 0x90
	.type	"std::os::exit",@function
"std::os::exit":
	.cfi_startproc
	pushq	%rax
	.cfi_def_cfa_offset 16
	callq	_exit@PLT
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end5:
	.size	"std::os::exit", .Lfunc_end5-"std::os::exit"
	.cfi_endproc

	.globl	"std::mem::alloc"
	.p2align	4, 0x90
	.type	"std::mem::alloc",@function
"std::mem::alloc":
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	pushq	%r14
	pushq	%rbx
	.cfi_offset %rbx, -32
	.cfi_offset %r14, -24
	movl	%edi, %ebx
	callq	malloc@PLT
	movq	%rsp, %r14
	leaq	-16(%r14), %rsp
	movq	%rax, -16(%r14)
	movq	%rax, %rdi
	xorl	%esi, %esi
	movl	%ebx, %edx
	callq	"std::mem::fill"@PLT
	movq	-16(%r14), %rax
	leaq	-16(%rbp), %rsp
	popq	%rbx
	popq	%r14
	popq	%rbp
	.cfi_def_cfa %rsp, 8
	retq
.Lfunc_end6:
	.size	"std::mem::alloc", .Lfunc_end6-"std::mem::alloc"
	.cfi_endproc

	.globl	"std::mem::free"
	.p2align	4, 0x90
	.type	"std::mem::free",@function
"std::mem::free":
	.cfi_startproc
	pushq	%rbx
	.cfi_def_cfa_offset 16
	.cfi_offset %rbx, -16
	movq	%rdi, %rbx
	movq	(%rdi), %rdi
	callq	free@PLT
	movq	$0, (%rbx)
	popq	%rbx
	.cfi_def_cfa_offset 8
	.cfi_restore %rbx
	retq
.Lfunc_end7:
	.size	"std::mem::free", .Lfunc_end7-"std::mem::free"
	.cfi_endproc

	.section	".note.GNU-stack","",@progbits
