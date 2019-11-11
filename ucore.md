#ucore实验报告
##lab1
###练习一  理解通过make生成执行文件的过程
> 问题一：操作系统镜像文件ucore.img是如何一步一步生成的？(需要比较详细地解释Makefile中每一条相关命令和命令参数的含义，以及说明命令导致的结果)

为了回答这个问题，我们需要仔细分析`makefile`文件的内容，看到程序具体是怎样编译和链接的。
首先输入命令`make V=`，使用`make`工具便把目录下的文件进行了编译。通过设置`V=`参数，把编译过程打印了下来，如图：
**由整个过程可以知道**
- 编译了kern目录下的文件
 - 将多个代码编译成.o文件
 - 将.o文件连接起来，构建出了内核`bin/kernel`
- 编译boot目录下的文件
 - 这里首先编译了`bootasm.S，bootmain.c`，生成了`obj/bootblock.o`,这是基本的引导程序，分析对应的`makefile`代码可以看到`bootblock`的入口地址为`0x7c00`
 - 编译`sign.c`生成了`sign.o`，这个工具要对刚才生成的`bootblock.o`进行规范化
 - 使用`sign.o`进行规范化，生成了`bin/bootblock`引导扇区，并且分析`sign.c`可以看到`sign.o`是将输入文件拷贝到输出文件，控制输出文件的大小为512字节，并将最后两个字节设置为`0x55AA`
- 生成`ucore.img`虚拟磁盘
 我们看到生成`ucore.img`下有三个dd命令
 - dd初始化`ucore.img`为`5120000 bytes`，内容全部为`0`，对makefile对应代码分析可以看到这`5120000bytes`的空间其实是10000个`block`，由于没有有指定大小，所以默认为`512bytes`
 - dd拷贝`bin/bootblock`到`ucore.img`第一个扇区，
 - dd不断拷贝`bin/kernel`到`ucore.img`第二个扇区往后的空间，循环读入直至读完

至此生成了`ucore.img`
**总结过程如下**
1. 编译`libs`和`kern`目录下所有的.c和.S文件，生成.o文件，并链接得到`bin/kernel`内核
1. 编译`boot`目录下所有的`.c`和`.S`文件，生成`.o`文件，并链接得到`bin/bootblock.o`
1. 编译`tools/sign.c`文件，得到`bin/sign.o`工具
1. 利用`bin/sign.o`工具将`bin/bootblock.o`文件规范化为512字节的`bin/bootblock`文件，并将`bin/bootblock`的最后两个字节设置为0x55AA
1. 为`bin/ucore.img`分配10000个`block`的内存空间，并将`bin/bootblock`复制到`bin/ucore.img`的第一个k的位置，然后将`bin/kernel`复制到`bin/ucore.img`第二个`block`开始的位置

> 问题二：一个被系统认为是符合规范的硬盘主引导扇区的特征是什么？

从问题一的分析中，可以知道由`sign.c`编译而成的工具`sign.o`对`bootblock`进行了规范化，所以只需分析`sign.c`的代码即可知道规范化的要求是什么

可以看到，`sign.c`首先检查了输入参数，然后把长度为`510bytes`的源文件读入长度为`512bytes`的数组buf，并把buf的最后两位设置为`0x55，0xAA`
所以主引导扇区规则应该是：
1.长度为`512bytes`
2.最后两个字节为`0x55、0xAA`，*这应该是一个标志，为什么是他们我也不明白
*

###练习二 使用qemu执行并调试lab1中的软件
>问题一：从CPU加电后执行的第一条指令开始，单步跟踪BIOS的执行。

为了能够使用gdb在qemu里调试，首先将文件`tools/gdbinit`的内容修改为：

	set architecture i8086
	target remote :1234
