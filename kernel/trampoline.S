        #
        # low-level code to handle traps from user space into
        # the kernel, and returns from kernel to user.
        #
        # the kernel maps the page holding this code
        # at the same virtual address (TRAMPOLINE)
        # in user and kernel space so that it continues
        # to work when it switches page tables.
        # kernel.ld causes this code to start at 
        # a page boundary.
        #

#include "riscv.h"
#include "memlayout.h"

.section trampsec
.globl trampoline
trampoline:
.align 4
.globl uservec
# 调用ecall之后立刻就到达了这里
uservec:    
	#
        # trap.c sets stvec to point here, so
        # traps from user space start here,
        # in supervisor mode, but with a
        # user page table.
        #

        # save user a0 in sscratch so
        # a0 can be used to get at TRAPFRAME.
        # 1.
        # 这里的uservec启动的时候，所有的32个寄存器都包含了中断代码所拥有的值，但是uservec需要能够修改一些寄存器的值
        # 能够设置satp并生成保存寄存器的值，
        
        # ssratch：内核在这里设置了一个值，在陷阱处理程序的一开始就会派上用场
        # csrw交换了sscratch和a0的值，用户代码发送的参数a0就被保存在ssratch里面了
        # uservec有一个寄存器可以使用了（a0）
        # a0保存sratch以前的值
        # 内核前一次切换回到用户空间的时候，内核会执行set sscratch指令，将这个寄存器的值设置为trapframe page 的虚拟地址
        # 可能会有一个问题：trapoline的值一直都是不变的，为什么还要保存在sscratch这个寄存器里面
        # 答案是：存储在寄存器里面速度快，还不用害怕被覆盖
        csrw sscratch, a0
        # 在交换a0和sscratch之后，a0现在持有当前进程陷阱帧的指针，uservec保存那里的所有用户寄存器，包括从sscratch读取的用户a0

        # each process has a separate p->trapframe memory area,
        # but it's mapped to the same virtual address
        # (TRAPFRAME) in every process's user page table.
        li a0, TRAPFRAME
        
        # save the user registers in TRAPFRAME
        # 2.
        # 保存用户寄存器，只需要将satp设置为内核页表，然后我们就可以使用所有的内核映射，s模式下是可以修改satp值
        # 这些指令将每个寄存器报春在trapframe的不同偏移位置，因为a0交换后，a0现在指向的就是trapframe page地址，所以每个寄存器都被保存在了偏移两+a0的位置
        # 这个寄存器存在CPU上，是一个特殊的寄存器

        # 我们把寄存器存放在trapframe里面，而不放在用户程序的栈
        # 因为我们不确定程序是否有栈，肯定有一些编程语言没有栈，对于这些编程语言程序，sp不指向任何地址
        # 所以要保存在内核中的trapframe里面，而不是用户内存
        # sd是把寄存器的值存到内存里面
        sd ra, 40(a0)
        sd sp, 48(a0)
        sd gp, 56(a0)
        sd tp, 64(a0)
        sd t0, 72(a0)
        sd t1, 80(a0)
        sd t2, 88(a0)
        sd s0, 96(a0)
        sd s1, 104(a0)
        sd a1, 120(a0)
        sd a2, 128(a0)
        sd a3, 136(a0)
        sd a4, 144(a0)
        sd a5, 152(a0)
        sd a6, 160(a0)
        sd a7, 168(a0)
        sd s2, 176(a0)
        sd s3, 184(a0)
        sd s4, 192(a0)
        sd s5, 200(a0)
        sd s6, 208(a0)
        sd s7, 216(a0)
        sd s8, 224(a0)
        sd s9, 232(a0)
        sd s10, 240(a0)
        sd s11, 248(a0)
        sd t3, 256(a0)
        sd t4, 264(a0)
        sd t5, 272(a0)
        sd t6, 280(a0)

	# save the user a0 in p->trapframe->a0
        csrr t0, sscratch
        sd t0, 112(a0)

        # initialize kernel stack pointer, from p->trapframe->kernel_sp
        # 初始化内核栈指针，从陷阱帧中的进程中的内核栈的顶部

        # 将a0后面的8个地址内存中的值放到sp寄存器里面，因为a0是trapframe的地址
        ld sp, 8(a0)
        # 这个执行完之后sp就是进程的kernel stack，因为xv6在每个内核栈下面放了guard page

        # make tp hold the current hartid, from p->trapframe->kernel_hartid
        # 向tp寄存器里面写入数据，因为在risc-V里面，没有一个直接的方法来确认运行在哪个核上，xv6会将CPU核的编号
        # 也就是hartid，放在tp上面，内核可以通过这个值来确认某个CPU核上运行的哪些进程
        # 如果打印tp值=0,说明我们在0核上面
        ld tp, 32(a0)

        # load the address of usertrap(), from p->trapframe->kernel_trap
        # t0里面就是trap的函数指针，我们使用这个指针就可以调用到这个函数了
        ld t0, 16(a0)


        # fetch the kernel page table address, from p->trapframe->kernel_satp.
        # 把satp里面的值放到t1里面，获得内核页表
        ld t1, 0(a0)

        # wait for any previous memory operations to complete, so that
        # they use the user page table.
        sfence.vma zero, zero

        # install the kernel page table.
        # 交换satp和t1的值，执行完这些之后，就从user page table转移到了kernel page table
        # satp里面的就是可以指向内核页表
        csrw satp, t1

        # 走到这里我们发现代码没有崩溃，pc里面保存vm，我们这里切换了page table，为什么同一个虚拟地址不会走到一些无关的page中
        # 因为我们还在trapoline里面，而trampoline代码在用户空间和内核空间都映射到了同一个地址
        # 
        # flush now-stale user entries from the TLB.
        sfence.vma zero, zero

        # jump to usertrap(), which does not return
        # 进入到usertrap函数里面，这个函数是不需要返回的，因为t0现在就是usertrap的地址了
        jr t0

