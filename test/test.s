    .text
    .globl main
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $64, %rsp
    movl $5, -4(%rbp)
    movl $3, -8(%rbp)
    movl -4(%rbp), %r10d
    movl %r10d, -12(%rbp)
    movl -12(%rbp), %r10d
    movl -8(%rbp), %eax
    imull %eax, %r10d
    movl %r10d, -12(%rbp)
    movl -4(%rbp), %r10d
    movl %r10d, -16(%rbp)
    movl -8(%rbp), %r10d
    subl %r10d, -16(%rbp)
    movl -12(%rbp), %r10d
    movl %r10d, -20(%rbp)
    movl -16(%rbp), %r10d
    addl %r10d, -20(%rbp)
    movl -20(%rbp), %r10d
    movl %r10d, -24(%rbp)
    movl -8(%rbp), %r10d
    cmpl %r10d, -4(%rbp)
    movl $0, -28(%rbp)
    setl -28(%rbp)
    cmpl $0, -28(%rbp)
    je .L0
    movl -24(%rbp), %r10d
    movl %r10d, -32(%rbp)
    addl $10, -32(%rbp)
    movl -32(%rbp), %r10d
    movl %r10d, -24(%rbp)
    jmp .L1
.L0:
    movl -24(%rbp), %r10d
    movl %r10d, -36(%rbp)
    subl $2, -36(%rbp)
    movl -36(%rbp), %r10d
    movl %r10d, -24(%rbp)
.L1:
    cmpl $10, -24(%rbp)
    movl $0, -40(%rbp)
    setg -40(%rbp)
    cmpl $0, -40(%rbp)
    je .L2
    movl -24(%rbp), %r10d
    movl %r10d, -44(%rbp)
    addl $1, -44(%rbp)
    movl -44(%rbp), %r10d
    movl %r10d, -48(%rbp)
    jmp .L3
.L2:
    movl -24(%rbp), %r10d
    movl %r10d, -52(%rbp)
    subl $1, -52(%rbp)
    movl -52(%rbp), %r10d
    movl %r10d, -48(%rbp)
.L3:
    movl -48(%rbp), %r10d
    movl %r10d, -56(%rbp)
    cmpl $0, -56(%rbp)
    movl $0, -60(%rbp)
    setne -60(%rbp)
    cmpl $0, -60(%rbp)
    je .L4
    movl -56(%rbp), %eax
    movq %rbp, %rsp
    popq %rbp
    ret
.L4:
    movl $0, %eax
    movq %rbp, %rsp
    popq %rbp
    ret
.section .note.GNU-stack,"",@progbits
