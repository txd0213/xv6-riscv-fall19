#include "kernel/param.h"
#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[])
{
    int pid;
    if(argc<2)
    {
        fprintf(2,"Usage: xargs [...]\n");
        exit(0);
    }
    char* maxBuf[MAXARG]={0};
    int buf,p=0,buff;
    for(buf=0;buf<argc-1;buf++)
    {
        maxBuf[buf]=argv[buf+1];
    }
    char c;
    buff=buf;
    char line[MAXARG];
    while (read(0,&c,1)==1)
    {
        if(c=='\n')
        {
            line[p]=0;
            maxBuf[buff]=line;
            buff++;
            maxBuf[buff]=0;
            p=0;
            pid=fork();
            if(pid==0)
            {
                exec(argv[1],(char**)maxBuf);
            }
            buff=buf;
        }
        else if (c==' ')
        {
            line[p]=0;
            strcpy(maxBuf[buff++],line);
            p=0;
        }
        else
        {
            line[p++]=c;
        }
        
    }
    while(wait(0)!=-1)
        ;
    exit(0);
    
}