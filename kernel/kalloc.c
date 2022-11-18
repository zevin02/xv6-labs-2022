// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct
{
  struct spinlock lock;
  int refcnt[PHYSTOP / PGSIZE];
} ref;
void kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    ref.refcnt[(uint64)p / PGSIZE] = 1;//因为下面调用kfree要把每个物理地址上的引用计数都减少1,为0才能够释放空间，所以这里我们先给每个初始化成1,保证能够释放空间成功
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  struct run *r;
  acquire(&ref.lock);
  --ref.refcnt[(uint64)pa / PGSIZE];
  release(&ref.lock);
  // Fill with junk to catch dangling refs.
  if (ref.refcnt[(uint64)pa / PGSIZE] == 0)
  {
    // release(&ref.lock);
    memset(pa, 1, PGSIZE);//当引用计数为0的时候，才把这个空间释放，同时添加到空闲链表里面
    r = (struct run *)pa;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  // else
  // {
  //   release(&ref.lock);
  // }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
  {
    kmem.freelist = r->next;
    release(&kmem.lock);
    acquire(&ref.lock);
    ref.refcnt[(uint64)r / PGSIZE] = 1;
    release(&ref.lock);
  }

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
// 1为cow页，0为错误,其他的page fault我们现在还处理不了
int iscow(pagetable_t pagetable, uint64 va)
{
  if (va > MAXVA)
    return 0;
  pte_t *pte = walk(pagetable, va, 0);
  if (pte == 0)
    return 0;
  if ((*pte & PTE_V) == 0)
    return 0;
  if ((*pte & PTE_U) == 0)
    return 0;
  if (*pte & PTE_COW)
    return 1;
  else
    return 0;
}

int cowalloc(pagetable_t pagetable, uint64 va)//为page fault的虚拟地址进行拷贝新的物理地址，内容从父进程里面全部拷贝过来
{
  if(va>MAXVA)
  return 0;
  pte_t *pte = walk(pagetable, va, 0);
  if (pte == 0)
    return 0;
  if ((*pte & PTE_V) == 0)
    return 0;
  if ((*pte & PTE_U) == 0)
    return 0;
  //这个函数就是用来进行分配物理空间的
  uint64 pa = PTE2PA(*pte);
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)//所有的物理地址大小都是4096字节，对齐，end是内核物理地址的最底段，PHYSTOP是内核物理地址的最顶端
    panic("cowalloc");
  uint64 ka = (uint64)kalloc();//引用计数初始化
  if (ka == 0)//物理内存已经满了，这里我们采取简单的方法，直接将这个进程给杀掉,但是实际上在课上讲过，可以使用LRU的方法，把最近一直没有使用的页表给释放出来，然后新的进程去使用这个页表，可以提高效率
  {
    return 0;
  }
  memmove((void *)ka, (void *)pa, PGSIZE);//把他原来对应物理内存的地址进行拷贝过来，都是4096字节
  *pte &= (~PTE_COW);//取消他的cow标志位
  *pte |= PTE_W;//添加写权限
  // *pte|=(PTE_V);
  uint flag = PTE_FLAGS(*pte);
  uvmunmap(pagetable, va, 1, 1);//这个地方因为是取消映射，也就是之前映射对应的物理地址对应的引用计数要减1
  if (mappages(pagetable, va, PGSIZE, ka, flag) != 0)//进行新的映射
  {
    //映射失败，同时页需要减少引用计数
    kfree((void*)ka);
    // *pte&=(~PTE_V);//添加这个有效的标志位
    uvmunmap(pagetable,va,1,1);
    return 0;
  }
  return 1;
}

void refadd(uint64 pa)//添加引用计数
{
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("refadd");
  acquire(&ref.lock);//添加的时候要上锁，避免出现多线程同时操作同一个数的情况 
  ref.refcnt[pa / PGSIZE]++;
  release(&ref.lock);
}