mysecond:
.LFB12:
        .cfi_startproc
        subq    $40, %rsp       #,
        .cfi_def_cfa_offset 48
        leaq    16(%rsp), %rdi  #, tmp95
        movq    %rsp, %rsi      #,
        call    gettimeofday    #
        vxorpd  %xmm0, %xmm0, %xmm0     # D.2838
        vxorpd  %xmm1, %xmm1, %xmm1     # D.2838
        vcvtsi2sdq      24(%rsp), %xmm0, %xmm0  # tp.tv_usec, D.2838, D.2838
        vcvtsi2sdq      16(%rsp), %xmm1, %xmm1  # tp.tv_sec, D.2838, D.2838
        vfmadd132sd     .LC0(%rip), %xmm1, %xmm0        #, D.2838, D.2838
        addq    $40, %rsp       #,
        .cfi_def_cfa_offset 8
        ret
        .cfi_endproc

.
.
.

main:
.LFB11:
        .cfi_startproc
        subq    $24, %rsp       #,
        .cfi_def_cfa_offset 32
        xorl    %eax, %eax      #
        call    mysecond        #
        xorl    %eax, %eax      #
        vmovsd  %xmm0, 8(%rsp)  #, %sfp
        call    mysecond        #
        vsubsd  8(%rsp), %xmm0, %xmm0   # %sfp, t2, D.2841
        movl    $.LC2, %edi     #,
        movl    $1, %eax        #,
        call    printf  #
        xorl    %eax, %eax      #
        addq    $24, %rsp       #,
        .cfi_def_cfa_offset 8
        ret
        .cfi_endproc
