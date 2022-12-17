# makefile语法格式

makefile就是一个深搜的过程，最上面的语句是顶级目标，顶级目标还有依赖

如果依赖不存在，下面我们还要写，

所以就是上面没有的，要在下面实现，再下面都实现了，上面的顶级目标才能实现

# 生成QEMU可执行文件

make qemu

qemu 依赖于kernel 和fs.img，把内核加载进去，文件系统挂载进去，之后一个操作系统就可以跑起来了

模拟risc-v指令集的CPU，比较关键的就是-kernel $K/kernel和-driver file=fs.img

```makefile

QEMU = qemu-system-riscv64	# 指定QEMU版本risc-v 的CPU
# --》 指定了使用的操作内核是kernel/kernel,-m 模拟了操作系统使用的内存128M，使用了3个cpu个数，
QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS) -nographic
# 
QEMUOPTS += -global virtio-mmio.force-legacy=false
# 把文件系统挂载上去，最终就是模拟出来的一个计算机
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

ifeq ($(LAB),net)
QEMUOPTS += -netdev user,id=net0,hostfwd=udp::$(FWDPORT)-:2000 -object filter-dump,id=net0,netdev=net0,file=packets.pcap
QEMUOPTS += -device e1000,netdev=net0,bus=pcie.0
endif

# qemu依赖于kernel， fs.img  
qemu: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)------->这里有了操作系统和用户程序还缺少硬件

```

