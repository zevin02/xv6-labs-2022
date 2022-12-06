struct buf {
  int valid;   // has data been read from disk?，缓冲区是否包含了磁盘块的副本
  int disk;    // does disk "own" buf? 缓存区是否交给了磁盘
  uint dev;
  uint blockno;//这个地方有记录是第几个blockno的编号
  struct sleeplock lock;
  uint refcnt;//有多少个进程使用了这个buf缓冲区
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

