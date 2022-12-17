#include "kernel/param.h"
#include "user/user.h"
#include "kernel/types.h"
#include "kernel/fcntl.h"

// MAXARGS,用来声明argv数组
// echo 1 | xargs echo bye
//这里的xargs就是把管道前面的输出东西添加到了xargs后面的命令去

int main(int argc, char *argv[])
{
    char *parm[MAXARG];
    int i;
    for (i = 1; i < argc; i++)
    {
        parm[i - 1] = argv[i];
    }

    parm[i - 1] = 0;

    char readchar;
    int n = 0;
    int flag = 1;
    while (1)
    {
        char buf[128];
        memset(buf, 0, sizeof(buf));
        char *p = buf;
        while (n = read(0, (char*)&readchar, 1))
        {
            
            if (readchar == '\n')
            {
                parm[i - 1] = buf;
                parm[i] = 0;
                //这里执行完了
                break;
            }
            else
            {
                *p = readchar;
                p++;
            }
        }
        if (fork() == 0)
        {
            exec(parm[0], parm);
        }
        else
        {
            wait(0);
        }
        if (n == 0)
        {
            break;
        }
    }

    exit(0);
}
