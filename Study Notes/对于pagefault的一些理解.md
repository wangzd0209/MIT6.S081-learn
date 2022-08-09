对于未修改xv6，在内核中的地址拟内存映射是直接的，同时每个进程的虚拟内存映射在进程初始化时就是一个固定的，是一个不变量。但是这样会占用许多内存，因此可以使用虚拟地址变为动态的。因此可以有四种

1. DEMAND PAGEING 和 LAZY PAGE ALLOCATION  其实我认为这两个应该是同一种大类 都是只有在使用时才会映射地址，但是LAZY PAGE ALLOACTION主要是进行sz的扩大，而DEMAND PAGEING则是将sz下的PTE_V设置为0，只有在使用时会去文件下映射PTE，并将PTE_V变为1.

2. ZERO FILL 为了节省初始化为0的页表

3. COW FORK 为了节省对于fork的复制，只有在进程修改时才会复制，其他父子进程映射同一个va

4. MMAP 参考文件的第二层 【参考csapp好像是第十章】

   