# 最后的返回用户空间，后回到ecall进行恢复
.globl userret
userret:
        # 代码从内核返回到用户空间
        # userret(pagetable)
        # called by usertrapret() in trap.c to
        # switch from kernel to user.
        # a0: user page table, for satp.

        # switch to the user page table.
        sfence.vma zero, zero
        csrw satp, a0
        sfence.vma zero, zero

        li a0, TRAPFRAME

        # restore all but a0 from TRAPFRAME
        ld ra, 40(a0)
        ld sp, 48(a0)
        ld gp, 56(a0)
        ld tp, 64(a0)
        ld t0, 72(a0)
        ld t1, 80(a0)
        ld t2, 88(a0)
        ld s0, 96(a0)
        ld s1, 104(a0)
        ld a1, 120(a0)
        ld a2, 128(a0)
        ld a3, 136(a0)
        ld a4, 144(a0)
        ld a5, 152(a0)
        ld a6, 160(a0)
        ld a7, 168(a0)
        ld s2, 176(a0)
        ld s3, 184(a0)
        ld s4, 192(a0)
        ld s5, 200(a0)
        ld s6, 208(a0)
        ld s7, 216(a0)
        ld s8, 224(a0)
        ld s9, 232(a0)
        ld s10, 240(a0)
        ld s11, 248(a0)
        ld t3, 256(a0)
        ld t4, 264(a0)
        ld t5, 272(a0)
        ld t6, 280(a0)

	# restore user a0
        
        ld a0, 112(a0)
        
        # 返回时候，恢复所有的用户寄存器
        # 恢复之后ssratch里面保存trapoline的值，a0里面也恢复成之前的值了

        # return to user mode and user pc.
        # usertrapret() set up sstatus and sepc.
        # 返回到用户空间里面
        sret
