#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[])
{
    int i,s,pid;
    int pid_fa = getpid();
    int p2[2],pothers[2];
    int buf[1];
    pipe(p2);
    for (i = 2; i <= 35; i++)
    {
        write(p2[1], &i, 4);
    }
    close(p2[1]);

loop:
    if (read(p2[0], (void *)buf, 4) == 4)
    {
        fprintf(1, "prime %d\n", buf[0]);
        s = buf[0];
    }
    else
        exit(0);
    pipe(pothers);
    pid = fork();
    if (pid > 0)
    {
        close(pothers[0]);
        while (read(p2[0], (void *)buf, 4) == 4)
        {
            if (buf[0] % s)
                write(pothers[1], (void *)buf, 4);
        }
        close(p2[0]);
        close(pothers[1]);
        if (getpid() != pid_fa)
            exit(0);
    }
    else
    {
        close(pothers[1]);
        close(p2[0]);
        p2[0] = dup(pothers[0]);
        close(pothers[0]);
        goto loop;
    }
    wait(0);
    exit(0);
}
