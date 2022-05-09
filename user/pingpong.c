#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[])
{
    int pid;
    char buf[1];
    int ping[2], pong[2];
    pipe(ping);
    pipe(pong);
    if ((pid = fork()) == 0)
    {
        close(ping[1]);
        close(pong[0]);
        if (read(ping[0], buf, sizeof buf) == 1)
        {
            fprintf(1, "%d: received ping\n", getpid());
            write(pong[1], buf, 1);
            close(pong[1]);
        }
    }
    else
    {
        close(ping[0]);
        write(ping[1], "p", 1);
        close(ping[1]);

        close(ping[1]);
        if (read(pong[0], buf, sizeof buf) == 1)
        {
            fprintf(1, "%d: received pong\n", getpid());
        }
    }
    exit(0);
}
