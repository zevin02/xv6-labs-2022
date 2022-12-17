#include "user/user.h"
#include"kernel/types.h"

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(2, "sleep error\n");
        exit(0);
    }
    else
    {
        // int time = atoi(argv[1]);
        sleep(atoi(argv[1]));
    }

    exit(0);
}

