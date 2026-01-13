    .text
    .globl _main
_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $32, %rsp
    movl $5, -4(%rbp)
    cmpl $0, -4(%rbp)
    sete %al
    movzbl %al, %r10d
    movl %r10d, -4(%rbp)
    movl $1, -8(%rbp)
    notl -8(%rbp)
    movl -4(%rbp), %r10d
    movl %r10d, -12(%rbp)
    movl -8(%rbp), %r10d
    addl %r10d, -12(%rbp)
    movl $3, -16(%rbp)
    negl -16(%rbp)
    movl -12(%rbp), %r10d
    movl %r10d, -20(%rbp)
    movl -16(%rbp), %r10d
    addl %r10d, -20(%rbp)
    movl $5, -24(%rbp)
    movl -24(%rbp), %eax
    cdq
    movl $2, %r10d
    idivl %r10d
    movl %edx, -24(%rbp)
    movl -24(%rbp), %r10d
    movl %r10d, -28(%rbp)
    movl -28(%rbp), %r10d
    movl $2, %eax
    imull %eax, %r10d
    movl %r10d, -28(%rbp)
    movl -20(%rbp), %r10d
    movl %r10d, -32(%rbp)
    movl -28(%rbp), %r10d
    addl %r10d, -32(%rbp)
    movl -32(%rbp), %eax
    movq %rbp, %rsp
    popq %rbp
    ret
