// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCUP];

void
kinit()
{
  char lockname[8];
  //将n个cpu初始化
  for (int i = 0; i < NCUP; i++){
    snprintf(lockname, sizeof(lockname), "kmem_%d", 1);
    initlock(&kmem[i].lock, lockname);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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

  //防止中断
  push_off();
  //要看当前cid的
  int cid = cupid();
  acquire(&kmem[cid].lock);
  r->next = kmem[cid].freelist;
  kmem[cid].freelist = r;
  release(&kmem[cid].lock);
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
  int cid = cupid();
  acquire(&kmem[cid].lock);
  r = kmem[id].freelist;
  if (r)
    kmem[id].freelist = r->next;
  else{
    int cid_another;
    //去寻找freelist有的
    for (cid_another = 0; cid_another < NCPU; cid_another++){
      if (cid_another == cid)
        continue;
      acquire(&kmem[cid_another].lock);
      r = kmem[cid_another].freelist;
      if(r){
          kmem[cid_another].freelist = r->next;
          release(&kmem[cid_another].lock);
          break;
      }
      release(&kmem[cid_another].lock);
    }
  }
  release(&kmem[cid].lock);
  pop_off();
  
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
