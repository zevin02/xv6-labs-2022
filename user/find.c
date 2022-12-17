#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//单一个目录下什么东西都没有时候，就返回

void display_R(char *dir, char *cmpfile);

char *
fmtname(char *path) //获得文件名
{
    static char buf[DIRSIZ];
    char *p;

    // Find first character after last slash.
    //在最后一个/后面找到第一个字符
    // ls hello/sad 找到sad这个文件名
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++; // p指向/后面的字符

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ) // p后面太大了，直接返回
        return p;
    memmove(buf, p, strlen(p));                        //把p的内容拷贝到buf里面sad
    memset(buf + strlen(p), '\0', DIRSIZ - strlen(p)); //后面全部给空,14个字符
    return buf;
}

void isfile(char *dirpath, char *cmpfile)
{
    char buf[128], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(dirpath, 0)) < 0)
    {
        fprintf(2, "ls: cannot open %s\n", dirpath);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "ls: cannot stat %s\n", dirpath);
        close(fd);
        return;
    }

    //打开的获得的东西
    if (st.type == T_DEVICE || st.type == T_FILE) //设备
    {                                             //文件,直接就是一个文件名
        close(fd);
        return;
    }

    else if (st.type == T_DIR)
    {
        if (strlen(dirpath) + 1 + DIRSIZ + 1 > sizeof buf)
        { //目录的路径名太长了
            printf("ls: path too long\n");
        }
        else
        {
            strcpy(buf, dirpath);  //把路径拷贝到buf里面
            p = buf + strlen(buf); //
            *p++ = '/';            //一个目录名后面加上
            int time = 0;

            while (read(fd, &de, sizeof(de)) == sizeof(de))
            { //读取目录
                if (de.inum == 0)
                    continue;
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0 || de.name[0] == '.')
                {
                    continue;
                }
                // printf("pbuf=%s\n",buf);
                memmove(p, de.name, DIRSIZ); //把目录下的文件名输出

                p[DIRSIZ] = 0; //用p来保存
                if (strcmp(fmtname(buf), cmpfile) == 0)
                {
                    printf("%s\n", buf); //./as获得文件名,1代表的就是目录
                }

                time++;
                //已经能打印当前目录了
                //现在就是要去实现递归的打印下面的所有目录
            }
            if (time)
            {
                display_R(dirpath, cmpfile); //这里的buf就是
            }
        }

        //把他下面的所有东西都打印完了
        //因为他是目录，所以要考虑他下面还有目录，所以我们要进入他里面

        close(fd);
    }
}

//因为是目录，所以才会走到这里
void display_R(char *dir, char *cmpfile)
{
    char buf[128], *p;
    int fd;
    struct dirent de;

    if ((fd = open(dir, 0)) < 0) //把这个目录打开
    {
        fprintf(2, "ls: cannot open %s\n", dir);
        return;
    }

    strcpy(buf, dir);      //把路径拷贝到buf里面
    p = buf + strlen(buf); //
    *p++ = '/';            //一个目录名后面加上

    while (read(fd, &de, sizeof(de)) == sizeof(de))
    { //读取目录
        if (de.inum == 0)
            continue;
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0 || de.name[0] == '.')
        {
            continue;
        }
        memmove(p, de.name, DIRSIZ); //把目录下的文件名输出
        p[DIRSIZ] = 0;               //用p来保存
        // p[strlen(buf)] = 0;                   //用p来保存

        isfile(buf, cmpfile); //这里的buf就已经组合上了后面的文件名
    }

    close(fd);
    return;
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        printf("ls [directory] [filename]\n");
        exit(1);
    }
    char dstdir[32];
    char cmpfile[32];
    strcpy(dstdir, argv[1]);
    strcpy(cmpfile, argv[2]);

    isfile(argv[1], argv[2]); //用来打印文件

    exit(0);
}
