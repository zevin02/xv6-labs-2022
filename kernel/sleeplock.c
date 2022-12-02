// Sleeping locks

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

void
acquiresleep(struct sleeplock *lk)
{
  //磁盘的操作我们使用的sleep lock是因为磁盘的操作需要花费很多时间

  //block cache里面使用的是sleep lock，使用这个函数就可以获得sleep lock
  acquire(&lk->lk);//先获取一个普通的spinlock
  while (lk->locked) {//如果这个sleep lock已经被持有了
    sleep(lk, &lk->lk);//进入sleep状态，并把打开中断，释放这个自旋锁，并把自己这个进程调度走
  }
  lk->locked = 1;//走到这锁名，已经有人释放了锁，所以我们就能获取锁
  lk->pid = myproc()->pid;//同时标注是哪个进程持有的这个sleep lock
  release(&lk->lk);//释放自旋锁，因为上面的操作都必须要保证他是原子性的
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



