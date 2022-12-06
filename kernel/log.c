#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
  //这个就是
  int n;//log里面有多少需要install到对应的block块里面
  int block[LOGSIZE];//block[0]-block[n]这里面对应的值就是block number，所有block的编号都在这个内存数据中
};

struct log {
  struct spinlock lock;
  int start;//这个里面记录的就是磁盘中log的开始位置
  int size;
  int outstanding; // how many FS sys calls are executing.有多少个文件系统的系统调用正在执行
  int committing;  // in commit(), please wait.，用来sleep 和wakeup保证只有一个文件系统在执行
  int dev;
  struct logheader lh;
};
struct log log;

static void recover_from_log(void);
static void commit();
 
void
initlog(int dev, struct superblock *sb)
{
  //当crash重启，xv6做的第一件事就是调用initlog
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  initlock(&log.lock, "log");//初始化锁
  log.start = sb->logstart;//把log进行初始化
  log.size = sb->nlog;
  log.dev = dev;
  recover_from_log();
}

// Copy committed blocks from log to their home location
static void
install_trans(int recovering)
{
  //把log里面的数据添加到他真正应该存在的地方
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk，同样我们需要的是log buf里的数据，在head里面就是有他要落盘的位置
    if(recovering == 0)//已经写入到了log中
      bunpin(dbuf);//这个地方就把之前的引用给减掉，解除pin状态
    brelse(lbuf);
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header
static void
read_head(void)//从磁盘里面把数据读取到内存的log header里面
{
  struct buf *buf = bread(log.dev, log.start);//已经在磁盘上了
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(void)
{
  //真正提交事物的地方
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;//将n拷贝到block中
  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];//将所有的block编号拷贝到header列表
  }
  //如果在之前crash，不会发生什么，因为我们已经把bn0-bnn的数据写入到log磁盘上了，所以不会发生什么
  bwrite(buf);//再将header block写回到磁盘中，
  //如果这个地方crash了，会读取到磁盘上的log头，那么就会正常的install到磁盘对应的位置上了
  //所以执行完这一步，就相当于事物完成了，因为已经在磁盘上了，磁盘是持久性的
  brelse(buf);//再把buf给释放掉
}

static void
recover_from_log(void)
{
  read_head();//从磁盘里面读取header
  install_trans(1); // if committed, copy from log to disk，重新安装
  log.lh.n = 0;
  write_head(); // clear the log
}

// called at the start of each FS system call.
void
begin_op(void)
{
  acquire(&log.lock);
  while(1){
    if(log.committing){//如果正在commit，就要等log提交完，因为我们不能在install的过程中写log
    
      sleep(&log, &log.lock);
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
      //如果当前操作是允许并发操作的下一个
      //那么当前操作就可能超过log的大小，
      // this op might exhaust log space; wait for commit.
      sleep(&log, &log.lock);
    } else {
      log.outstanding += 1;
      release(&log.lock);
      break;
    }
  }
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
  int do_commit = 0;

  acquire(&log.lock);
  log.outstanding -= 1;//结束了事物，就要先对多少个文件系统调用--
  if(log.committing)//检查是否在commit状态，当前不可能在commit状态因为我们下面才commit，如果在就触发panic
    panic("log.committing");
  if(log.outstanding == 0){//如果但前操作是并发操作的最后一个
    do_commit = 1;//就会立即执行commit
    log.committing = 1;
  } else {
    // begin_op() may be waiting for log space,
    // and decrementing log.outstanding has decreased
    // the amount of reserved space.
    wakeup(&log);//都不是就会唤醒前面的begin_op
  }
  release(&log.lock);

  if(do_commit){
    // call commit w/o holding locks, since not allowed
    // to sleep with locks.
    //欸有其他文件系统正在处理中
    commit();//调用commit将事物提交
    acquire(&log.lock);
    log.committing = 0;
    wakeup(&log);
    release(&log.lock);
  }
}

// Copy modified blocks from cache to log.
static void
write_log(void)
{
  int tail;
  //log分为两部分，头和后面的block nu以及对应的数据，这里我们是把数据拷贝进去，head还没拷贝
  for (tail = 0; tail < log.lh.n; tail++) {
    //一次遍历内存log中的block，写入到磁盘中的log中
    struct buf *to = bread(log.dev, log.start+tail+1); // log block，从log block里面也读取一个block缓冲区，
    struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
    memmove(to->data, from->data, BSIZE);//将从block cache里面的数据写道log block里面,这样可以确保写入的都再log里面
    bwrite(to);  // write the log，将对应的数据写道磁盘里面，to和from最大的不同就是他们在磁盘上的编号不一样，数据都是一样的，保证落盘到disk上的log区
    brelse(from);//
    brelse(to);
  }
}

static void
commit()
{
  if (log.lh.n > 0) {
    write_log();     // Write modified blocks from cache to log*//将所有存在在内存里面的log header中的block编号对应的block，
    //从block cache写入到磁盘上的log区域，先写道log区域，后面再安装到fs对应block区域上面
    //假如在这个地方之前crush了，那么就好像transaction没有发生过
    write_head();    // Write header to disk -- the real commit*//将内存中的log header写到磁盘中，commit point
    install_trans(0); // Now install writes to home locations
    log.lh.n = 0;//把log的n字段设置为0
    write_head();    // Erase the transaction from the log，删除log
  }
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache by increasing refcnt.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  //这个函数并没有直接调用bwrite进行对磁盘的写操作
  //所有文件系统调用的begin_op和end_op之间的写操作都会走到这个地方

  int i;

  acquire(&log.lock);//这里先获得log headler的锁，避免多个进程同时操作

  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction");
  if (log.outstanding < 1)
    panic("log_write outside of trans");

  for (i = 0; i < log.lh.n; i++) {
    //先检查这个blockno是否已经被log记录了
    if (log.lh.block[i] == b->blockno)   // log absorption,找到了对应的编号，就把他添加进去，我们已经在block cache里面写了block 45
      break;
  }
  
  //如果日志里面没有记录这个block的buf被修改了，那么就需要在log handler里面多新增加一条对于该block的记录
  //如果原本已经记录过了的话，那这一行就没有起作用
  log.lh.block[i] = b->blockno;//将这个编号写到对应的handler
  
  printf("write: %d\n",b->blockno);
  if (i == log.lh.n) {  // Add new block to log?
    bpin(b);//将b固定到block cache里面
    log.lh.n++;//如果这个i是到最后一个都没有找到这个记录，所以我们就需要增加到这个列表里面，添加headler的数量
  }
  release(&log.lock);
}

