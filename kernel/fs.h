// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  //文件系统的系统信息都在这个superblock里面
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.  inode number
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block bitmap块的位置
};

#define FSMAGIC 0x10203040

#define NDIRECT 11
#define NSINGLEINDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define NDOUBLEDIRECT ((NINDIRECT)*(NINDIRECT))
#define MAXFILE (NDIRECT + NINDIRECT+NDOUBLEDIRECT)

// On-disk inode structure
//磁盘上的inode的结构
struct dinode {
  short type;           // File type区分文件，目录，特殊文件（设备），type位0表示磁盘上的inode是空闲的
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system，统计这个inode的目录条目数，以便识别何时应该释放磁盘上的inode以及数据块
  uint size;            // Size of file (bytes)//文件中内容的字节数
  uint addrs[NSINGLEINDIRECT+1];   // Data block addresses，保存文件内容的磁盘块的块号
};
//sizeof(dinode)=64byte,

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
//i就是inode number
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)//通过inode number计算出他在磁盘上的哪个位置（block number）

// Bitmap bits per block
#define BPB           (BSIZE*8)//一个块1024个字节，每个字节8个bit位，有这么多个状态
 
// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {//目录层的每个条目都是一个这个结构
//一个目录16字节
  ushort inum;//inode编号，inum=0说明这个条目是空的，不存在，2个字节的的inode编号
  char name[DIRSIZ];//目录的名字，以及14个字节的名字
};

