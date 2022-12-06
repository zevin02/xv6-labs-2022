struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;//这个地方有记录是第几个blockno的编号
  struct sleeplock lock;
  uint refcnt;//有多少个进程使用了这个buf缓冲区
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];
};

