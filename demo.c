
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include<string.h>

void mmap_test1()
{
    int fd;
    void *start=NULL;
    struct stat sb;

    fd = open("text.txt", O_RDONLY|O_CREAT); // 打开文件text.txt
    printf("fd=%d\n",fd);
    fstat(fd, &sb); // 获取文件状态
    start = (char*)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0); // 建立内存映射
    if(start == MAP_FAILED){
        return (-1);
    }
    
    printf("%s\n", (char*)start); // 输出内存内容
    char s[123]="write to it\n";
    munmap(start, sb.st_size); // 解除内存映射
    close(fd); // 关闭文件

}

void mmap_test2()
{
    void* p=mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_PRIVATE,-1,0);
    if(p==MAP_FAILED)
    {
        printf("success\n");
    }
    else
    {
        printf("failure\n");
    }
    *(int*)p=2;
    printf("read=%d\n",*(int*)p);
}

int main(int argc, char* argv[])
{
    // mmap_test1();
    mmap_test2();
    return 0;
}

