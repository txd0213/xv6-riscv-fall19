#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// return pid if success or -1 when failed
uint64 sys_wait2()
{
  uint64 retime, rutime, stime;
  argaddr(0, &retime);
  argaddr(1, &rutime);
  argaddr(2, &stime);
  int pid = sys_wait();
  struct proc* p= myproc();
  copyout(p->pagetable, retime, (char*)&(p->retime),sizeof(int));
  copyout(p->pagetable, rutime, (char*)&(p->rutime),sizeof(int));
  copyout(p->pagetable, stime, (char*)&(p->stime),sizeof(int));
  return pid;
}

uint64 sys_setprio()
{
  int priority;
  argint(0, &priority);
  if(priority < 1 || priority > 3)
    return -1;
  struct proc* p = myproc();
  acquire(&p->lock);
  p->priority = priority;
  release(&p->lock);
  return 0;
}

uint64 sys_yield()
{
  yield();
  return 0;
}