![](https://cdn.jsdelivr.net/gh/zevin02/picb@master/imgss/20221217212740.png)

## 生成kernel可执行文件

```makefile
$K/kernel: $(OBJS) $(OBJS_KCSAN) $K/kernel.ld $U/initcode
# 把所有.o文件用kernel.ld配置的链接器进行链接起来,生成一个kernel
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $(OBJS_KCSAN)
# 把kernel反汇编成kernel.asm，让我们能够进行debug
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
# 把asm中的一些数据进行过滤，方便进行查找  
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

```


### 生成kernel下的OBJS

kernel下的许多程序函数，都需要在 `kernel/main.c`函数中使用

(1)编译目标定义

```makefile

# 展开kernel/entry.o
# s\换行符
# 我们可以把OBJS这个变量别名理解为一个string
# 这里是编译内核态的代码，kernel依赖于这些代码
# 所以这下面的要一个一个开始生成
OBJS = \
  $K/entry.o \
  $K/kalloc.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trampoline.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/bio.o \
  $K/fs.o \
  $K/log.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/pipe.o \
  $K/exec.o \
  $K/sysfile.o \
  $K/kernelvec.o \
  $K/plic.o \
  $K/virtio_disk.o
```

> %.o就是一个通配符，所有的.o都依赖于.c文件,这些都是kernel下的程序

```makefile
$K/%.o: $K/%.c
	$(CC) $(CFLAGS) $(EXTRAFLAG) -c -o $@ $<
```

.S汇编都是下面这样生成的

> riscv64-linux-gnu-gcc    -c -o kernel/entry.o kernel/entry.S

.c就是下面这样编译的，规则是不一样的

> riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o kernel/kalloc.o kernel/kalloc.c

到这里整个 `kernel`的 `OBJS` 都被build

---

### kernel.ld

这是kernel目录底下的链接脚本，指导着我们把kernel的依赖文件链接成一个目标文件

链接器 `ld `将按照脚本内的指令将 `.o`文件生成可执行文件

主要描述的就是处理链接脚本的方式，以及生成可执行文件的内容布局

```
OUTPUT_ARCH( "riscv" )
ENTRY( _entry )虚拟地址的路口
这下面都是虚拟地址，这个就是kernel的虚拟地址空间
SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   */
  . = 0x80000000;这个就是设置entry入口为0x80000000，“."就是当前位置
  /*
   * 这里text里面存放的就是用户的代码

  */
  .text : {
    *(.text .text.*)把目标文件中的所有.o文件中的text节都拿出来，生成一个全新的text节
    . = ALIGN(0x1000);做一个4KB对齐
    _trampoline = .;保存trampoline代码，记录当前位置
    *(trampsec)	保存trampoline代码
    . = ALIGN(0x1000);
    ASSERT(. - _trampoline == 0x1000, "error: trampoline larger than one page");
    PROVIDE(etext = .);
  }

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  }

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  }

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  }

  PROVIDE(end = .);
}
```

![](https://cdn.jsdelivr.net/gh/zevin02/picb@master/imgss/SmartSelect_20221217_004333_Samsung%20Notes.jpg)

![](https://cdn.jsdelivr.net/gh/zevin02/picb@master/imgss/20221217005945.png)

### build OBJS_KCSAN

这些程序都是处理和硬件中断相关的程序

```makefile
OBJS_KCSAN = \
  $K/start.o \
  $K/console.o \
  $K/printf.o \
  $K/uart.o \
  $K/spinlock.o

```

```makefile
$K/%.o: $K/%.c
	$(CC) $(CFLAGS) $(EXTRAFLAG) -c -o $@ $<
```

### build initcode

用户空间初始化程序

在 `kernel/main.c`中使用到了这个程序，执行**第一个用户**程序 `"init"`程序,这一段可加可不加，kernel编译中只加了这一部分

```c
// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
    0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
    0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
    0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
    0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

```

```makefile

$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.o > $U/initcode.asm
```


## 生成一个fs.img文件系统

这个就是生成一个文件系统,相当于一个硬盘的镜像（用来存放用户程序的)

> 这里的mkfs/mkfs程序，就是将后面的$(UPROGS),$(UEXTRA)用户编译好的程序,写入 `fs.img`这个文件系统中

```makefile
# 这些都是伪目标，可以直接使用
fs.img: mkfs/mkfs README $(UEXTRA) $(UPROGS)
	mkfs/mkfs fs.img README $(UEXTRA) $(UPROGS)
```

> mkfs/mkfs fs.img README  user/_cat user/_echo user/_forktest user/_grep user/_init user/_kill user/_ln user/_ls user/_mkdir user/_rm user/_sh user/_stressfs user/_usertests user/_grind user/_wc user/_zombie  user/_pgtbltest

### mkfs

```makefile
mkfs/mkfs: mkfs/mkfs.c $K/fs.h $K/param.h
	gcc $(XCFLAGS) -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c
```

`mkfs/mkfs`就是往文件系统中写文件的程序

> gcc -DSOL_PGTBL -DLAB_PGTBL -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

### 用户程序的编译

```makefile
UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\

ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o	#  这个对标的就是C语言中的一些库函数，如printf，malloc之类的
# 这里的_就是一个字符，后面的%就是匹配所有以_开头的文件
# 依赖中的%.o,就是匹配所有.o结尾的文件，同时还要添加一个ULIB,依次去匹配这些东西
_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

```

`ulib`中包含的就是一些string和内存操作的一些库函数,`printf`  包含了和标准输出相关的一些函数

`umalloc`包含了malloc相关的动态开辟之类的函数

`usys`里面包含的就是系统调用相关的入口函数

这些将来就可以被OS调用，从文件系统中加载到内存中进行执行

用户态生成这些应用文件

硬盘构建完了上面这些在mkfs中使用到的编译完之后，就能构建fs.img

# 配置工具

指定工具的版本，如果找不到合适的版本就输出错误信息

```makefile
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
```

配置编译器，汇编器，链接器，copy工具，dump工具（反汇编)

---

```makefile

CC = $(TOOLPREFIX)gcc		#  指定编译器
AS = $(TOOLPREFIX)gas		#  汇编器
LD = $(TOOLPREFIX)ld		#  链接器，前面都加上工具的版本
OBJCOPY = $(TOOLPREFIX)objcopy	# 把一个目标文件的内容拷贝到另一个目标文件中
OBJDUMP = $(TOOLPREFIX)OBJDUMP	# objdump是把二进制文件反汇编成一个.asm文件
```
