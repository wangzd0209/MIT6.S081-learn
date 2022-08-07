//primes

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int
lpipe_first_data(int lpipe[2], int *first)
{
	if(read(lpipe[0], first, sizeof(int)) == sizeof(int)){
		printf("prime %d\n", *first);
		return 0;
	}	
	return -1;
}

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
	//将管道的第一个输出
	//将输入管道的write关闭
	close(lpipe[1]);

	if(lpipe_first_data(lpipe, &first) == 0){
		int rpipe[2];
		pipe(rpipe);
		transmit_data(lpipe, rpipe, first);

		if(fork() == 0){
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

	for(i=2; i<=35; i++){
		write(p[1], &i, sizeof(int));
	}
	close(p[1]);
	//子进程递归，父进程回收子进程
	if(fork() == 0){
		primes(p);
	}else{
		close(p[0]);
		wait(0);
	}
	exit(0);
}