![](https://i.imgur.com/ltB20ag.jpg)
然后在lab1的目录下执行`make debug`，启动qemu和gdb开始调试

- 此时的CS为`0xF000`，EIP为`0xFFF0`，所以内存地址为`0xFFFF0`
- 由此可以知道，CPU加电以后`CS:IP`会被防止在地址`0xFFFF0`的位置，也就是第一条执行的指令位置
- 第一条指令是一个长跳转，跳转的目的地址为`0xFe05b`
![](https://i.imgur.com/zUkGfZU.jpg)
- 跳转的目的地址是引导程序BIOS，所以BIOS储存在`CS：IP`为`0xf000:0xe05b`的位置
![](https://i.imgur.com/6q5UlqC.jpg)


>问题二：在初始化位置0x7c00设置实地址断点,测试断点正常。

在gdb中输入`b *0x7c00`，即在0x7c00设置实地址断点，
输入`c`使程序继续执行，直到刚才设置的断点，可以发现程序停在了0x7c00处，断点正常

>问题三：从0x7c00开始跟踪代码运行,将单步跟踪反汇编得到的代码与bootasm.S和 bootblock.asm进行比较。

输入`x /10i $pc`查看pc接下来的10条指令：

    => 0x7c00: cli
    0x7c01: cld
    0x7c02: xor %eax,%eax
    0x7c04: mov %eax,%ds
    0x7c06: mov %eax,%es
    0x7c08: mov %eax,%ss
    0x7c0a: in 
    0x64,0x7c0c:test
    0x64,0x7c0c:test
    0x2,%al
    0x7c0e: jne 0x7c0a
    0x7c10: mov $0xd1,%al
![](https://i.imgur.com/jBfWlBv.jpg)
对比起`boot/bootasm.s`中的汇编代码，可以发现与单步跟踪到的反汇编代码基本是一致的
接着可以通过指令`si`单步调试

>问题四：自己找一个bootloader或内核中的代码位置，设置断点并进行测试。

还是修改`tools/gdbinit`文件，在其中添加断点break kern_init，在进行刚才的过程，测试断点正常

###练习三 分析bootloader进入保护模式的过程
>问题一：为何开启A20，以及如何开启A20

在实模式下, 我们可以访问超过1MB的空间，但我们只希望访问1MB以内的内存空间，而不希望程序访问到1MB以外的空间，所以CPU中添加了一个可控制`A20`地址线的模块，通过这个模块，我们在实模式下将第`20bit`的地址线限制为`0`，这样CPU就不能访问超过`1MB`的空间了。进入保护模式后，我们再通过这个模块解除对A20地址线的限制，这样我们就能访问超过1MB的内存空间了。

那么如何开启A20呢？分析bootmain.c中的代码：

	seta20.1:
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x2, %al
    jnz seta20.1

    movb $0xd1, %al                                 # 0xd1 -> port 0x64
    outb %al, $0x64                                 # 0xd1 means: write data to 8042's P2 port

	seta20.2:
    inb $0x64, %al                                  # Wait for not busy(8042 input buffer empty).
    testb $0x2, %al
    jnz seta20.2

    movb $0xdf, %al                                 # 0xdf -> port 0x60
    outb %al, $0x60                                 # 0xdf = 11011111, means set P2's A20 bit(the 1 bit) to 1


从`seta20.1`我们而可以看到，一开始程序在检查8042输入是否空闲，如果不空闲，代码执行到`jnz seta20.1`会跳转到标识`seta20.1`处继续执行，这样实现了一直等待。直到端口可用之后，代码继续往下执行，将`0xd1`赋给端口`0x64`，意思是要向8042芯片的`P2`端口写数据了。然后继续等待，如果端口可用，代码会继续往下执行，将`0xdf`赋给端口`0x60`，意思是设置`P2`的`A20`位为`1`
这样就开启了A20地址线

>问题二：如何初始化GDT表

还是分析代码:

	...
	#define SEG_NULLASM                                             \
	    .word 0, 0;                                                 \
	    .byte 0, 0, 0, 0
	
	#define SEG_ASM(type,base,lim)                                  \
	    .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);          \
	    .byte (((base) >> 16) & 0xff), (0x90 | (type)),             \
	        (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)
	...
	lgdt gdtdesc
	...
	gdt:
	    SEG_NULLASM                                     # null seg
	    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # code seg for bootloader and kernel
	    SEG_ASM(STA_W, 0x0, 0xffffffff)                 # data seg for bootloader and kernel
	
	gdtdesc:
	    .word 0x17                                      # sizeof(gdt) - 1
	    .long gdt                                       # address gdt
可以看到，代码首先有两个宏定义：`SEG_NULLASM`和`SEG_ASM(type,base,lim)`
`SEG_NULLASM`是一个零表项定义，他的内容都是零
`SEG_ASM(type,base,lim)`是一个段描述符的格式，他有三个内容，分别是类型，内容和长度限制

然后代码定义了gdt表项的内容：一个空表项、一个代码段、一个数据段。代码段定义为可读可写的属性，内容为全零；数据段定义为可读的内容（这里我有一个问题，为什么这两个属性是不一样的），内容都是零，长度限制都是`0xffffffff`，也就是`4G`
之后的`gdtdesc`则是定义了一个段描述符的大小和起始地址，可以看到描述符表项的大小为`0x17+1=0x18`，十进制就是24个字节

而`lgdt gdtdesc `则是将全局描述符表的大小和起始地址共8个字节加载到全局描述符表寄存器`GDTR`中，这样在程序开始运行过程中则可以通过GDTR寄存器随时可以找到全局描述符表项的位置并访问它。

>问题三：如何进入保护模式
进入保护模式很简单，分析代码：

	movl %cr0, %eax
    orl $CR0_PE_ON, %eax
    movl %eax, %cr0
我们可以看到代码中只需将寄存器cr0的PE位置为1，即将cr0的内容与CRO_PE_ON这个数异或，就可以进入保护模式了

###练习四 分析bootloader加载ELF格式的OS的过程
*要求：通过阅读bootmain.c，了解bootloader如何加载ELF文件。通过分析源代码和通过qemu来运行并调试bootloader&OS，理解：
bootloader如何读取硬盘扇区的？
bootloader是如何加载ELF格式的OS？*

>问题一：bootloader如何读取硬盘扇区的？

题目要求我们阅读bootmain.c，那么我们就分析代码：
bootmain.c中一开始定义了几个函数，通过函数名我们不难理解函数的作用，我们要的答案就在函数readsect中：

	static void readsect(void *dst, uint32_t secno) {
    // wait for disk to be ready
    waitdisk();

    outb(0x1F2, 1);                         // count = 1
    outb(0x1F3, secno & 0xFF);
    outb(0x1F4, (secno >> 8) & 0xFF);
    outb(0x1F5, (secno >> 16) & 0xFF);
    outb(0x1F6, ((secno >> 24) & 0xF) | 0xE0);
    outb(0x1F7, 0x20);                      // cmd 0x20 - read sectors

    // wait for disk to be ready
    waitdisk();

    // read a sector
    insl(0x1F0, dst, SECTSIZE / 4);
	}
根据代码，函数readsect大致的过程：


1. waitdisk():等待硬盘空闲
2. 发出读取扇区的命令
3. waitdisk():再次等待硬盘空闲
4. 读取一个扇区
关键的理解点在第二点，而想要理解2，就需要仔细的阅读材料：*所有的IO操作是通过CPU访问硬盘的IO地址寄存器完成。硬盘共有8个IO地址寄存器，其中第1个存储数据，第8个存储状态和命令，第3个存储要读写的扇区数，第4~7个存储要读写的起始扇区的编号（共28位）*
也就是说，0x1F2寄存器说明了要读取一个扇区，0x1F3~0x1F6中依次存放了要读写的那一个扇区的编号；0x1F7寄存器说明了发出的命令是要读取。
而第四点就是在实际读取一个扇区，insl说明程序从0x1F0寄存器中读数据，以dword双字为单位，所以sectsize除以了4

>问题2： bootloader如何加载ELF格式的OS

想要知道这个问题，我们就得从bootmain的函数去分析：

	void bootmain(void) {
    // read the 1st page off disk
    readseg((uintptr_t)ELFHDR, SECTSIZE * 8, 0);

    // is this a valid ELF?
    if (ELFHDR->e_magic != ELF_MAGIC) {
        goto bad;
    }

    struct proghdr *ph, *eph;

    // load each program segment (ignores ph flags)
    ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
    eph = ph + ELFHDR->e_phnum;
    for (; ph < eph; ph ++) {
        readseg(ph->p_va & 0xFFFFFF, ph->p_memsz, ph->p_offset);
    }

    // call the entry point from the ELF header
    // note: does not return
    ((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();
	}
首先我们要知道ELF格式的OS就是bootloasder要加载的bin/kernel文件，OS指的就是它。其开头是ELF header，ELF Header里面含有phoff字段，用于记录program header表在文件中的偏移，由该字段可以找到程序头表的起始地址。程序头表是一个结构体数组，其元素数目记录在ELF Header的phnum字段中。
程序头表的每个成员分别记录一个Segment的信息，包括以下加载需要用到的信息： 
- uint offset; // 段相对文件头的偏移值，由此可知怎么从文件中找到该Segment
- uint va; // 段的第一个字节将被放到内存中的虚拟地址，由此可知要将该Segment加载到内存中哪个位置
- uint memsz; // 段在内存映像中占用的字节数，由此可知要加载多少内容
根据ELF Header和Program Header表的信息，我们便可以将ELF文件中的所有Segment逐个加载到内存中

我们逐行代码分析：
1. 首先从程序硬盘中将bin/kernel文件的第一页内容加载到内存地址为0x10000的位置，目的是读取kernel文件的ELF Header信息。
1. 然后校验ELF_Header,确保文件类型没有出错
1. 接着读取ELF Header的e_phoff字段，得到Program Header表的起始地址；读取ELF Header的e_phnum字段，得到Program Header表的元素数目。
1. 遍历Program Header表中的每个元素，得到每个Segment在文件中的偏移、要加载到内存中的位置（虚拟地址）及Segment的长度等信息，并通过磁盘I/O进行加载
1. 最后加载完毕，通过ELF Header的e_entry得到内核的入口地址，并跳转到该地址开始执行内核代码

###练习五 实现函数调用堆栈跟踪函数
>问题一：我们需要在lab1中完成kdebug.c中函数`print_stackframe`的实现，可以通过函数`print_stackframe`来跟踪函数调用堆栈中记录的返回地址。如果能够正确实现此函数，可在lab1中执行 `“make qemu”`后，在`qemu`模拟器中得到类似如下的输出：

题目中给了很多提示，我们可以根据提示去编程实现。
*(1) call read_ebp() to get the value of ebp. the type is (uint32_t);
(2) call read_eip() to get the value of eip. the type is (uint32_t);*
根据这里的提示，只需要调用两个函数就可以得到ebp和eip的值，当然需要先定义两个`uint32_t`类型的变量`ebp`和`eip`，代码：

	 uint32_t *ebp = 0;
	 uint32_t esp = 0;
	
	 ebp = (uint32_t *)read_ebp();
	 eip = read_eip();
继续看提示：*(3) from 0 .. STACKFRAME_DEPTH
  (3.1) printf value of ebp, eip
  (3.2) (uint32_t)calling arguments [0..4] = the contents in address (uint32_t)ebp +2 [0..4]
  (3.3) cprintf("\n");
  (3.4) call print_debuginfo(eip-1) to print the C calling function name and line number, etc.
  (3.5) popup a calling stackframe
  NOTICE: the calling funciton's return addr eip  = ss:[ebp+4]
  the calling funciton's ebp = ss:[ebp]*
可以看到，首先需要一个从0到栈底的循环，然后打印`ebp，eip`的值，再向上的四个都是输入的参数，也要打印出来。然后调用函数`print_debuginfo`打印出函数名*(print_debuginfo函数完成查找对应函数名并打印至屏幕的功能)*，由于eip指向的是下一条要执行的指令，不是现在正在执行的指令，所以要把eip减1，最后弹出调用函数的栈帧即可，代码如下：

	while (ebp)
 	{
     cprintf("ebp:0x%08x eip:0x%08x args:", (uint32_t)ebp, esp);
     cprintf("0x%08x 0x%08x 0x%08x 0x%08x\n", ebp[2], ebp[3], ebp[4], ebp[5]);

     print_debuginfo(esp - 1);

     esp = ebp[1];
     ebp = (uint32_t *)*ebp;
	 }
程序完成，执行`make qemu`，可以得到与题目要求一致的答案
但是这个答案是什么意思呢？我们来分析一下。我们从代码中的函数调用关系，可以看到函数之间的关系是这样的：

	kern_init ->
		grade_backtrace ->
       		grade_backtrace0(0, (int)kern_init, 0xffff0000)->
           		grade_backtrace1(0, 0xffff0000) ->
               		grade_backtrace2(0, (int)&0, 0xffff0000, (int)&(0xffff0000)) ->
                   		mon_backtrace(0, NULL, NULL) ->
                           	print_stackframe ->
通过打印出的结果，我们可以对应到这个函数调用关系上：

	ebp:0x00007b38 eip:0x00100bf2 args:0x00010094 0x0010e950 0x00007b68 0x001000a2
	    kern/debug/kdebug.c:297: print_stackframe+48
	ebp:0x00007b48 eip:0x00100f40 args:0x00000000 0x00000000 0x00000000 0x0010008d
	    kern/debug/kmonitor.c:125: mon_backtrace+23
	ebp:0x00007b68 eip:0x001000a2 args:0x00000000 0x00007b90 0xffff0000 0x00007b94
	    kern/init/init.c:48: grade_backtrace2+32
	ebp:0x00007b88 eip:0x001000d1 args:0x00000000 0xffff0000 0x00007bb4 0x001000e5
	    kern/init/init.c:53: grade_backtrace1+37
	ebp:0x00007ba8 eip:0x001000f8 args:0x00000000 0x00100000 0xffff0000 0x00100109
	    kern/init/init.c:58: grade_backtrace0+29
	ebp:0x00007bc8 eip:0x00100124 args:0x00000000 0x00000000 0x00000000 0x0010379c
	    kern/init/init.c:63: grade_backtrace+37
	ebp:0x00007be8 eip:0x00100066 args:0x00000000 0x00000000 0x00000000 0x00007c4f
	    kern/init/init.c:28: kern_init+101
	ebp:0x00007bf8 eip:0x00007d6e args:0xc031fcfa 0xc08ed88e 0x64e4d08e 0xfa7502a8
	    <unknow>: -- 0x00007d6d --
这里有一个问题。ebp和eip的值可以理解，这是函数kern_init的栈顶和返回地址，但最后一行的四个参数是哪里来的呢？bootmain函数调用kern_init没有传入参数啊
我们注意，此时栈顶的位置恰好在boot loader第一条指令存放的地址的上面，而args恰好是kern_int的ebp寄存器指向的栈顶往上第2~5个单元，因此args存放的就是boot loader指令的前16个字节。

###练习六 完善中断初始化和处理
>问题一：中断描述符表（也可简称为保护模式下的中断向量表）中一个表项占多少字节？其中哪几位代表中断处理代码的入口？
中断描述符表中一个表项占8个字节，结构为：

	bit 63..48: offset 31..16
	bit 47..32: 属性信息，包括DPL、P flag等
	bit 31..16: Segment selector
	bit 15..0: offset 15..0
其中第16~32位是段选择子，用于索引全局描述符表GDT来获取中断处理代码对应的段地址，再加上第0~15、48~63位构成的偏移地址，即可得到中断处理代码的入口。

>问题二：请编程完善kern/trap/trap.c中对中断向量表进行初始化的函数idt_init。在idt_init函数中，依次对所有中断入口进行初始化。使用mmu.h中的SETGATE宏，填充idt数组内容。每个中断的入口由tools/vectors.c生成，使用trap.c中声明的vectors数组即可。
idt_init顾名思义，它的功能是初始化IDT表，IDT表中每个元素均为一个门描述符，记录一个中断向量的属性，包括中断向量对应的中断处理函数的段选择子/偏移量、门类型（是中断门还是陷阱门）、DPL等。因此，初始化IDT表实际上是初始化每个中断向量的这些属性。

代码trap.c中已经给出了过程的提示：
*(1) Where are the entry addrs of each Interrupt Service Routine (ISR)?
All ISR's entry addrs are stored in vectors. where is uintptr_t vectors[] ?
 vectors[] is in kern/trap/vector.S which is produced by tools/vector.c
(try "make" command in lab1, then you will find vector.S 
You can use  "extern uintptr_t __vectors[];" to define this extern variable which will be used later.
(2) Now you should setup the entries of ISR in Interrupt Description Table (IDT).
Can you see idt[256] in this file? Yes, it's IDT! you can use SETGATE macro to setup each item of IDT
(3) After setup the contents of IDT, you will let CPU know where is the IDT by using 'lidt' instruction.
You don't know the meaning of this instruction? just google it! and check the libs/x86.h to know more.
Notice: the argument of lidt is idt_pd. try to find it!*

什么意思呢？翻译一下就是：

*题目已经提供中断向量的门类型和DPL的设置方法：除了系统调用的门类型为陷阱门、DPL=3外，其他中断的门类型均为中断门、DPL均为0.
中断处理函数的段选择子及偏移量的设置要参考kern/trap/vectors.S文件：由该文件可知，所有中断向量的中断处理函数地址均保存在__vectors数组中，该数组中第i个元素对应第i个中断向量的中断处理函数地址。而且由文件开头可知，中断处理函数属于.text的内容。因此，中断处理函数的段选择子即.text的段选择子GD_KTEXT。从kern/mm/pmm.c可知.text的段基址为0，因此中断处理函数地址的偏移量等于其地址本身。
完成IDT表的初始化后，还要使用lidt命令将IDT表的起始地址加载到IDTR寄存器中。*

根据提示，编写程序：

	extern uintptr_t __vectors[];
	int i;
	for(i = 0 ; i < 256 ; i++) {
	    SETGATE(idt[i], 0, GD_KTEXT, __vectors[i], DPL_KERNEL);
	}
	lidt(&idt_pd);

>问题三：请编程完善trap.c中的中断处理函数trap，在对时钟中断进行处理的部分填写trap函数中处理时钟中断的部分，使操作系统每遇到100次时钟中断后，调用print_ticks子程序，向屏幕上打印一行文字”100 ticks”
trap函数只是直接调用了trap_dispatch函数，而trap_dispatch函数实现对各种中断的处理，题目要求我们完成对时钟中断的处理，实现非常简单：定义一个全局变量ticks，每次时钟中断将ticks加1，加到100后打印"100 ticks"，然后将ticks清零重新计数。代码实现如下：

 	case IRQ_OFFSET + IRQ_TIMER:
        if (((++ticks) % TICK_NUM) == 0) {
            print_ticks();
            ticks = 0;
        }
测试一下，如图：
![Demo](https://i.imgur.com/SJ0Szek.png)

###challenge 1
>要求：扩展proj4,增加syscall功能，即增加一用户态函数（可执行一特定系统调用：获得时钟计数值），当内核初始完毕后，可从内核态返回到用户态的函数，而用户态的函数又通过系统调用得到内核态的服务。

所以这个拓展练习就是在让我们完成中断处理函数。

 int 指令进行下面一些步骤:（这些步骤来自xv6中文文档）            1. 从 IDT 中获得第 n 个描述符,n 就是 int 的参数。             2.检查 %cs 的域 CPL <= DPL,DPL 是描述符中记录的特权级。             3. 如果目标段选择符的 PL < CPL,就在 CPU 内部的寄存器中保存 %esp 和 %ss 的值。（这里的目标段选择符我不太清楚是什么，我觉得可能是相应的GDT表项里特区级或者就是这个中断描述符里的CS所记录的特权级）            
 4.从一个任务段描述符中加载 %ss 和 %esp。（这里的任务段就是TSS，一般设定为每个CPU一个，且在固定的段表项）            
 5.将 %ss 压栈。            
 6.将 %esp 压栈。            
 7.将 %eflags 压栈。            
 8.将 %cs 压栈。            
 9.将 %eip 压栈。            
 10.清除 %eflags 的一些位。            
 11.设置 %cs 和 %eip 为描述符中的值。    

从上面的内容可以看出，int这短短的一条指令其实做了非常多的事（这可能表明实现这条指令的电路非常复杂吧）。不过，如果步骤3不为真的话，3，4，5，6这几个步骤都是不会执行的。
因此，对于没有发生特权级转换的中断，其实是没有栈切换的。一直都是用那个栈在处理中断。      
至于iret指令的动作也是类似的，因为int指令和iret指令是一对的，就像call指令与ret指令。其指令的步骤如下：             
 1.将 %eip 弹栈。            
 2.将 %cs 弹栈。            
 3.将 %eflags 弹栈。            
 4.将 %esp 弹栈。            
 5.将 %ss 弹栈。          
从上面的内容可以看出，iret其实就是int的逆过程。相应的，如果弹栈过程中，发现栈中%cs和当前%cs的特权级是一样的，那么步骤4,5是不会发生的。因为cpu会认为int指令并没有把这两个内容压栈。      
至于cpu如何表示特权级：就是通过%cs寄存器的最低两位来表示，表示为数字即0~3，其中数字越小特权级越高。一般实现只用两个状态，内核态为0，用户态为3。

因为，内核初始化完之后，是处于内核态的。要想通过中断来实现转换到用户态，就必须在一个中断里把当前中断的trapframe里的各种段寄存器强行改为用户态的内容，然后通过iret指令弹栈就实现了特权级的转换了。不过，根据上面的知识，这里有点坑人的地方就是：因为这个中断是在内核态陷入的，所以int指令并没有将%ss和%esp压栈！！！但是，我们在这个中断例程里却要修改它的值（%ss），所以，如果我们在int指令调用前，不做点处理的话，就是写入到错误的位置，从而破坏了栈结构。因此，解决办法就是：int指令没有为我们提供足够的位置，那我们就自己提供。于是，在int指令前，我们将%esp的值减8就为%esp和%ss预留了空间了。这里还有一个小细节就是，用户态返回后要进行cprintf调用，这个函数使用了in和out指令。但是如果不设置好eflags的值的话，在用户态执行这两条指令是会产生13号中断错误的。于是，要在系统调用里修改以下trapframe的eflags的值。最后，中断返回后，通过汇编指令，把ebp的值传给esp就使栈顶位置正确归位，于是就完成了。

至于从用户态转为内核态也是大同小异。坑人的地方就是，这次int指令的确把%ss和%esp压栈了，但是这回是iret指令没有把他们弹出，因为我们强行改为内核态，会让cpu认为没有发生特权级转换。于是，%esp的值就不对了（%ss的值是对的）。但是，解决办法很简单，因为%ebp的值是正确的，而执行int指令前，%ebp和%esp的值是一样的。因为，只要把%ebp的值传给%esp就会使栈顶位置正确归位，于是就顺利完成了调用。

init.c中的switch_to_use：

	static void
	lab1_switch_to_user(void) {
	    //LAB1 CHALLENGE 1 : TODO
	    asm volatile(
	        //设置新栈顶指向switchktou，当返回出栈，则出栈switchktou 中的值。
	        "sub $0x8, %%esp \n" 
	        "int %0 \n"//调用 T_SWITCH_TOU 中断
	        "movl %%ebp, %%esp" //恢复栈指针
	   :
	   : "i"(T_SWITCH_TOU)
	    );
	}

调用了T_SWITCH_TOU中断：

	case T_SWITCH_TOU:
        if (tf->tf_cs != USER_CS) {
            //当前在内核态，需要建立切换到用户态所需的trapframe结构的数据switchktou
            switchktou = *tf;    //设置switchktou
            //将cs，ds，es，ss设置为用户态。
            switchktou.tf_cs = USER_CS;
            switchktou.tf_ds = switchktou.tf_es = switchktou.tf_ss = USER_DS;
            switchktou.tf_esp = (uint32_t)tf + sizeof(struct trapframe) - 8;
		    //设置EFLAG的I/O特权位，使得在用户态可使用in/out指令
            switchktou.tf_eflags |= FL_IOPL_MASK;
            //设置临时栈，指向switchktou，这样iret返回时，CPU会从switchktou恢复数据，而不是从现有栈恢复数据。
            *((uint32_t *)tf - 1) = (uint32_t)&switchktou;
        }

init.c中的switch_to_kernel:
	static void
	lab1_switch_to_kernel(void) {
    //LAB1 CHALLENGE 1 :  TODO
    //发出中断时，CPU处于用户态，我们希望处理完此中断后，CPU继续在内核态运行，
    //把tf->tf_cs和tf->tf_ds都设置为内核代码段和内核数据段
	asm volatile (
	    "int %0 \n"//调用T_SWITCH_TOK号中断。
	    "movl %%ebp, %%esp \n"//因为我们强行改为内核态，会让cpu认为没有发生特权级转换。于是，%esp的值就不对了
	    : 
	    : "i"(T_SWITCH_TOK)
	);
}

调用了T_SWITCH_TOK中断：

	case T_SWITCH_TOK:
    if (tf->tf_cs != KERNEL_CS) {
        //发出中断时，CPU处于用户态，我们希望处理完此中断后，CPU继续在内核态运行，
        //所以把tf->tf_cs和tf->tf_ds都设置为内核代码段和内核数据段
      tf->tf_cs = KERNEL_CS;
      tf->tf_ds = tf->tf_es = KERNEL_DS;
      //设置EFLAGS，让用户态不能执行in/out指令
      tf->tf_eflags &= ~FL_IOPL_MASK;

      switchutok = (struct trapframe *)(tf->tf_esp - (sizeof(struct trapframe) - 8));
      //相当于在栈中挖出sizeof(tf-8)的空间
      memmove(switchutok, tf, sizeof(struct trapframe) - 8);
      *((uint32_t *)tf - 1) = (uint32_t)switchutok;
     }
     break;