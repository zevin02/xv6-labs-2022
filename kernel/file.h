struct file {//每个打开的文件都由这个结构表示
//他是管道或者inode的封装
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct inode *ip;  // FD_INODE and FD_DEVICE
  uint off;          // FD_INODE，IO偏移量，多个进程独立的打开同一个文件，不同的实例会有不同的偏移量
  short major;       // FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// in-memory copy of an inode
//内核将活动的inode集合保存在内存中
//是磁盘上dinode的副本
//当c指针引用某个inode的时候，才会在内存中存储该inode
struct inode {
  //指向inode指针可以来自文件描述符，当前工作目录，exec的瞬态内核代码
  uint dev;           // Device number
  uint inum;          // Inode number ，inode编号
  int ref;            // Reference count，引用内存中inode的数量，为0时就舍弃这个inode
  struct sleeplock lock; // protects everything below here，保护当前inode的睡眠锁，因为时磁盘操作比较慢，所以使用睡眠锁
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;        //link计数器，跟踪当前inode被多少个文件名指向
  uint size;          //表明了文件数据有多少字节
  uint addrs[NDIRECT+1];
  //这个地方的addrs，就是用来索引block number，通过block number可以索引到对应的数据
  /*
  1. 有12个direct block number，通过这12个direct number，我们可以直接索引到blcok number对应的数据块内容
  2. 有1个indirect block number：类似页表的3级映射，我们这个就是通过这个indirect number，可以查找到内存中存在的256个direct number中的一个条目
  
  */
};

// map major device number to device functions.
struct devsw {
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
