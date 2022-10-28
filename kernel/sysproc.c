#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

#define LAB_PGTBL 1
#ifdef LAB_PGTBL
int sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  // if (pgaccess(buf, 32, &abits) < 0)

  int len;         //
  argint(1, &len); //获取参数
  uint64 base;
  argaddr(0, &base);
  int mask; //返回给用户的掩码
  argint(2, &mask);

  // walk()，遍历，查看谁被使用了
  for(int i=0;i<len;i++)
  {
    pte_t *pte = walk(myproc()->pagetable, base+i*PGSIZE, 0);
    if (*pte &PTE_A)
    {
      int l=0;
      while(l<32)
      {
        if((mask&(1<<i))==0)
        {
          mask&=(1<<i);
          break;
        }
      }
      *pte&=(~PTE_A);
    }
  }

  // copyout to usersppace
  if(copyout(myproc()->pagetable,base,(char*)&mask,sizeof(mask))<0)
  {
    return -1;
  }
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
