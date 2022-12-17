#include "user/user.h"

int main(int argc, char *argv[])
{
    //用两个管道来连接父子进程
    int fd1[2];//给子进程读
    int fd2[2];//给父进程

    pipe(fd1);
    pipe(fd2);
    if (fork() == 0)
    {
        // child

        char buf[5];
        int n = read(fd1[0], buf, sizeof(buf) - 1);
        if (n <= 0)
        {
            exit(1);
        }
        else
        {
            int pid = getpid();
            buf[n] = 0;
            printf("%d: received %s\n", pid,buf);
            write(fd2[1], "pong", 5);
        }
        // 4 receive ping
    }
    else
    {
        // father
        write(fd1[1], "ping", 5);
        char buf[5];
        int n = read(fd2[0], buf, sizeof(buf) - 1);
        wait(0);//我们需要子进程先输出，后wait等待他退出，再进行输出父进程
        if (n <= 0)
        {
            exit(1);
        }
        else
        {
            int pid = getpid();
            buf[n] = 0;
            // printf("buf=%s\n",buf);
            printf("%d: received %s\n", pid,buf);
        }
    }
    exit(0);
}

