#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
// usertrap任务是确认陷阱的原因，处理之后并返回，
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);//他首先改变stvec（内核在这里写入其陷阱处理程序的地址），这样内核中的陷阱将有kernelvec来处理
  // 他做的第一件事就是更改stvec寄存器，取决于trap来自用户空间还是内核空间，在内核执行开始之前，usertrap先将stvec指向kernelvec
  // 这是内核处理trap代码的地方，而不是用户空间处理代码的位置

  struct proc *p = myproc();//我们需要知道我们现在在运行什么进程，这个函数实际会查找当前CPU核的编号索引的数组，CPU核的编号是hartid
  //我们已经把这个值存在了tp寄存器
  // save user program counter.
  p->trapframe->epc = r_sepc();//保存sepc（保存用户的的pc程序计数器），再次保存是因为usertrap可能会有一个进程切换，怕sepc被覆盖
  //sepc始终保存的就是pc，但是可能程序在运行的时候，切换进程，进入那个程序的用户空间，然后那个进程可能再次调用系统调用导致sepc被覆盖
  //所以我们需要保存当前进程的sepc，到和该进程相关内存里面，
  //查找trap原因
  if(r_scause() == 8){
    // system call
    //如果是陷阱的原因是系统调用

    if(killed(p))//如果这个进程已经被杀死了，就不要处理
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;//因为epc里面指向的ecall的指令地址，+4是因为在系统调用的情况下，risc-v会留下指向ecall指令的程序指针，返回后需要执行ecall的下一条指令，

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();//允许设备中断，xv6在处理系统调用的时候才会中断，这样中断效率高，但是有些系统调用处理时间长，中断总是会被trap的硬件关闭
    //所以我们需要显示的打开中断

    syscall();//执行系统调用
  } else if((which_dev = devintr()) != 0){
    // ok
    //设备中断，这里会处理

  } else {
    //否则就是一个异常了，内核会杀杀死错误的进程
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    setkilled(p);
  }
  //退出时，usertrap需要检查进程是已经杀死了还是应该要让出CPU
  //因为我们不想恢复一个被杀掉的进程，因为可能在系统调用执行的时候程序就已经崩了，就没必要再执行这个程序了
  if(killed(p))
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  //如果这个陷阱是计时器中断
  if(which_dev == 2)
    yield();
  
  //返回用户空间的第一部就是调用这个函数，
  usertrapret();
}

//
// return to user space
//该函数设置risc-v控制寄存器，为将来来自用户空间的陷阱作准备，这是返回到用户空间的最后一个C函数
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();//关闭了中断，因为我们在系统调用的时候是把中断打开的
  //这里关闭中断因为我们要更新stvec来指向用户空间的trap代码
  //这时我们仍然在内核执行代码。如果这时发生了一个中断，那么程序就会走到用户空间的trap代码，即使我们还在内核中

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline); 
  w_stvec(trampoline_uservec);//这里我们设置了stvec指向trampoline代码，在那里会执行sret返回到用户空间
  //所以在我们执行代码的时候中断总是开始的


  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  //下面填入了trapframe的内容
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.

  //sstatus寄存器，这是一个控制寄存器，其中的SIE位控制设备中断是否启用，如果内核清空SIE，risc-v就会推迟设备中断，直到内核重新设置SIE
  //SPP indicate that whether trap come frome user mode or supervisor mode
  //SPP bit =0 means sret :return to user mode instead of s mode
  // SPIE bit control that after execuate the sret instruction whether or not open the interrupt

  unsigned long x = r_sstatus();
  //modify the value
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode,1 means open
  //having modified the args of the sstatus
  w_sstatus(x);//rewrite the  new value to sstatus

  // set S Exception Program Counter to the saved user pc.
  // restore to the previous pc
  w_sepc(p->trapframe->epc);//sepc store the user addr when trap to kernel

  // tell trampoline.S the user page table to switch to.
  // make satp base on user page table addr,然后我们会在返回到用户空间的时候完成页表的切换
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);//计算出我们要跳转到的汇编代码的地址我们希望跳转的是userret
  ((void (*)(uint64))trampoline_userret)(satp);//调用userret，在这里调用函数，这里无论调用什么参数，这里都是trapframe和用户页表，这里的第一个参数就进入到a0里面
  // userytrap在用户和内核页标中都映射的蹦床页面上调用userret，userret中的汇编代码会切换页表，

}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    //
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();//调用这个给其他线程一个机会，在某个时刻，其中一个线程就会让不，让我们的线程和他的kerneltrap再次恢复

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if(cpuid() == 0){
      clockintr();
    }
    
    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

