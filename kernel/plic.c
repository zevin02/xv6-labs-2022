#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

//
// the riscv Platform Level Interrupt Controller (PLIC).
//

void
plicinit(void)//PLIC就是用来路由中断的
{
  //这个函数是由0号CPU运行的
  // main函数里面还需要调用这个函数，
  // set desired IRQ priorities non-zero (otherwise disabled).
  // PLIC占用了一个I/O地址（0XC00）
  *(uint32*)(PLIC + UART0_IRQ*4) = 1;//使UART能中断，这里实际上是接受了PLIC会接受那些中断，进而将中断转到CPU
  *(uint32*)(PLIC + VIRTIO0_IRQ*4) = 1;//接收来自磁盘的中断
}

void
plicinithart(void)//之后每个核都需要调用这个函数表明对哪个外设中断感兴趣
{
  int hart = cpuid();//获取自己的核心号，
  // set enable bits for this hart's S-mode
  // for the uart and virtio disk.
  *(uint32*)PLIC_SENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);//让每个核都表明自己对来自UART和UIRTIO的中断感兴趣,
  

  // set this hart's S-mode priority threshold to 0.
  *(uint32*)PLIC_SPRIORITY(hart) = 0;//我们忽略了中断的优先级，所以这里我们把优先级都设置成了0
  //现在我们有了可以生成中断的外部设备，有了PLIC可以传递中断到每个CPU上，但是CPU自己没有设置好接收中断，要设置sstatus寄存器
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)//表明哪个core对这个中断感兴趣，占据这个core，把中断传递到单个CPU，此时我们就把路由中断搞定，还需要CPU能够接收中断
{
  int hart = cpuid();
  int irq = *(uint32*)PLIC_SCLAIM(hart);//当前CPU核户会告诉PLIC，自己要处理中断，将中断号返回
  return irq;
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
  int hart = cpuid();
  *(uint32*)PLIC_SCLAIM(hart) = irq;
}
