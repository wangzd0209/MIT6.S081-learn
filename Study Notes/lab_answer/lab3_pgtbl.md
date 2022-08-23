# Lab3 : Page Table

## lab说明

第一个lab比较简单，主要是一个循环的构造

第二个和第三个lab主要是

- 实现每个用户进程都有一个自己的`kernel page table`
- 将`user page table`复制给自己的`kernel page table`，然后适应改版的copy的相关函数





### Print a page table[easy]

#### 说明

实现一个当前的页表打印

#### 代码和实现

先参考一下`freewalk`,通过pagetable依次根据pte获取pa，然后找到子节点然后调用`kfree`函数

```c
void
freewalk(pagetable_t pagetable)
{
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } else if(pte & PTE_V){
      panic("freewalk: leaf");
    }
  }
  kfree((void*)pagetable);
}
```

然后根据`freewalk`依次获取pte和对应的pa然后打印`page table`

```c
void
vmprintcore(pagetable_t pagetable, int depth)
{
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if (pte & PTE_V) {
      for (int j = 0; j < depth; j++){
        printf(".. ");
      }    
      uint64 child = PTE2PA(pte);
      printf("..%d: pte %p pa %p\n", i, pte, child);
      //&(pagetable[i])
      if (depth < 2){
        vmprintcore((pagetable_t)child, depth + 1);
      }
    }
  }
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
vmprint(pagetable_t pagetable)
{
  printf("page table %p\n", pagetable);
  vmprintcore(pagetable, 0);
}
```

exec输出vmprint

```c
int
exec(char *path, char **argv)
{
    if(p->pid == 1)
    	vmprint(p->pagetable);
}  

```

### A kernel page table per process[hard]

#### 说明

实现每个用户进程都有一个自己的`kernel page table`

基本思想：我的实现是在`user proc`中记住`kernel_pagetable`,而不通过原有设计**蹦床页面**去实现，然后仿照`kvminit`实现一个新的`kernel_pagetable`,然后在切换页表是注意页表的切换。

#### 代码和实现

首先在`proc`中设置`kernel page table`

```c
//proc.h 
pagetable_t kernel_pagetable;  //每个用户进程拷贝的内核页表
```

实现一个修改版本`kvminit`

```c
pagetable_t
ukvminit()
{
  pagetable_t pagetable;

  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // uart registers
  uvmmap(pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);
  // virtio mmio disk interface
  uvmmap(pagetable, VIRTIO0, VIRTIO0, PGSIZE, PTE_R | PTE_W);

  // CLINT
  uvmmap(pagetable, CLINT, CLINT, 0x10000, PTE_R | PTE_W);

  // PLIC
  uvmmap(pagetable, PLIC, PLIC, 0x400000, PTE_R | PTE_W);

  // map kernel text executable and read-only.
  uvmmap(pagetable, KERNBASE, KERNBASE, (uint64)etext-KERNBASE, PTE_R | PTE_X);

  // map kernel data and the physical RAM we'll make use of.
  uvmmap(pagetable, (uint64)etext, (uint64)etext, PHYSTOP-(uint64)etext, PTE_R | PTE_W);

  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  uvmmap(pagetable, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);

  return pagetable;
}

void
uvmmap(pagetable_t pagetable, uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(pagetable, va, sz, pa, perm) != 0)
    panic("uvmmap");
}

```

修改`procinit`将原本为每个进程分配一个`kstack`取消

```c
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");

      // Allocate a page for the process's kernel stack.
      // Map it high in memory, followed by an invalid
      // guard page.
      //char *pa = kalloc();
      //if(pa == 0)
      //  panic("kalloc");
      //uint64 va = KSTACK((int) (p - proc));
      //kvmmap(va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
      //p->kstack = va;
  }
  //kvminithart();
}
```

根据提示在`allocproc`中调用`ukvminit`,新建一个`kernel_pagetable`, 为每个用户进程新建一个`kstack`，最主要是调用`uvmmap`，从而在当前的`pagetable`中分配`kstack`

```c
static struct proc*
allocproc(void)
{
    ......
  p->kernel_pagetable = ukvminit();
  //分配一个page
  char *pa = kalloc();
  if(pa == 0)
    panic("kalloc");
  uint64 va = KSTACK((int) (p - proc));
  uvmmap(p->kernel_pagetable, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  p->kstack = va;
    ......
}
```

