# Lab 1：util

### sleep[easy]zhu

#### 说明

主要是`call`一个很简单的`syscall`，熟悉`user/`下的文件调用流程

#### 代码和实现

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	if(argc != 2){
		fprintf(2, "Usage: sleep ticks\n");
		exit(1);
	}

	sleep(atoi(argv[1]));
	exit(0);
}
```

#### 注意

>- See `kernel/sysproc.c` for the xv6 kernel code that implements the `sleep` system call (look for `sys_sleep`), `user/user.h` for the C definition of `sleep` callable from a user program, and `user/usys.S` for the assembler code that jumps from user code into the kernel for `sleep`.

注意要根据上述提示完成

### pingpong[easy]

#### 说明

主要是`call`一个很简单的`syscall`，主要是了解`pipe`从而理解管道和输入和输出以及缓冲区(尽管这里并没有，但是我们可以猜想带是通过一个缓冲区实现的)

#### 代码和实现

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
	int parent_fd[2], child_fd[2];
	char buf;
	char msg = '0';
	
	pipe(parent_fd);
	pipe(child_fd);

	if(fork() == 0){
		read(parent_fd[0], &buf, 1);
		printf("%d, received ping\n", getpid());
		write(child_fd[1], &msg, 1);
	}else{
		write(parent_fd[1], &msg, 1);
		read(child_fd[0], &buf, 1);
		printf("%d, received pong\n", getpid());
	}
	exit(0);
}
```

#### 注意

- 几个简单的函数说明

  ```
  pipe(fd); //构建一个pipe管道
  fork();//构建一个子进程，两个进程相同
  read(inode，buf， len)//inode文件
  ```

- 管道说明

  >parent_fd[], 0适用于read， 1是用于write，

- 注意关闭管道，尽管在这个task中不需要

### primes[moderate/hard]

#### 说明

原理：https://swtch.com/~rsc/thread/
我的理解：初始创建一个管道向管道输入一定序列，将第一个输入作为一个初始值，将不可整除的数输入带另一个管道。

```
p = get a number from left neighbor
print p
loop:
    n = get a number from left neighbor
    if (p does not divide n)
        send n to right neighbor
```

通过管道传递从而将序列中不可整除的数输出，理解`pipe`以及了解`close`的调用

#### 代码和实现

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


//读取左边管道传来的first数
int
lpipe_first_data(int lpipe[2], int *first)
{
	if(read(lpipe[0], first, sizeof(int)) == sizeof(int)){
		printf("prime %d\n", *first);
		return 0;
	}	
	return -1;
}

//将当前管道的可以被整除的放入右边的管道
void
transmit_data(int lpipe[2], int rpipe[2], int first)
{
	int data;

	while(read(lpipe[0], &data,sizeof(int)) == sizeof(int)){
		if (data % first)
			write(rpipe[1], &data, sizeof(int));
	}
	close(lpipe[0]);
	close(rpipe[1]);
}

void
primes(int lpipe[2])
{
	int first;
	//将管道的write关闭
	close(lpipe[1]);
	//得到first
	if(lpipe_first_data(lpipe, &first) == 0){
		int rpipe[2];
		pipe(rpipe);
        //传输数据
		transmit_data(lpipe, rpipe, first);

		if(fork() == 0){
            //进入下层循环
			primes(rpipe);
		}else{
			close(rpipe[0]);
			wait(0);
		}
	}
	exit(0);
}

int
main(void)
{
	int i;
	int p[2];
	//构建一个新的管道，输入初始数据
	//;	
	pipe(p);
    
	//先将2-35放入序列中
	for(i=2; i<=35; i++){
		write(p[1], &i, sizeof(int));
	}
    //然后关闭管道的write
	close(p[1]);
	//子进程递归，父进程回收子进程
	if(fork() == 0){
        //进入递归
		primes(p);
	}else{
        //父进程关闭read
		close(p[0]);
        //等待子进程
		wait(0);
	}
	exit(0);
}
```

#### 注意

- 一定要注意何时关闭管道，大部分错误都是这样的

### find[moderate]

#### 说明

找到指定文件夹下对应的文件

一定要基本读懂user/ls.c

#### 代码和实现

```c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char *path, char *fname)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    fprintf(2, "Usage: find <DIRECTORY> <filename>\n");
    return;
  }

  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
    fprintf(2, "find: path too long\n");
    return;
  }
  strcpy(buf, path);
  p = buf+strlen(buf);
  *p++ = '/';  
    
   //核心代码
  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ); 
    p[DIRSIZ] = 0;
    if(stat(buf, &st) < 0){
      fprintf(2, "find: cannot stat %s\n", buf);
      continue;
    }
    switch(st.type){
      case T_FILE:
        if(strcmp(fname, de.name) == 0){
          printf("%s\n", buf);
        } 
        break;
      case T_DIR:
        if(strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0){
          find(buf, fname);
        } 
        break;
    }
  }
  
  close(fd);
}

int
main(int argc, char *argv[])
{
  //判断是否小于3个参数
  if(argc < 3){
    fprintf(2, "Usage: find <path> <experssion>");
    exit(1);
  }
  //进入递归函数
  find(argv[1], argv[2]);
  exit(0);
}

```

### xargs[moderate]

#### 说明

```sh
$ echo hello too | xargs echo bye
bye hello too
```

将`hello too`作为参数加到`bye`的后面

xrgs的说明：https://www.ruanyifeng.com/blog/2019/08/xargs-tutorial.html

#### 代码和实现

```c
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[])
{
  char buf[512];
  char* full_argv[MAXARG];
  int i;
  int len;
  if(argc < 2){
    fprintf(2, "usage: xargs your_command\n");
    exit(1);
  }
  // we need an extra arg and a terminating zero
  // and we don't need the first argument xargs
  // so in total, we need one extra space than the origin argc
  if (argc + 1 > MAXARG) {
      fprintf(2, "too many args\n");
      exit(1);
  }
  // copy the original args
  // skip the first argument xargs
  for (i = 1; i < argc; i++) {
      full_argv[i-1] = argv[i];
  }
  // full_argv[argc-1] is the extra arg to be filled
  // full_argv[argc] is the terminating zero
  full_argv[argc] = 0;
  while (1) {
      i = 0;
      // read a line
      while (1) {
        len = read(0,&buf[i],1);
        if (len == 0 || buf[i] == '\n') break;
        i++;
      }
      if (i == 0) break;
      // terminating 0
      buf[i] = 0;
      full_argv[argc-1] = buf;
      if (fork() == 0) {
        // fork a child process to do the job
        exec(full_argv[0],full_argv);
        exit(0);
      } else {
        // wait for the child process to complete
        wait(0);
      }
  }
  exit(0);
}
```

一篇知乎上的答案，大致是将xrgs的后面放到 full_argv上，然后依次读取输入的指令，执行后到full_argv后面
