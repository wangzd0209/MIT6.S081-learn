# Lab 2: Systeam Call

### System call tracing[moderate]

#### 说明

实现一个系统调用，打印指定执行过程中的系统调用。

基本思路是通过trace将mask注入到当前进程的proc中，然后修改`syscall()`进行打印。

##### 代码和实现

首先看一下`user/trace.c`的核心代码。

```c
//将`trace mask`之后的命令执行，并将`mask`中指定的`system call`进行输出。
for(i = 2; i < argc && i < MAXARG; i++){
nargv[i-2] = argv[i];
}
exec(nargv[0], nargv);
exit(0);
```

根据提示将`mask`放入`proc`中

```c
//proc.h
struct proc {
    ......
  int mask; //trace mask
};
```

通过`sys_trace()`将`mask`放入`proc`中

```c
//sysproc.c
uint64
sys_trace(void)
{
  struct proc *p = myproc();
  argint(0, &(p->mask));  //通过argint调用argraw将a0寄存器的值放入p->mask
  return 0;
}
```

修改`syscall`函数将系统调用输入

```c
//syscall.c

static char *syscalls_name[] = {
......
[SYS_close]   sys_close,
[SYS_trace]   "trace"，
};


void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  //系统调用的函数
  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    //执行程序，返回输出
    p->trapframe->a0 = syscalls[num]();
    //判断如果是mask的话调用trace
    if(p->mask & (1 << num)){
      printf("%d: syscall %s -> %d\n", p->pid, syscalls_name[num], p->trapframe->a0);
    }
  } else {
    printf("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
```

修改`fork()`将父进程和子进程的信息同步

```c
//proc.c
int
fork(void)
{
    ......
   np->state = RUNNABLE;
   np->mask = p->mask;
    ......
}
```

最后按照提示首先将`syscall`注入到内核中

```c
//syscall.h
#define SYS_trace  22

//syscall.c
extern uint64 sys_trace(void);

static uint64 (*syscalls[])(void) = {
......
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
};;
```

### Sysinfo[moderate]

#### 说明

实现一个系统调用,获取当前当前的proc数和freemem

基本思路：先观察test的调用理解sysinfo结构要获取什么,然后根据`hint`在对应的文件下获取。nproc->获取当前的可用进程数, freemem->获取内存中的可用存储分片

#### 代码和实现

首先看一下user/sysinfo的核心代码

```c
//sysinfo.c
struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
};

sinfo(&info); //总体来说就是系统调用获取freemem和nproc然后进行加减操作
```

根据提示获取nporc

```c
uint64
cal_nproc(void)
{
  struct proc *p;
  int proc_num = 0;
  for(p = proc; p < &proc[NPROC]; p++) {
    if(p->state != UNUSED) {
      proc_num++;
    }
  }
  return (uint64)proc_num;
}
```

根据提示获取freemem

```c
uint64
cal_freemem(void)
{
  int page_num = 0;
  struct run *head = kmem.freelist;
  while (head)
  {
    head = head->next;
    page_num++;
  }
  return page_num * PGSIZE;
}
```

获取freemem和nproc的值，然后通过argaddr获取地址，将sysinfo放入赋值进入地址中

```c
//sysproc.c
uint64
sys_sysinfo(void)
{
  uint64 uaddr;
  struct sysinfo info;
  
  //将freemem和nproc放入
  info.freemem = cal_freemem();
  info.nproc = cal_nproc();
  //传参
  if (argaddr(0, &uaddr) < 0)
    return -1;
  //在复制
  if(copyout(myproc()->pagetable, uaddr, (char *)&info, sizeof(info)) < 0)
    return -1;
  return 0;
}
```

