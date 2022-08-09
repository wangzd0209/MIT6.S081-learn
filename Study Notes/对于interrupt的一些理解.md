这里我学习的是2020 MIT S6.081课程，主要是通过QEMU模拟CONSOLE和键盘输入。 即$ ls是怎样显示到屏幕的，所有的不会问题请仔细理解视频可翻译，我这里不做任何预先说明。

  对于$_ 首先在user/init.c中初始化了shell程序以及将文件描述符0，1，2对应我们一般的，其次会在user/sh.c中getcmd()中向文件描述符2 输出$_，因此会调用syscall，判断出是对于设备的write，调用设备top的write, 在这里是kernel/console。c中的consolewrite(), 通过either\_copyin将字符拷入，之后调用uartputc函数，如果buffer正确则，调用uartstart()，然后发起一个uart中断，通过PLIC分发中断然后调用uartintr(),首先判断uart是否有输入，在这里是没有的，因此我们调用consoleintr(）在console中输出，然后通过uartstart()中断完成

  对于ls我们是通过read系统调用，然后判断是设备的，因此调用consoleread，这里将shell睡眠，将cons的buffer中的依次传入UART中，生成UART中断，这里与上面基本是相同的，但是如果是换行则会唤醒shell。

注意：这里我希望去研究代码想中断是如何返回shell的。