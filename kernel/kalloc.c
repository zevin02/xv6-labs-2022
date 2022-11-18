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
    ref.refcnt[(uint64)p / PGSIZE] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  acquire(&ref.lock);
  // Fill with junk to catch dangling refs.
  if (--ref.refcnt[(uint64)pa / PGSIZE] == 0)
  {
    release(&ref.lock);
    memset(pa, 1, PGSIZE);
    r = (struct run *)pa;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  else
  {
    release(&ref.lock);
  }
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

int cowalloc(pagetable_t pagetable, uint64 va)
{
  pte_t *pte = walk(pagetable, va, 0);
  if (pte == 0)
    return 0;
  if ((*pte & PTE_V) == 0)
    return 0;
  if ((*pte & PTE_U) == 0)
    return 0;
  //这个函数就是用来进行分配物理空间的
  uint64 pa = PTE2PA(*pte);
  uint64 ka = (uint64)kalloc();
  if (ka == 0)
  {
    return 0;
  }
  memmove((void *)ka, (void *)pa, PGSIZE);
  *pte &= (~PTE_COW);
  *pte |= PTE_W;
  uint flag = PTE_FLAGS(*pte);
  uvmunmap(pagetable, va, 1, 1);
  if (mappages(pagetable, va, PGSIZE, ka, flag) != 0)
  {
    return 0;
  }
  return 1;
}

void refadd(uint64 pa)
{
  acquire(&ref.lock);
  ref.refcnt[pa / PGSIZE]++;
  release(&ref.lock);
}