# 使用示例

```c
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int main(int argc, char* argv[])
{
    int fd;
    void *start;
    struct stat sb;

    fd = open("text.txt", O_RDONLY|O_CREAT); // 打开文件text.txt
    printf("fd=%d\n",fd);
    fstat(fd, &sb); // 获取文件状态
    start = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0); // 建立内存映射
    if(start == MAP_FAILED){
        return (-1);
    }

    printf("%s\n", (char*)start); // 输出内存内容
    munmap(start, sb.st_size); // 解除内存映射
    close(fd); // 关闭文件

    return 0;
}
```

![](https://cdn.jsdelivr.net/gh/zevin02/picb@master/imgss/20221212233554.png)

> 这段代码实现将文件text.txt 打开，并用mmap函数将文件映射到虚拟内存中，通过执政start对文件进行读写，可以在中断中看到由文件写入的数据，程序结束后，可以查看text.txt文件，来查看写入的数据

# 函数原型

## mmap

> void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);

* addr :制定映射的起始地址，通常是NULL，由内核来分配，是一个虚拟地址
* len：代表将文件中印社到内存的部分的长度
* prot：映射区域（内存)的保护方式
  * PROT_EXEC：映射区域可执行
  * PROT_READ:  映射区域可读取
  * PROT_NONE:  映射区域不能存取
* flag：映射区的特性标志位
  * MAP_SHARD:写入映射区的数据会复制回文件，和其他映射文件的进程共享，多个进程可以共享
  * MAP_PRIVATE:对映射区的写入操作会产生一个映射区的复制，对此区域的修改不会写会原文件
* fd：要映射到内存中的文件描述符，有open函数打开文件时返回的值
* offset：文件映射的偏移量，通常设置为0，代表从文件最前方开始对应，offset必须是分页大小的整数倍。

函数的返回值

> 实际分配的内存的起始地址

## munmap

> int munmap( void * addr, size_t len )

该调用在进程地址空间中解除一个映射关系,来表明应用程序完成了对文件的操作，addr是mmap时返回的地址，len是映射区的长度

解除映射之后，对原来映射地址的访问会导致段错误

# mmap 原理

![](https://cdn.jsdelivr.net/gh/zevin02/picb@master/imgss/20221212235155.png)

`mmap `将文件映射到进程的虚拟内存空间中，通过对这段内存的 `lord `和 `store `，实现对文件的读取和修改，不使用 `read `和 `write`

off为映射的部分在文件中的偏移量，len为映射的长度

图中实际含义

从文件描述符对应的offset开始映射长度为len的内容到虚拟地址va中s

如果内存使用的是eager方式来实现

> 对于文件的读写，内核会从文件的offset开始，将数据拷贝到内核中，设置好PTE指向物理内存的位置，后程序就可以使用load或者store来修改内存中文件的内容，完成后，使用unmmap，将dirty block写回文件中，我们可以很容易找到哪个block是dirty，因为对应的PTE_D被设置了


> 但是现在的计算机都不会这样做，都是以 `lazy`的方式实现
>
> * 记录这个PTE属于这个文件描述符
> * 存储相应的信息在VMA（Virtual Memory Area）)结构体中（这些信息来表示对应的虚拟地址的实际内容在哪里)
>   * 文件描述符
>   * 偏移量等
> * 获得VMA范围的page fault，内核从磁盘读数据，加载到内存

如果其他进程直接修改了文件的内容，内容不会出现在内存中，
