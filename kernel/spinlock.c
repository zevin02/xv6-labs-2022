// Mutual exclusion spin locks.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"
//spinlock需要处理两类并发
//* 不同CPU之间的并发
//* 中断和普通程序之间的并发，在获取锁的时候关闭中断，在释放锁的时候打开中断，避免出现死锁的问题

void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void
acquire(struct spinlock *lk)
{
  push_off(); // disable interrupts to avoid deadlock.关闭中断，避免死锁，调用这个来跟踪当前CPU上锁的嵌套级别
  if(holding(lk))//检测这个CPU是否造就已经拥有这个锁了，如果这个锁已经有了，直接就挂掉了
    panic("acquire");
  
  //先把这个中断关闭再获得锁才可以，不然就会出现一个又拥有锁，又可以启动中断的短暂区域，这个短暂的时间点有可能会出现中断发生，直接就出现死锁了

  // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
  //   a5 = 1
  //   s1 = &lk->locked
  //   amoswap.w.aq a5, a5, (s1)
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0)//如果锁没有被持有时，lk->locked=0，这个是一个原子的内存交换指令
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen strictly after the lock is acquired.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();//内存保障fence,这个地方是为了避免编译器的优化，编译器的优化可能会移动指令的位置，对于普通程序没有什么影响，但是对于并发程序，就是灾难，因为我们在锁之间的就是我们需要保护的临界区，
  //如果指令被移动了位置，就达不到并发的效果了
  //任何在他之前的lord/store指令都不能移动到他后面
  

  // Record info about lock acquisition for holding() and debugging.
  lk->cpu = mycpu();
}

// Release the lock.
void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->cpu = 0;

  // Tell the C compiler and the CPU to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released,
  // and that loads in the critical section occur strictly before
  // the lock is released.
  // On RISC-V, this emits a fence instruction.
  __sync_synchronize();//在这个点之前执行的命令不允许被放到这个点之后去运行，我们这个地方设置了一个内存屏蔽，防止我们需要在临界区的代码逃离临界区

  // Release the lock, equivalent to lk->locked = 0.
  // This code doesn't use a C assignment, since the C standard
  // implies that an assignment might be implemented with
  // multiple store instructions.
  // On RISC-V, sync_lock_release turns into an atomic swap:
  //   s1 = &lk->locked
  //   amoswap.w zero, zero, (s1)
  __sync_lock_release(&lk->locked); 
  //为什么不能使用store指令将locked写为0,因为其他处理其可能向locked字段里面写入0/1,这个指令并不是一个原子的指令

  pop_off();//在解锁的时候重新把中断打开
  //在释放锁之后再，release再调用这个popoff很重要
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int
holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.

//这些都是用来处理嵌套的临界区域，
void
push_off(void)//中断关闭
{
  int old = intr_get();

  intr_off();
  if(mycpu()->noff == 0)
    mycpu()->intena = old;
  mycpu()->noff += 1;
}

void
pop_off(void)//中断打开
{
  struct cpu *c = mycpu();
  if(intr_get())
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  c->noff -= 1;//
  if(c->noff == 0 && c->intena)//如果嵌套级别为0的时候，就直接把中断打开即可，
    intr_on();//中断打开，都是使用SIE，使用汇编指令来打开中断，sstatus里面都是CPU的一些状态
}
