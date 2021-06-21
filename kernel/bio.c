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


#define NBUCKET 13
struct {
  struct spinlock lock[NBUCKET];

  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;

int hash (int n) {
  int result = n % NBUCKET;
  return result;
}

int can_lock(int id, int j) {
  int num = NBUCKET/2;
  if (id <= num) {
    if (j > id && j <= (id+num))
      return 0;
  } else {
    if ((id < j && j < NBUCKET) || (j <= (id+num)%NBUCKET)) {
      return 0;
    }
  }
  return 1;
}

void
binit(void)
{
  struct buf *b;
  for (int i = 0;i < NBUCKET; ++i) {
    initlock(&bcache.lock[i], "bcache");
  }

  // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head[0].next = &bcache.head;
  bcache.head[0].next = &bcache.buf[0];

  for(b = bcache.buf; b < bcache.buf+NBUF-1; b++){
    // b->next = bcache.head[0].next;
    b->next = b+1;
    // b->prev = &bcache.head;
    initsleeplock(&b->lock, "buffer");
    // bcache.head.next->prev = b;
    // bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int id = hash(blockno);
  acquire(&bcache.lock[id]);

  b = bcache.head[id].next;

  // Is the block already cached?
  while (b) {
    if (b->dev == dev && b->blockno == blockno) {
        b->refcnt++;
        if (holding(&bcache.lock[id]))
            release(&bcache.lock[id]);
        acquiresleep(&b->lock);
        return b;
    }
    b = b->next;
  }

  int index = -1;
  uint smallest_tick = __UINT32_MAX__;
  // find the LRU unused buffer
  for (int j = 0; j < NBUCKET; ++j) {
      if (j != id && can_lock(id, j)) {
          // if j == id, then lock is already acquired
          // can_lock is to maintain an invariant of lock acquisition order
          // to avoid dead lock
          acquire(&bcache.lock[j]);
      } else if (!can_lock(id, j)) {
          continue;
      }
      // find suitable buck can lock, require buffer
      b = bcache.head[j].next;
      while (b) {
          if (b->refcnt == 0) {
              if (b->time_stamp < smallest_tick) {
                  smallest_tick = b->time_stamp;
                  // all these buffers in lock j
                  // relese the lock from jth hashtable
                  if (index != -1 && index != j && holding(&bcache.lock[index])) 
                    release(&bcache.lock[index]);
                  index = j;
              }   
          }
          b = b->next;
      }
      if (j != id && j!=index && holding(&bcache.lock[j])) 
        release(&bcache.lock[j]);
  }


  if (index == -1) 
    panic("bget: no buffers");
  
  //find suit able buffer bcuket
  b = &bcache.head[index];
  struct buf* selected;
  while (b) {
      //remove this buffer from j_th table,and save it into selected
      if ((b->next)->refcnt == 0 && (b->next)->time_stamp == smallest_tick) {
          selected = b->next;
          b->next = b->next->next;
          break;
      }
      b = b->next;
  }

  if (index != id && holding(&bcache.lock[index])) 
    release(&bcache.lock[index]);
  
  b = &bcache.head[id];
  while (b->next) {
      b = b->next;
  }
  //move selected into id-th table

  b->next = selected;
  selected->next = 0;
  selected->dev = dev;
  selected->blockno = blockno;
  selected->valid = 0;
  selected->refcnt = 1;
  if (holding(&bcache.lock[id]))
      release(&bcache.lock[id]);
  acquiresleep(&selected->lock);
  return selected;
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

  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->time_stamp = ticks;
  }
  
  release(&bcache.lock[id]);
}

void
bpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt++;
  release(&bcache.lock[id]);
}

void
bunpin(struct buf *b) {
  int id = hash(b->blockno);
  acquire(&bcache.lock[id]);
  b->refcnt--;
  release(&bcache.lock[id]);
}


