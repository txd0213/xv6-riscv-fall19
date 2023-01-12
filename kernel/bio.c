// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUKETS 13
#define hash(x) ((x) % NBUKETS)
#define tablesize ((NBUF + NBUKETS) / NBUKETS)
#define Min(a, b) ((a) < (b) ? (a) : (b))

typedef struct hashtable{
    struct spinlock lock;
    struct buf head;
    struct buf buf[tablesize];
}hashtable;

struct {
  struct spinlock lock;
  hashtable table[NBUKETS];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for (int i = 0; i < NBUKETS; i++)
  {
      bcache.table[i].head.prev = &bcache.table[i].head;
      bcache.table[i].head.next = &bcache.table[i].head;
      for (b = bcache.table[i].buf; b < bcache.table[i].buf + tablesize; b++)
      {
        b->next = bcache.table[i].head.next;
        b->prev = &bcache.table[i].head;
        initsleeplock(&b->lock, "buffer");
        bcache.table[i].head.next->prev = b;
        bcache.table[i].head.next = b;
      }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint id = hash(blockno);
  uint mintime = 0;
  struct buf *exbuffer = 0;

  acquire(&bcache.table[id].lock);

  // Is the block already cached?
  for (b = bcache.table[id].head.next; b != &bcache.table[id].head; b = b->next)
  {
      if (b->dev == dev && b->blockno == blockno)
      {
        b->refcnt++;

        acquire(&tickslock);
        b->timestap = ticks;
        release(&tickslock);

        release(&bcache.table[id].lock);
        acquiresleep(&b->lock);
        return b;
      }
      if (b->refcnt == 0)
      {
        if (mintime == 0 || b->timestap < mintime)
        {
          mintime = b->timestap;
          exbuffer = b;
        }
      }
  }

  if (exbuffer != 0)
  {
  next:
      exbuffer->dev = dev;
      exbuffer->blockno = blockno;
      exbuffer->refcnt = 1;
      exbuffer->valid = 0;

      acquire(&tickslock);
      b->timestap = ticks;
      release(&tickslock);

      release(&bcache.table[id].lock);
      acquiresleep(&exbuffer->lock);
      return exbuffer;
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for (int i = 0; i < NBUKETS; i++)
  {
      if (i == id)
        continue;
      acquire(&bcache.table[i].lock);
      for (b = bcache.table[i].head.next; b != &bcache.table[i].head; b = b->next)
      {
        if (b->refcnt == 0)
        {
          if (mintime == 0 || b->timestap < mintime)
          {
            mintime = b->timestap;
            exbuffer = b;
          }
        }
      }
      if (exbuffer != 0)
      {
        exbuffer->next->prev=exbuffer->prev;
        exbuffer->prev->next=exbuffer->next;

        exbuffer->next = bcache.table[id].head.next;
        exbuffer->prev = &bcache.table[id].head;
        bcache.table[id].head.next->prev = exbuffer;
        bcache.table[id].head.next = exbuffer;

        release(&bcache.table[i].lock);
        goto next;
      }
      release(&bcache.table[i].lock);
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint id = hash(b->blockno);
  acquire(&bcache.table[id].lock);

  b->refcnt--;
  acquire(&tickslock);
  b->timestap = ticks;
  release(&tickslock);

  release(&bcache.table[id].lock);
}

void
bpin(struct buf *b) {
  uint id = hash(b->blockno);
  acquire(&bcache.table[id].lock);
  b->refcnt++;
  release(&bcache.table[id].lock);
}

void
bunpin(struct buf *b) {
  uint id = hash(b->blockno);
  acquire(&bcache.table[id].lock);
  b->refcnt--;
  release(&bcache.table[id].lock);
}