修改`scheduler()`在切换进程时，不切换为内核页表，而切换成用户的内核页表

```c
void
scheduler(void)
{
    ......
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        //切换为用户页表
        w_satp(MAKE_SATP(p->kernel_pagetable));
        sfence_vma();
        swtch(&c->context, &p->context);
        //重新切换为内核页表
        kvminithart();
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;

        found = 1;
      }
      release(&p->lock);
    }
    ......
}
```

修改`freeproc`

```c
static void
freeproc(struct proc *p)
{
    if (p->kstack)
    	uvmunmap(p->kernel_pagetable, p->kstack, 1, 1);
 	p->kstack = 0;
 	if (p->kernel_pagetable)
    	proc_free_kernel_pagetable(p->kernel_pagetable, 0);
    p->kernel_pagetable = 0;
}

// free user part in pagetable-pages (below PLIC)
void
free_user_pagetable(pagetable_t pagetable) {
  for (int i = 0; i < 1; i++) {
    pte_t pte0 = pagetable[i];
    if ((pte0 & PTE_V) == 0) continue;
    pagetable_t pgtbl1 = (pagetable_t) PTE2PA(pte0);
    for (int j = 0; j < 96; j++) {
      pte_t* pte1 = &pgtbl1[j];
      if ((*pte1 & PTE_V) == 0) continue;
      pagetable_t pgtbl2 = (pagetable_t) PTE2PA(*pte1);
      kfree(pgtbl2);
      *pte1 = 0;
    }
  }
}
```

##### 注意

- 最简单lab实现的可以去看[6.S081 / Fall 2020 麻省理工操作系统 - 2020 年秋季 中英文字幕](https://www.bilibili.com/video/BV19k4y1C7kA?p=6&vd_source=e93351302af1b70e53af717e32a67f74)教授有自己的实现

- 对于实现用两种基本的思路

  - 重新实现一个`kvminit`
  - 修改`kvminit`

  我这里实现的第一种方法，教授实现的是第二种方法，但是有人实现直接修改后的`kvminit`的方法一般来说不会遇到，但是这种实现需要修改`main`我并不推荐这种方法。

### Simplify `copyin/copyinstr`[hard]

#### 说明

将`user page table`复制给自己的`kernel page table`，然后适应改版的copy的相关函数

#### 代码和实现

将pagetable复制进入kernel_pagetable

```c
void
u2kvmcopy(pagetable_t pagetable, pagetable_t kernel_pagetable, uint64 oldsz, uint64 newsz)
{
  pte_t *pte_from, *pte_to;
  oldsz = PGROUNDUP(oldsz);
  for (uint64 i = oldsz; i < newsz; i += PGSIZE){
    if((pte_from = walk(pagetable, i, 0)) == 0)
      panic("u2kvmcopy: src pte does not exist");
    if((pte_to = walk(kernel_pagetable, i, 1)) == 0)
      panic("u2kvmcopy: pte walk failed");
    uint64 pa = PTE2PA(*pte_from);
    uint flags = (PTE_FLAGS(*pte_from)) & (~PTE_U);
    *pte_to = PA2PTE(pa) | flags;
  }
}
```

修改`fork()`, `exec()`, and `sbrk()`.

```c
int
fork(void)
{
    ......
	u2kvmcopy(np->pagetable, np->kernel_pagetable, 0, np->sz);
    ......
}

int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;

  if(n > 0){
    if (PGROUNDUP(sz + n) >= PLIC){
      return -1;
    }
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
    u2kvmcopy(p->pagetable, p->kernel_pagetable, sz - n, sz);
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

int
exec(char *path, char **argv)
{
  	u2kvmcopy(p->pagetable, p->kernel_pagetable, 0, sz);

}
```

修改userinit

```c
void
userinit(void)
{
    u2kvmcopy(p->pagetable, p->kernel_pagetable, 0, p->sz);
}
```

最后修改copy的函数

```c
//vm.c
int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  return copyin_new(pagetable, dst, srcva, len);
}

int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  return copyinstr_new(pagetable, dst, srcva, max);
}
```
