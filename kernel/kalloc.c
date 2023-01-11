// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end ,int cpuID);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void kinit()
{
  for (int i = 0; i < NCPU; i++)
  {
    initlock(&kmem[i].lock, "kmem");
    freerange(end, (void *)PHYSTOP, i);
  }
}

void freerange(void *pa_start, void *pa_end, int cpuID)
{
  uint64 p;
  p = PGROUNDUP((uint64)pa_start);

  uint64 size_cpu = ((uint64)pa_end - (uint64)p) / NCPU;
  uint64 begin =  PGROUNDUP(p + size_cpu * cpuID);
  uint64 end = begin + size_cpu;

  for (p = begin; p + PGSIZE <= end; p += PGSIZE)
    kfree((void *)p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();

  int cpuID = cpuid();
  acquire(&kmem[cpuID].lock);
  r->next = kmem[cpuID].freelist;
  kmem[cpuID].freelist = r;
  release(&kmem[cpuID].lock);

  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int cpuID = cpuid();

  acquire(&kmem[cpuID].lock);
  r = kmem[cpuID].freelist;
  if (r)
    kmem[cpuID].freelist = r->next;
  else
  {
    for (int i = NCPU - 1; i >= 0; i--)
    {
      if (i == cpuID)
        continue;
      else
      {
        acquire(&kmem[i].lock);
        r = kmem[i].freelist;
        if (r)
        {
          kmem[i].freelist = r->next;
          release(&kmem[i].lock);
          break;
        }
        release(&kmem[i].lock);
      }
    }
  }
  release(&kmem[cpuID].lock);

  pop_off();

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk

  return (void *)r;
}
