#include"user/user.h"

int main(int argc,char* argv[])
{
    int pid;
    char c;
    pid=fork();
    if(pid==0)
    {
        c='/';
    }
    else
    {
        printf("parent pid is %d,child is %d\n",getpid(),pid);
        c='\\';
    }
    for(int i=0;;i++)//两个进程都会进入这个死循环，但是他们不会主动让出CPU，为了让这两个进程都能执行，有必要让他们相互切换
    {
        //交替打印时因为定时器中断在当中发挥了作用
        if((i%10000000)==0)
        {
            write(2,&c,1);
        }
    }
    //这里我们有两个计算密集型的进程
    exit(0);
}
