#include "user/user.h"
void pipecmd(int *inpipe)
{
    int pipefd[2];
    pipe(pipefd);
    int firstprime;
    int n = read(inpipe[0], &firstprime, sizeof(int));
    if(firstprime==0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        // exit(0);
        return;
    }
    printf("prime %d\n", firstprime);
    if (fork() == 0)
    {
        close(pipefd[1]);
        pipecmd(pipefd);
        // exit(0);
    }
    int innum;
    int runtime = 0;
    while (n = read(inpipe[0], &innum, sizeof(int)))
    {

        // printf("%d ", innum);
        if (innum % firstprime)
        {
            int x = innum;
            close(pipefd[0]);
            write(pipefd[1], &x, sizeof(int));
        }
        runtime++;
    }
    close(pipefd[1]);

    if (runtime == 0)
    {
        close(pipefd[0]);
        int x=0;
        write(pipefd[1],&x,sizeof(int));
        // exit(0);
    }
    // printf("\n");
    wait(0);
}
int main()
{

    int rpipe[2];
    pipe(rpipe);
    if (fork() == 0)
    {
        close(rpipe[1]);
        pipecmd(rpipe);
        // exit(0);
    }
    else
    {
        printf("prime %d\n", 2);
        close(rpipe[0]);
        for (int i = 3; i < 36; i++)
        {
            if (i % 2)
            {
                int x = i;
                // printf("%d\n",i);
                write(rpipe[1], &x, sizeof(int));
            }
        }
        close(rpipe[1]);
        wait(0);
        exit(0);
    }
}

