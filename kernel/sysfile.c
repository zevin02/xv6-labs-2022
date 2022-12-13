//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  argint(n, &fd);
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *p = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd] == 0){
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

uint64
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

uint64
sys_read(void)
{
  struct file *f;
  int n;
  uint64 p;

  argaddr(1, &p);
  argint(2, &n);
  if(argfd(0, 0, &f) < 0)
    return -1;
  return fileread(f, p, n);
}

uint64
sys_write(void)
{
  struct file *f;
  int n;
  uint64 p;
  
  argaddr(1, &p);
  argint(2, &n);
  if(argfd(0, 0, &f) < 0)
    return -1;

  return filewrite(f, p, n);
}

uint64
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

uint64
sys_fstat(void)
{
  struct file *f;
  uint64 st; // user pointer to struct stat

  argaddr(1, &st);
  if(argfd(0, 0, &f) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
uint64
sys_link(void)
{
  char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
  struct inode *dp, *ip;

  if(argstr(0, old, MAXPATH) < 0 || argstr(1, new, MAXPATH) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

uint64
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], path[MAXPATH];
  uint off;

  if(argstr(0, path, MAXPATH) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;

  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0){
    iunlockput(dp);
    return 0;
  }

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      goto fail;
  }

  if(dirlink(dp, name, ip->inum) < 0)
    goto fail;

  if(type == T_DIR){
    // now that success is guaranteed:
    dp->nlink++;  // for ".."
    iupdate(dp);
  }

  iunlockput(dp);

  return ip;

 fail:
  // something went wrong. de-allocate ip.
  ip->nlink = 0;
  iupdate(ip);
  iunlockput(ip);
  iunlockput(dp);
  return 0;
}

uint64
sys_open(void)
{
  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  argint(1, &omode);
  if((n = argstr(0, path, MAXPATH)) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }

  if(ip->type == T_DEVICE){
    f->type = FD_DEVICE;
    f->major = ip->major;
  } else {
    f->type = FD_INODE;
    f->off = 0;
  }
  f->ip = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if((omode & O_TRUNC) && ip->type == T_FILE){
    itrunc(ip);
  }

  iunlock(ip);
  end_op();

  return fd;
}

uint64
sys_mkdir(void)
{
  char path[MAXPATH];
  struct inode *ip;

  begin_op();
  if(argstr(0, path, MAXPATH) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

uint64
sys_mknod(void)
{
  struct inode *ip;
  char path[MAXPATH];
  int major, minor;

  begin_op();
  argint(1, &major);
  argint(2, &minor);
  if((argstr(0, path, MAXPATH)) < 0 ||
     (ip = create(path, T_DEVICE, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

uint64
sys_chdir(void)
{
  char path[MAXPATH];
  struct inode *ip;
  struct proc *p = myproc();
  
  begin_op();
  if(argstr(0, path, MAXPATH) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(p->cwd);
  end_op();
  p->cwd = ip;
  return 0;
}

uint64
sys_exec(void)
{
  char path[MAXPATH], *argv[MAXARG];
  int i;
  uint64 uargv, uarg;

  argaddr(1, &uargv);
  if(argstr(0, path, MAXPATH) < 0) {
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv)){
      goto bad;
    }
    if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0){
      goto bad;
    }
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if(argv[i] == 0)
      goto bad;
    if(fetchstr(uarg, argv[i], PGSIZE) < 0)
      goto bad;
  }

  int ret = exec(path, argv);

  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);

  return ret;

 bad:
  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);
  return -1;
}

uint64
sys_pipe(void)
{
  uint64 fdarray; // user pointer to array of two integers
  struct file *rf, *wf;
  int fd0, fd1;
  struct proc *p = myproc();

  argaddr(0, &fdarray);
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  if(copyout(p->pagetable, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
     copyout(p->pagetable, fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  return 0;
}
// kernel/sysfile.c
uint64 sys_mmap(void){
    //mmap本身不分配物理内存，而是等待page fault去分配物理页
    // uint64 addr; // 都为 0
    //mmap只是去找一个空闲的区域，写入VMA，修改但前的进程的sz
    int length, prot, flags, fd;
    // int offset; // 都为 0
    struct file* f;
    uint64 err=(uint64)-1;
    // 获取参数
    argint(1, &length);//获得要映射的字节大小
    argint(2, &prot);//标志位
    argint(3, &flags); //对进程的标志位
    argfd(4, &fd, &f);//通过fd获得对应的file结构体
 

    // 如果把只读区域映射为可写的而且是 MAP_SHARED 则直接报错
    // MAP_PRIVATE 不会写
    // 有 read-only 测试
    if(!f->writable && (prot & PROT_WRITE) && (flags & MAP_SHARED))
        return err;

    // 找到空闲区域, 找到空闲 VMA
    struct proc* p = myproc();
    for(int i = 0; i < MAXVMA; ++i) {
        struct VMA* v = &(p->vma[i]);
        if(v->length == 0) {//找到了一个空闲的区域，用于保存记录
            v->length = length;//要开辟的内存的大小
            v->start = p->sz;//把着一块空闲的起始地址保存
            v->prot = prot;//标志位置
            v->flags = flags;//设置了进程的flag
            v->offset = 0;//偏移量，我们默认为0即可
            v->file = filedup(f); // 引用计数+1，获得对应的文件file，因为要使用这个file，所以这个file的引用计数要增加
            // 地址必须是页对齐的
            length = PGROUNDUP(length);//地址必须是页的倍数
            p->sz += length;//length是要开辟的内存的大小，所以我们需要增加length字节
            v->end = p->sz;//记录增加之后，地址空间的位置
            return v->start;//返回地址空间的起始位置
        }
    }
    return err;
}

uint64 sys_munmap(void) {
  //遍历VMA，找到对应的映射
  //判断是否从start开始释放，如果是，就判断是否需要释放整个文件
  //如果要释放只做标记，之后再释放，否则会出问题
  //如果是MAP_SHARED,就需要再释放前进行写操作

  uint64 addr;//要释放哪个虚拟地址
  int length;//要释放的大小
  // 获取参数
  argaddr(0, &addr);
  argint(1, &length);

  struct proc* p = myproc();
  for(int i = 0; i < MAXVMA; ++i) {
    struct VMA* v = &(p->vma[i]);
    // 左闭右开
    if(v->length != 0 && addr < v->end && addr >= v->start) {//再VMA中找到了对应的addr范围内的地址
      int should_close = 0;
      int offset = v->offset;//
      addr = PGROUNDDOWN(addr);
      length = PGROUNDUP(length);
      // 是否从 start 开始
      if(addr == v->start) {//这个虚拟地址如果那个的起始地址
        // 是否释放整个文件
        if(length == v->length) {
          v->length = 0;
          // 不能在这个时候释放, 得在写回之后
          should_close = 1;
        } else {
          //不是释放所有的内容
          v->start += length;//start往后移动
          v->length -= length;//length要减少
          v->offset += length;//读取文件的偏移量也要移动，因为我们少读了length长度，所以页要往后动
        }
      } else {
        //如果不是从start开始释放
        // 根据要求这个时候只能是释放到结尾
        v->length -= length;
      }
      // 处理 MAP_SHARED
      if(v->flags & MAP_SHARED) {
        // 对于SHARED需要特殊处理
        // 一种简单的实现就是直接把整个文件写回去
        // !!!!(不行, 可能现在的映射已经不是整个文件)
        filewrite_offset(v->file, addr, length, offset);//文件需要共享
      }
      // 解除映射
      // 这里还有些问题, 可能并没有映射
      // if(walkaddr(p->pagetable, addr) != 0)
      uvmunmap(p->pagetable, addr, length/PGSIZE, 1);
      if(should_close)
        fileclose(v->file);
    }
  }
  return 0;
}
// n 表示写的字节数
int filewrite_offset(struct file *f, uint64 addr, int n, int offset) {
    int r, ret = 0;
    if(f->writable == 0)
        return -1;
    if(f->type != FD_INODE) {//这个地方我们只处理inode类型的文件写入
        panic("filewrite: only FINODE implemented!");
    }

    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n) {
        int n1 = n - i;
        if(n1 > max)
            n1 = max;

        begin_op();
        ilock(f->ip);
        if ((r = writei(f->ip, 1, addr + i, offset, n1)) > 0)//把数据写入到对应的inode里面
            offset += r;
        iunlock(f->ip);
        end_op();

        if(r != n1) {
            break;
        }
        i += r;
    }
    ret = (i == n ? n : -1);
    return ret;
}
int map_mmap(struct proc *p, uint64 addr) {
    // 遍历 vma 找到具体的文件，这里的addr就是出错的虚拟地址，所以我们就需要实际的为这个虚拟地址进行开辟一个物理地址来进行映射
    //我们需要做的就是
    //1. 遍历进程对应的vma，找到对应的文件
    //2. 申请真实的物理空间
    //3. 建立这个虚拟地址和这个物理空间的映射
    //4. 从文件中读如内存
    for(int i = 0; i < MAXVMA; ++i) {
        struct VMA* v = &(p->vma[i]);
        // 左闭右开
        if(v->length != 0 && addr < v->end && addr >= v->start) {//如果我们需要的虚拟地址在我们的vma的范围内，就找到了，因为这个范围内的地址，我们并没有实际开辟物理地址进行映射
            //在这个VMA中进行操作，可以获得这个页的起始地址
            uint64 start = PGROUNDDOWN(addr);
            // uint64 end = PGROUNDUP(addr);
            // 可能释放了一部分, 但是后面部分没有建立映射(offset)
            uint64 offset = start - v->start + v->offset;//获得了offset

            // 申请一块空间
            char* mem = kalloc();//实际的去申请一块物理空间
            if(!mem) {
                return 0;
            }
            memset(mem, 0, PGSIZE);//初始化

            // PROT_NONE       0x0   PTE_V (1L << 0)
            // PROT_READ       0x1   PTE_R (1L << 1)
            // PROT_WRITE      0x2   PTE_W (1L << 2)
            // PROT_EXEC       0x4   PTE_X (1L << 3)
            //                       PTE_U (1L << 4)
            // 建立映射关系
            if(mappages(p->pagetable, start, PGSIZE,
                        (uint64)mem, (v->prot<<1)|PTE_U) != 0//这里为什么虚拟左移1呢？
              ){
                kfree(mem);//失败，就需要解除映射
                return 0;
            }

            // 读取文件
            ilock(v->file->ip);//因为我们提前保存了，所以需要读取这个文件
            // 1 表示虚拟地址
            readi(v->file->ip, 1, start, offset, PGSIZE);//把数据读取到start这个虚拟地址，用户态可以使用这个start地址，从offset偏移量位置开始读取PGSIZE大小的文件
            iunlock(v->file->ip);
            //执行完之后用户态就能正常使用这个start地址
            return 1;
        }
    }
    return 0;
}