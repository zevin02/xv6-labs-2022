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
  if (fd < 0 || fd >= NOFILE || (f = myproc()->ofile[fd]) == 0)
    return -1;
  if (pfd)
    *pfd = fd;
  if (pf)
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

  for (fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd] == 0)
    {
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

  if (argfd(0, 0, &f) < 0)
    return -1;
  if ((fd = fdalloc(f)) < 0)
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
  if (argfd(0, 0, &f) < 0)
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
  if (argfd(0, 0, &f) < 0)
    return -1;

  return filewrite(f, p, n);
}

uint64
sys_close(void)
{
  int fd;
  struct file *f;

  if (argfd(0, &fd, &f) < 0)
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
  if (argfd(0, 0, &f) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
uint64
sys_link(void)
{
  char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
  struct inode *dp, *ip;

  if (argstr(0, old, MAXPATH) < 0 || argstr(1, new, MAXPATH) < 0)
    return -1;

  begin_op();
  if ((ip = namei(old)) == 0)
  {
    end_op();
    return -1;
  }

  ilock(ip);
  if (ip->type == T_DIR)
  {
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if ((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if (dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0)
  {
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

  for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de))
  {
    if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if (de.inum != 0)
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

  if (argstr(0, path, MAXPATH) < 0)
    return -1;

  begin_op();
  if ((dp = nameiparent(path, name)) == 0)
  {
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if ((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if (ip->nlink < 1)
    panic("unlink: nlink < 1");
  if (ip->type == T_DIR && !isdirempty(ip))
  {
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if (writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if (ip->type == T_DIR)
  {
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

static struct inode *
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if ((dp = nameiparent(path, name)) == 0)
    return 0;

  ilock(dp);

  if ((ip = dirlookup(dp, name, 0)) != 0)
  {
    iunlockput(dp);
    ilock(ip);
    if (type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
      return ip;
    iunlockput(ip);
    return 0;
  }

  if ((ip = ialloc(dp->dev, type)) == 0)
  {
    iunlockput(dp);
    return 0;
  }

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if (type == T_DIR)
  { // Create . and .. entries.
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      goto fail;
  }

  if (dirlink(dp, name, ip->inum) < 0)
    goto fail;

  if (type == T_DIR)
  {
    // now that success is guaranteed:
    dp->nlink++; // for ".."
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
  if ((n = argstr(0, path, MAXPATH)) < 0)
    return -1;

  begin_op();

  if (omode & O_CREATE)
  {
    ip = create(path, T_FILE, 0, 0);
    if (ip == 0)
    {
      end_op();
      return -1;
    }
  }
  else
  {
    if ((ip = namei(path)) == 0)
    {
      end_op();
      return -1;
    }
    ilock(ip);
    if (ip->type == T_DIR && omode != O_RDONLY)
    {
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if (ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV))
  {
    iunlockput(ip);
    end_op();
    return -1;
  }

  if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0)
  {
    if (f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }

  if (ip->type == T_DEVICE)
  {
    f->type = FD_DEVICE;
    f->major = ip->major;
  }
  else
  {
    f->type = FD_INODE;
    f->off = 0;
  }
  f->ip = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if ((omode & O_TRUNC) && ip->type == T_FILE)
  {
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
  if (argstr(0, path, MAXPATH) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0)
  {
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
  if ((argstr(0, path, MAXPATH)) < 0 ||
      (ip = create(path, T_DEVICE, major, minor)) == 0)
  {
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
  if (argstr(0, path, MAXPATH) < 0 || (ip = namei(path)) == 0)
  {
    end_op();
    return -1;
  }
  ilock(ip);
  if (ip->type != T_DIR)
  {
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
  if (argstr(0, path, MAXPATH) < 0)
  {
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for (i = 0;; i++)
  {
    if (i >= NELEM(argv))
    {
      goto bad;
    }
    if (fetchaddr(uargv + sizeof(uint64) * i, (uint64 *)&uarg) < 0)
    {
      goto bad;
    }
    if (uarg == 0)
    {
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if (argv[i] == 0)
      goto bad;
    if (fetchstr(uarg, argv[i], PGSIZE) < 0)
      goto bad;
  }

  int ret = exec(path, argv);

  for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);

  return ret;

bad:
  for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
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
  if (pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0)
  {
    if (fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  if (copyout(p->pagetable, fdarray, (char *)&fd0, sizeof(fd0)) < 0 ||
      copyout(p->pagetable, fdarray + sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0)
  {
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  return 0;
}

uint64 sys_mmap()
{
  // void *mmap(void *addr, size_t length, int prot, int flag,int fd, off_t offset);
  // 这里的addr我们不需要处理，内核自己处理即可
  // 这里的offset也是0,也是默认从文件的起始位置开始读取
  int length, prot, flag, fd; // 使用这个fd在内存中查找struct file，打开的文件
  argint(1, &length);
  argint(2, &prot);
  argint(3, &flag);
  struct file *f;
  argfd(4, &fd, &f);
  struct proc *p = myproc();
  if (!f->writable && flag & MAP_SHARED && prot & PROT_WRITE)
  {
    return (uint64)-1;
  }
  for (int i = 0; i < MAXVMA; i++)
  {
    struct vma *v = &(p->vma[i]);
    if (!v->used)
    {
      // 找到了一个没被使用过的地区，可以用来映射文件
      v->flag = flag;
      v->offset = 0;
      v->prot = prot;
      length = PGROUNDDOWN(length);
      v->length = length; // 这个length必须是page的倍数
      v->used = 1;
      v->start = p->sz;
      p->sz += length;
      v->end = p->sz;
      v->f = filedup(f); // 他对应的文件应该是引用计数增加的文件
      return v->start;
    }
  }
  return (uint64)-1;
}

// usertrap(): unexpected scause 0x000000000000000d pid=3    sepc=0x0000000000000074 stval=0x0000000000005000

uint64 sys_munmap()
{
  uint64 addr;
  argaddr(0, &addr);
  int length;
  argint(1, &length);
  struct proc *p = myproc();
  for (int i = 0; i < MAXVMA; i++)
  {
    struct vma *v = &p->vma[i];
    if (v->used && addr >= v->start && addr < v->end)//左闭右开区间
    {
      int offset=v->offset;
      int over=0;
      if (addr == v->start)
      {
        if (length == v->length)
        {
          // 需要移除所有的
          v->length=0;
          v->used=0;//把标志位清理掉
          over=1;
        }
        else
        {
          // 移除部分
          v->start += length; // start位置需要往后走
          v->length -= length;
          v->offset += length;//文件的偏移量也要移动
        }
      }
      else
      {
        v->length -= length; // 释放结尾的数据，start不变
      }
      
      if (v->flag & MAP_SHARED)
      {
        // 这个标志位，就需要写到inode中,解除映射的时候，就需要进行一个同步
        sync_file(v->f, addr, length,offset); // 进行同步,从offset位置开始进行同步，同步到文件, 因为从offset-lenght都被解除映射了，
        //所以这部分就需要被修改回去了
      }
      uvmunmap(p->pagetable, addr, length / PGSIZE, 1);

      if(over)//全部都被解除的话，就要把该文件关闭掉
      {
          fileclose(v->f);
      }
      return 0;
    }
  }
  return -1;
}
int sync_file(struct file *f, uint64 addr, int n,int offset)
{
  int r, ret = 0;

  if (f->writable == 0)
    return -1;
  if (f->type != FD_INODE)
  {
    panic("sync_file");
  }

  // write a few blocks at a time to avoid exceeding
  // the maximum log transaction size, including
  // i-node, indirect block, allocation blocks,
  // and 2 blocks of slop for non-aligned writes.
  // this really belongs lower down, since writei()
  // might be writing a device like the console.
  int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * BSIZE;
  int i = 0;
  while (i < n)
  {
    int n1 = n - i;
    if (n1 > max)
      n1 = max;

    begin_op();
    ilock(f->ip);
    if ((r = writei(f->ip, 1, addr + i, offset, n1)) > 0)
      offset += r;
    iunlock(f->ip);
    end_op();

    if (r != n1)
    {
      // error from writei
      break;
    }
    i += r;
  }
  ret = (i == n ? n : -1);

  return ret;
}

int mmap_handler(uint64 addr)
{
  // 为这个出错的虚拟地址，开辟一个物理地址进行映射
  struct proc *p = myproc();
  for (int i = 0; i < MAXVMA; i++)
  {
    struct vma *v = &p->vma[i];
    if (v->used && addr >= v->start && addr < v->end)
    {
      // 找到了这个虚拟地址在的map范围
      // 就在这个地方开始操作

      char *pa = kalloc();
      if (!pa)
      {
        return -1;
      }
      uint64 offset = addr - v->start; // 偏移量随着不同进来的次数都要变化，因为读取文件的次数不一样
      // 文件可能已经读了一部分了
      memset(pa, 0, PGSIZE); // 从实际的内存中读取pgsize大小
      int flag = PTE_U;
      if (v->prot & PROT_EXEC)
        flag |= PTE_X;
      if (v->prot & PROT_WRITE)
        flag |= PTE_W;
      if (v->prot & PROT_READ)
        flag |= PTE_R;

      if (mappages(p->pagetable, addr, PGSIZE, (uint64)pa, flag) < 0)
      {
        // 映射失败
        kfree(pa);
        return -1;
      }
      // 映射成功
      // 从offset位置去读取数据
      ilock(v->f->ip);
      readi(v->f->ip, 1, addr, offset, PGSIZE); // 把ip中的数据拷贝过来，使得v->start在用户态能够使用，因为在用户态使用了这个
      iunlock(v->f->ip);
      return 0;
    }
  }
  return -1;
}
