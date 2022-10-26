// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.
//物理内存分配

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;//自旋锁
  struct run *freelist;//空闲页的列表元素是一个struct run
} kmem;

void
kinit()//初始化空闲列表，保存从内核结束到PHYSTOP中的每一页，
{
  initlock(&kmem.lock, "kmem");//初始化锁
  freerange(end, (void*)PHYSTOP);//将内存添加到闲散列表中，
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);//每页都调用这个
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)//分配一些管理空间
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);//

  r = (struct run*)pa;//把地址视为指针。以便操作存储在每个页面中的run结构

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;//添加闲散的数据
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.

//分配一个4页大小给物理地址
void *
kalloc(void)//删除并返回空闲列表中的第一个元素
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;//kem里面串联的就是空闲的空间，
    //如果r存在，说明有空间，那么kem就要往后走，前面的都是用过的，后面的都是空闲的
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
