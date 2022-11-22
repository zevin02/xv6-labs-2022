// Sleeping locks

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"

//xv6有时候要长时间保持锁，但是如果长时间保持自旋锁，会导致再自旋的时候，浪费很多时间，
//自旋锁缺点：无法让出CPU（yield），但是我们希望这个持有锁的进程等待磁盘IO的时候，，其他进程也可以使用这个CPU
//持有自旋锁时让出CPU是非法的，


void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

//中断锁会保持中断的性能，
//自旋锁适合临界区短的区域，睡眠锁适合临界区长的区域

void
acquiresleep(struct sleeplock *lk)//睡眠锁，在等待的时候会让出CPU
{
  acquire(&lk->lk);//这个地方有一一个被自旋锁保护的字段
  while (lk->locked) {
    sleep(lk, &lk->lk);//这个地方调用了这个东西，在休眠的时候就直接把锁给释放掉了，同时还可以让出CPU，其他线程可以在这个线程等待的时候，获得这个CPU操作
  }
  lk->locked = 1;//
  lk->pid = myproc()->pid;
  release(&lk->lk);
}

void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  lk->pid = 0;
  wakeup(lk);
  release(&lk->lk);
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk);
  r = lk->locked && (lk->pid == myproc()->pid);
  release(&lk->lk);
  return r;
}



