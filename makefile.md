# 生成QEMU可执行文件

## 生成kernel可执行文件

### 生成kernel下的OBJS

.S汇编都是下面这样生成的

> riscv64-linux-gnu-gcc    -c -o kernel/entry.o kernel/entry.S

.cpp就是下面这样编译的，规则是不一样的

> riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o kernel/kalloc.o kernel/kalloc.c

riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/string.o` `kernel/string.c`
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/main.o` `kernel/main.c`
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/vm.o kernel/vm.c`
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o ` kernel/proc.o` kernel/proc.c
riscv64-linux-gnu-gcc    -c -o `kernel/swtch.o `kernel/swtch.S
riscv64-linux-gnu-gcc    -c -o `kernel/trampoline.o` kernel/trampoline.S
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/trap.o` kernel/trap.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/syscall.o` kernel/syscall.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/sysproc.o` kernel/sysproc.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/bio.o` kernel/bio.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/fs.o` kernel/fs.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/log.o `kernel/log.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/sleeplock.o `kernel/sleeplock.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/file.o` kernel/file.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/pipe.o` kernel/pipe.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/exec.o` kernel/exec.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/sysfile.o `kernel/sysfile.c
riscv64-linux-gnu-gcc    -c -o `kernel/kernelvec.o `kernel/kernelvec.S riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/plic.o `kernel/plic.c riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o `kernel/virtio_disk.o` kernel/virtio_disk.c


到这里整个 `kernel`的 `OBJS` 都被build完成了

---




### build OBJS_KCSAN


riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o kernel/start.o kernel/start.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o kernel/console.o kernel/console.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o kernel/printf.o kernel/printf.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o kernel/uart.o kernel/uart.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie  -c -o kernel/spinlock.o kernel/spinlock.c







### build initcode


生成一个initcode.o文件

> riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie -march=rv64g -nostdinc -I. -Ikernel -c user/initcode.S -o user/initcode.o

用链接器生成initcode.out可执行文件

> riscv64-linux-gnu-ld -z max-page-size=4096 -N -e start -Ttext 0 -o user/initcode.out user/initcode.o

把initcode.out拷贝到initcode里面

> riscv64-linux-gnu-objcopy -S -O binary user/initcode.out user/initcode

把initcode.o反汇编出initcode.asm

> riscv64-linux-gnu-objdump -S user/initcode.o > user/initcode.asm

---


所有build kernel的前值任务

把所有.o文件链接起来生成一个可执行文件

riscv64-linux-gnu-ld -z max-page-size=4096 -T kernel/kernel.ld -o kernel/kernel kernel/entry.o kernel/kalloc.o kernel/string.o kernel/main.o kernel/vm.o kernel/proc.o kernel/swtch.o kernel/trampoline.o kernel/trap.o kernel/syscall.o kernel/sysproc.o kernel/bio.o kernel/fs.o kernel/log.o kernel/sleeplock.o kernel/file.o kernel/pipe.o kernel/exec.o kernel/sysfile.o kernel/kernelvec.o kernel/plic.o kernel/virtio_disk.o kernel/start.o kernel/console.o kernel/printf.o kernel/uart.o kernel/spinlock.o

生成一个.asm反汇编

riscv64-linux-gnu-objdump -S kernel/kernel > kernel/kernel.asm

指定了一些字符串过滤，后输出到kernel.sym

riscv64-linux-gnu-objdump -t kernel/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > kernel/kernel.sym

---

## 生成一个fs.img可执行文件

这个就是生成一个文件系统

这个可执行文件依赖于mkfs,README ,user的可执行文件,以及如果lab=util的话，还要有UPROGS

### mkfs

生成mkfs,就还需要有user中的一些可执行文件

gcc -DSOL_PGTBL -DLAB_PGTBL -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c


#### mkfs中使用了很多函数,UPROGS

用户态生成这些应用文件

riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/ulib.o user/ulib.c
perl user/usys.pl > user/usys.S
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie -c -o user/usys.o user/usys.S
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/printf.o user/printf.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/umalloc.o user/umalloc.c
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/cat.o user/cat.c



把这些应用文件链接起来
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_cat user/cat.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_cat > user/cat.asm
riscv64-linux-gnu-objdump -t user/_cat | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/cat.sym



继续生成用户的文件
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/echo.o user/echo.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_echo user/echo.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_echo > user/echo.asm
riscv64-linux-gnu-objdump -t user/_echo | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/echo.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/forktest.o user/forktest.c


forktest has less library code linked in - needs to be small

in order to be able to max out the proc table.

riscv64-linux-gnu-ld -z max-page-size=4096 -N -e main -Ttext 0 -o user/_forktest user/forktest.o user/ulib.o user/usys.o
riscv64-linux-gnu-objdump -S user/_forktest > user/forktest.asm
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/grep.o user/grep.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_grep user/grep.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_grep > user/grep.asm
riscv64-linux-gnu-objdump -t user/_grep | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/grep.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/init.o user/init.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_init user/init.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_init > user/init.asm
riscv64-linux-gnu-objdump -t user/_init | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/init.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/kill.o user/kill.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_kill user/kill.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_kill > user/kill.asm
riscv64-linux-gnu-objdump -t user/_kill | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/kill.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/ln.o user/ln.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_ln user/ln.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_ln > user/ln.asm
riscv64-linux-gnu-objdump -t user/_ln | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/ln.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/ls.o user/ls.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_ls user/ls.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_ls > user/ls.asm
riscv64-linux-gnu-objdump -t user/_ls | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/ls.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/mkdir.o user/mkdir.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_mkdir user/mkdir.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_mkdir > user/mkdir.asm
riscv64-linux-gnu-objdump -t user/_mkdir | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/mkdir.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/rm.o user/rm.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_rm user/rm.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_rm > user/rm.asm
riscv64-linux-gnu-objdump -t user/_rm | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/rm.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/sh.o user/sh.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_sh user/sh.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_sh > user/sh.asm
riscv64-linux-gnu-objdump -t user/_sh | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/sh.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/stressfs.o user/stressfs.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_stressfs user/stressfs.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_stressfs > user/stressfs.asm
riscv64-linux-gnu-objdump -t user/_stressfs | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/stressfs.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/usertests.o user/usertests.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_usertests user/usertests.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_usertests > user/usertests.asm
riscv64-linux-gnu-objdump -t user/_usertests | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/usertests.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/grind.o user/grind.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_grind user/grind.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_grind > user/grind.asm
riscv64-linux-gnu-objdump -t user/_grind | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/grind.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/wc.o user/wc.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_wc user/wc.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_wc > user/wc.asm
riscv64-linux-gnu-objdump -t user/_wc | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/wc.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/zombie.o user/zombie.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_zombie user/zombie.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_zombie > user/zombie.asm
riscv64-linux-gnu-objdump -t user/_zombie | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/zombie.sym
riscv64-linux-gnu-gcc -Wall -Werror -O -fno-omit-frame-pointer -ggdb -gdwarf-2 -DSOL_PGTBL -DLAB_PGTBL -MD -mcmodel=medany -ffreestanding -fno-common -nostdlib -mno-relax -I. -fno-stack-protector -fno-pie -no-pie   -c -o user/pgtbltest.o user/pgtbltest.c
riscv64-linux-gnu-ld -z max-page-size=4096 -T user/user.ld -o user/_pgtbltest user/pgtbltest.o user/ulib.o user/usys.o user/printf.o user/umalloc.o
riscv64-linux-gnu-objdump -S user/_pgtbltest > user/pgtbltest.asm
riscv64-linux-gnu-objdump -t user/_pgtbltest | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$/d' > user/pgtbltest.sym



硬盘构建完了上面这些在mkfs中使用到的编译完之后，就能构建fs.img
mkfs/mkfs fs.img README  user/_cat user/_echo user/_forktest user/_grep user/_init user/_kill user/_ln user/_ls user/_mkdir user/_rm user/_sh user/_stressfs user/_usertests user/_grind user/_wc user/_zombie  user/_pgtbltest




nmeta 46 (boot, super, log blocks 30 inode blocks 13, bitmap blocks 1) blocks 1954 total 2000
balloc: first 789 blocks have been allocated
balloc: write bitmap block at sector 45



## 执行qemu

qemu-system-riscv64 -machine virt -bios none -kernel kernel/kernel -m 128M -smp 3 -nographic -global virtio-mmio.force-legacy=false -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0


xv6 kernel is booting

hart 1 starting
hart 2 starting
page table 0x0000000087f6b000
..0: pte 0x0000000021fd9c01 pa 0x0000000087f67000
.. ..0: pte 0x0000000021fd9801 pa 0x0000000087f66000
.. .. ..0: pte 0x0000000021fda01b pa 0x0000000087f68000
.. .. ..1: pte 0x0000000021fd9417 pa 0x0000000087f65000
.. .. ..2: pte 0x0000000021fd9007 pa 0x0000000087f64000
.. .. ..3: pte 0x0000000021fd8c17 pa 0x0000000087f63000
..255: pte 0x0000000021fda801 pa 0x0000000087f6a000
.. ..511: pte 0x0000000021fda401 pa 0x0000000087f69000
.. .. ..509: pte 0x0000000021fdcc13 pa 0x0000000087f73000
.. .. ..510: pte 0x0000000021fdd007 pa 0x0000000087f74000
.. .. ..511: pte 0x0000000020001c0b pa 0x0000000080007000
init: starting sh
