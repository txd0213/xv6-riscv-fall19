#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *fmtname(char *path)
{
    char *p;
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;
    return p;
}

void find(char *path, char *fname)
{
    int fd;
    char *p, *temp;
    struct stat st;
    struct dirent de;
    char buf[512];
    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch (st.type)
    {
    case T_FILE:
        if (!strcmp(fmtname(path), fname))
        {
            printf("%s\n", path);
        }
        break;
    case T_DIR:
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
                continue;
            if (de.inum == 0)
                continue;
            temp = p;
            memmove(p, de.name, strlen(de.name));
            p += strlen(de.name);
            *p = 0;
            int fdd;
            if ((fdd = open(buf, 0)) < 0)
            {
                fprintf(2, "find: cannot open %s\n", buf);
                continue;
            }
            struct stat s;
            if (fstat(fdd, &s) < 0)
            {
                fprintf(1, "find: cannot stat %s\n", buf);
                close(fdd);
                continue;
            }
            if (s.type == T_FILE && !strcmp(de.name, fname))
                printf("%s\n", buf);
            else if (s.type == T_DIR)
                find(buf, fname);
            p = temp;
            close(fdd);
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        fprintf(2, "Usage: find [path] [fname]\n");
        exit(0);
    }
    find(argv[1], argv[2]);
    exit(0);
}
