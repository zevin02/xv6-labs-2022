
# To compile and run with a lab solution, set the lab name in conf/lab.mk
# (e.g., LAB=util).  Run make grade to test solution with the lab's
# grade script (e.g., grade-lab-util).

-include conf/lab.mk	# 定义是哪个lab，定义LAB，我们后面就可以使用这个LAB了

K=kernel				# 定义前导目录，方便我们写
U=user

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

OBJS_KCSAN = \
  $K/start.o \
  $K/console.o \
  $K/printf.o \
  $K/uart.o \
  $K/spinlock.o

ifdef KCSAN
OBJS_KCSAN += \
	$K/kcsan.o
endif

ifeq ($(LAB),$(filter $(LAB), lock))	#if(LAB==pgtbl||LAB==lock),就OBJS继续往后增加两个目标文件
OBJS += \
	$K/stats.o\
	$K/sprintf.o
endif


ifeq ($(LAB),net)		# if(LAB==net),就把下面这些文件也加入到OBJS中
OBJS += \
	$K/e1000.o \
	$K/net.o \
	$K/sysnet.o \
	$K/pci.o
endif


# riscv64-unknown-elf- or riscv64-linux-gnu-
# perhaps in /opt/riscv/bin
#TOOLPREFIX = 

# Try to infer the correct TOOLPREFIX if not set
# 我们可以在makefile、中调用shell command
# 尝试区看如果是的话，就把TOOLPREFIX设置成对应的值，echo就是把TOOLPREFIX设置成那个值
# 最下面如果都没有的话，就会报一个错误
# shell 函数，用来执行shell命令，把返回值赋值给=左边
ifndef TOOLPREFIX
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
endif

QEMU = qemu-system-riscv64	# 指定QEMU版本

CC = $(TOOLPREFIX)gcc		#  指定编译器
AS = $(TOOLPREFIX)gas		#  汇编器
LD = $(TOOLPREFIX)ld		#  链接器，前面都加上工具的版本
OBJCOPY = $(TOOLPREFIX)objcopy	# 把一个目标文件的内容拷贝到另一个目标文件中
OBJDUMP = $(TOOLPREFIX)OBJDUMP	# objdump是把二进制文件反汇编成一个.asm文件

CFLAGS = -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2	# 指定一些标志

# 如果定义了LAB，就执行下面的操作
ifdef LAB
LABUPPER = $(shell echo $(LAB) | tr a-z A-Z)
XCFLAGS += -DSOL_$(LABUPPER) -DLAB_$(LABUPPER)
endif

CFLAGS += $(XCFLAGS)
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)

ifeq ($(LAB),net)
CFLAGS += -DNET_TESTS_PORT=$(SERVERPORT)
endif

ifdef KCSAp
CFLAGS += -DKCSAN
KCSANFLAG = -fsanitize=thread
endif

# Disable PIE when possible (for Ubuntu 16.10 toolchain)
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]no-pie'),)
CFLAGS += -fno-pie -no-pie
endif
ifneq ($(shell $(CC) -dumpspecs 2>/dev/null | grep -e '[^f]nopie'),)
CFLAGS += -fno-pie -nopie
endif

LDFLAGS = -z max-page-size=4096

# 我们想要生成的目标文件是kernel/kernel,依赖于右边的这些东西
# 把所有的目标文件给总和在一起，我们需要kernel.ld,生成的叫做kernel,依赖于objs,objs_kscan
# objdump -S参数就是把二进制文件转化成汇编文件
$K/kernel: $(OBJS) $(OBJS_KCSAN) $K/kernel.ld $U/initcode
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $(OBJS_KCSAN)		
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm    
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

$(OBJS): EXTRAFLAG := $(KCSANFLAG)

$K/%.o: $K/%.c
	$(CC) $(CFLAGS) $(EXTRAFLAG) -c -o $@ $<


$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode
	$(OBJDUMP) -S $U/initcode.o > $U/initcode.asm

tags: $(OBJS) _init
	etags *.S *.c

ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o	#  用户层面的东西

ifeq ($(LAB),$(filter $(LAB), lock))
ULIB += $U/statistics.o
endif

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym
# 用perl来生成 ，把user.pl写成usey.S 
# 用gcc，依赖.S,生成.o
$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S			 

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S	
$U/_forktest: $U/forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o
	$(OBJDUMP) -S $U/_forktest > $U/forktest.asm

mkfs/mkfs: mkfs/mkfs.c $K/fs.h $K/param.h
	gcc $(XCFLAGS) -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o
# 用户层面的东西，目标层序
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



# 如果filter,在，右边找和左边重复的元素
ifeq ($(LAB),$(filter $(LAB), lock))
UPROGS += \
	$U/_stats
endif

ifeq ($(LAB),traps)
UPROGS += \
	$U/_call\
	$U/_bttest
endif

ifeq ($(LAB),lazy)
UPROGS += \
	$U/_lazytests
endif

ifeq ($(LAB),cow)
UPROGS += \
	$U/_cowtest
endif

ifeq ($(LAB),thread)
UPROGS += \
	$U/_uthread

$U/uthread_switch.o : $U/uthread_switch.S
	$(CC) $(CFLAGS) -c -o $U/uthread_switch.o $U/uthread_switch.S

$U/_uthread: $U/uthread.o $U/uthread_switch.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_uthread $U/uthread.o $U/uthread_switch.o $(ULIB)
	$(OBJDUMP) -S $U/_uthread > $U/uthread.asm
# 这些都是伪目标
ph: notxv6/ph.c
	gcc -o ph -g -O2 $(XCFLAGS) notxv6/ph.c -pthread

