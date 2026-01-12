    .text
    .globl _main
_main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp
    movl $5, -4(%rbp)
    movl -4(%rbp), %eax
    cdq
    movl $2, %r10d
    idivl %r10d
    movl %edx, -4(%rbp)
    movl -4(%rbp), %eax
    movq %rbp, %rsp
    popq %rbp
    ret
