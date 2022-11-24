# Context switch
#
#   void swtch(struct context *old, struct context *new);
# 
# Save current registers in old. Load from new.	
# 保存寄存器，加载寄存器
# 这里保存了很多s寄存器，但是没有保存pc，因为这个没什么用，我们切换到另一个进程，获得了对方的ra
# 可以直接从ra返回，到调用点进行执行

# risc-v 有32个寄存器，但是这里只保存和和恢复了14个寄存器
# 这里因为调用者保存寄存器都在栈上，而这里swtch函数只需要保存c编译器不会保存，但是又有用的寄存器，所以这里我们只需要保存callee saved即可
# swtch函数只能使用汇编代码，因为c语言没办法修改寄存器，很难完成寄存器的存储和加载，用c中嵌套汇编也可以，但是，我们这里直接使用汇编很方便


.globl swtch
swtch:
        sd ra, 0(a0) # 这个地方的a0就是进程的上下文,保存进程当前的信息到该进程的上下文中
        sd sp, 8(a0) # 使用sd保存在内存里面
        sd s0, 16(a0)
        sd s1, 24(a0)
        sd s2, 32(a0)
        sd s3, 40(a0)
        sd s4, 48(a0)
        sd s5, 56(a0)
        sd s6, 64(a0)
        sd s7, 72(a0)
        sd s8, 80(a0)
        sd s9, 88(a0)
        sd s10, 96(a0)
        sd s11, 104(a0)

        ld ra, 0(a1) # a1里面就是我们需要切换的那个上下文的内容，我们把内容取出来
        ld sp, 8(a1)
        ld s0, 16(a1)
        ld s1, 24(a1)
        ld s2, 32(a1)
        ld s3, 40(a1)
        ld s4, 48(a1)
        ld s5, 56(a1)
        ld s6, 64(a1)
        ld s7, 72(a1)
        ld s8, 80(a1)
        ld s9, 88(a1)
        ld s10, 96(a1)
        ld s11, 104(a1)
        # 从目标的上下文返回，走到这里，就恢复了之前调度期线程保存的寄存器集
        ret             # 返回到函数调用的地方，也就是ra的地址

	