barrier: notxv6/barrier.c
	gcc -o barrier -g -O2 $(XCFLAGS) notxv6/barrier.c -pthread
endif

ifeq ($(LAB),pgtbl)
UPROGS += \
	$U/_pgtbltest
endif

ifeq ($(LAB),lock)
UPROGS += \
	$U/_kalloctest\
	$U/_bcachetest
endif

ifeq ($(LAB),fs)
UPROGS += \
	$U/_bigfile
endif



ifeq ($(LAB),net)
UPROGS += \
	$U/_nettests
endif

UEXTRA=
ifeq ($(LAB),util)
	UEXTRA += user/xargstest.sh
endif

# 这些都是伪目标，可以直接使用
fs.img: mkfs/mkfs README $(UEXTRA) $(UPROGS)
	mkfs/mkfs fs.img README $(UEXTRA) $(UPROGS)
# .d就是依赖文件，和.mk一样都是依赖文件
-include kernel/*.d user/*.d

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*/*.o */*.d */*.asm */*.sym \
	$U/initcode $U/initcode.out $K/kernel fs.img \
	mkfs/mkfs .gdbinit \
        $U/usys.S \
	$(UPROGS) \
	ph barrier

# try to generate a unique GDB port
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
# QEMU's gdb stub command line changed in 0.11
QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
	then echo "-gdb tcp::$(GDBPORT)"; \
	else echo "-s -p $(GDBPORT)"; fi)
ifndef CPUS# 定义CPU个数
CPUS := 3
endif
ifeq ($(LAB),fs)# 如果是fs的话，CPU就直接指定为1
CPUS := 1
endif

FWDPORT = $(shell expr `id -u` % 5000 + 25999)
# 这些都是make qemu的以一些参数
# 这里就是模拟的一些硬件
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
	$(QEMU) $(QEMUOPTS)

.gdbinit: .gdbinit.tmpl-riscv
	sed "s/:1234/:$(GDBPORT)/" < $^ > $@

qemu-gdb: $K/kernel .gdbinit fs.img
	@echo "*** Now run 'gdb' in another window." 1>&2
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

ifeq ($(LAB),net)
# try to generate a unique port for the echo server
SERVERPORT = $(shell expr `id -u` % 5000 + 25099)

server:
	python3 server.py $(SERVERPORT)

ping:
	python3 ping.py $(FWDPORT)
endif

##
##  FOR testing lab grading script
##

ifneq ($(V),@)
GRADEFLAGS += -v
endif

print-gdbport:
	@echo $(GDBPORT)

grade:
	@echo $(MAKE) clean
	@$(MAKE) clean || \
          (echo "'make clean' failed.  HINT: Do you have another running instance of xv6?" && exit 1)
	./grade-lab-$(LAB) $(GRADEFLAGS)

##
## FOR web handin
##


WEBSUB := https://6828.scripts.mit.edu/2022/handin.py

handin: tarball-pref myapi.key
	@SUF=$(LAB); \
	curl -f -F file=@lab-$$SUF-handin.tar.gz -F key=\<myapi.key $(WEBSUB)/upload \
	    > /dev/null || { \
		echo ; \
		echo Submit seems to have failed.; \
		echo Please go to $(WEBSUB)/ and upload the tarball manually.; }

handin-check:
	@if ! test -d .git; then \
		echo No .git directory, is this a git repository?; \
		false; \
	fi
	@if test "$$(git symbolic-ref HEAD)" != refs/heads/$(LAB); then \
		git branch; \
		read -p "You are not on the $(LAB) branch.  Hand-in the current branch? [y/N] " r; \
		test "$$r" = y; \
	fi
	@if ! git diff-files --quiet || ! git diff-index --quiet --cached HEAD; then \
		git status -s; \
		echo; \
		echo "You have uncomitted changes.  Please commit or stash them."; \
		false; \
	fi
	@if test -n "`git status -s`"; then \
		git status -s; \
		read -p "Untracked files will not be handed in.  Continue? [y/N] " r; \
		test "$$r" = y; \
	fi

UPSTREAM := $(shell git remote -v | grep -m 1 "xv6-labs-2022" | awk '{split($$0,a," "); print a[1]}')

tarball: handin-check
	git archive --format=tar HEAD | gzip > lab-$(LAB)-handin.tar.gz

tarball-pref: handin-check
	@SUF=$(LAB); \
	git archive --format=tar HEAD > lab-$$SUF-handin.tar; \
	git diff $(UPSTREAM)/$(LAB) > /tmp/lab-$$SUF-diff.patch; \
	tar -rf lab-$$SUF-handin.tar /tmp/lab-$$SUF-diff.patch; \
	gzip -c lab-$$SUF-handin.tar > lab-$$SUF-handin.tar.gz; \
	rm lab-$$SUF-handin.tar; \
	rm /tmp/lab-$$SUF-diff.patch; \

myapi.key:
	@echo Get an API key for yourself by visiting $(WEBSUB)/
	@read -p "Please enter your API key: " k; \
	if test `echo "$$k" |tr -d '\n' |wc -c` = 32 ; then \
		TF=`mktemp -t tmp.XXXXXX`; \
		if test "x$$TF" != "x" ; then \
			echo "$$k" |tr -d '\n' > $$TF; \
			mv -f $$TF $@; \
		else \
			echo mktemp failed; \
			false; \
		fi; \
	else \
		echo Bad API key: $$k; \
		echo An API key should be 32 characters long.; \
		false; \
	fi;

# 声明这些都是伪目标
.PHONY: handin tarball tarball-pref clean grade handin-check
