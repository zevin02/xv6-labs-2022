// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"


//bcache这个缓冲区就是用链表进行串联，每次插入都是头插，头后面的数据都是最近使用的，越后面的数据都是最少使用的
struct {
  struct spinlock lock;
  struct buf buf[NBUF];//保存磁盘块的缓冲区是一定的

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;//这个地方的bcache是一个带头循环双向链表，链表的每个元素就是一个buf

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    //对链表进行头插
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  //blcokno里面就是block块的number编号
  struct buf *b;
  //这里的bache就是一个大的缓存区链条
  acquire(&bcache.lock);//获取这个bufer cache的大锁。这是一个粗力度的大锁
  
  //这样没有人可以在特定的时间内修改缓冲区的缓存

  //先检查我们这里要找的blockno的编号是否已经存在在缓存里面了，buffer cache里面

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){//如果这个blockno已经在缓存里面存在了，
      b->refcnt++;//这里我们就把引用计数进行增加即可，不需要进行创建了，因为我们知道这个blockno已经在buff cache里面了 
      release(&bcache.lock);//执行完之后就要把锁释放掉

      acquiresleep(&b->lock);//每个缓冲块都有一个sleep lock的锁
      //所有对该block 编号处理的时候，都需要获得该block number对应的sleeplock，这个地方他确保了每次只有一个进程
      //可以修改该block对应的数据
      //因为上面释放掉了bcache的大锁，所以可能两个进程同时去索要sleep lock
      return b;//直接返回，其中一个进程就会直接放回，而另一个进程就会在acquieresleep里面一直不断的等待，直到他释放完锁
    }
  }
  //走到这里说明，在缓存里面没有这个block的inode数据，说明我们并没有在创建过这个
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0) {//我们只需要查找对应blcok的refcnt=0的，并填写入数据
    //当我们再缓冲区里面找不到数据，我们就需要一些东西来腾出空间，我们从最近使用的块进行往前遍历
    //这样可以满足局部性原理
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock);
      acquiresleep(&b->lock);//这里同样也是在返回的时候，返回一个带头的节点
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)//这个read是从内存中获得一个buffer，
{
  struct buf *b;
  //bget就是在缓冲区高速缓存获取一个插槽
  b = bget(dev, blockno);//bget函数会为我们在buffer cache里面找到block的缓存，这个地方就获取了对应的缓存信息
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;//返回一个上锁的缓冲区
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)//将修改后的缓冲区写入到磁盘的相应块里面
{
  //文件系统中的所有的bwrite都不能直接使用，所有的bwrite都被用log_write进行替代
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  //
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);//这个地方首先先释放掉sleep 

  acquire(&bcache.lock);//之后重新获取了bcache的锁
  b->refcnt--;//引用计数减少1,如果当这个引用计数=0的时候，我们就需要把这个buffer给释放掉
  //表示一个进程对这个缓冲区不再感兴趣了

  if (b->refcnt == 0) {
    // no one is waiting for it.
    //修改buffer cache的链表
    //将这个buffer移动到最近使用过的块缓冲区里面
    b->next->prev = b->prev;//我们将blcok cache以用到linked-list的头部去
    b->prev->next = b->next;//
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
    //这样表明这个block cache是我们最近使用过的block cache，我们需要在buffer cache里面腾出空间来存放新的block cache
    //这个时候就会使用LRU算法，找到最不常用的blcok cache，撤回他
  }
  
  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;//这样就可以避免brelse测绘之前的block
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


