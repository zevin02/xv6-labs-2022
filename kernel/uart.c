//
// low-level driver routines for 16550a UART.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// the UART control registers are memory-mapped
// at address UART0. this macro returns the
// address of one of the registers.
#define Reg(reg) ((volatile unsigned char *)(UART0 + reg))

// the UART control registers.
// some have different meanings for
// read vs write.
// see http://byterunner.com/16550.html
#define RHR 0                 // receive holding register (for input bytes)，这下面的这些字符如果有的话，可以从这个RHR寄存器读取
#define THR 0                 // transmit holding register (for output bytes)
#define IER 1                 // interrupt enable register
#define IER_RX_ENABLE (1<<0)
#define IER_TX_ENABLE (1<<1)
#define FCR 2                 // FIFO control register
#define FCR_FIFO_ENABLE (1<<0)
#define FCR_FIFO_CLEAR (3<<1) // clear the content of the two FIFOs
#define ISR 2                 // interrupt status register
#define LCR 3                 // line control register
#define LCR_EIGHT_BITS (3<<0)
#define LCR_BAUD_LATCH (1<<7) // special mode to set baud rate
#define LSR 5                 // line status register,指示输入的字符是否正在等待软件读取的位
#define LSR_RX_READY (1<<0)   // input is waiting to be read from RHR
#define LSR_TX_IDLE (1<<5)    // THR can accept another character to send

#define ReadReg(reg) (*(Reg(reg)))  //  
#define WriteReg(reg, v) (*(Reg(reg)) = (v))  

// the transmit output buffer.
struct spinlock uart_tx_lock;//uart只有一把锁，我们可以认为是一个粗粒度的锁设计
#define UART_TX_BUF_SIZE 32
char uart_tx_buf[UART_TX_BUF_SIZE];
uint64 uart_tx_w; // write next to uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE]，为consumer提供的读指针
uint64 uart_tx_r; // read next from uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE]，为producer提供的写指针,来构建一个环形buffer队列，指向下一个需要被传输的位置
//现在是有两个指针，读指针内容是需要被显示，写指针接收例如printf的数据，

extern volatile int panicked; // from printf.c

void uartstart();

void
uartinit(void)//这个函数实际上就是配置好了UART芯片，使其能够使用
{
  
  // disable interrupts.
  // 关闭中断
  WriteReg(IER, 0x00);

  //设置波特率（串口线的传输速率）
  // special mode to set baud rate.
  WriteReg(LCR, LCR_BAUD_LATCH);

  // LSB for baud rate of 38.4K.
  WriteReg(0, 0x03);

  // MSB for baud rate of 38.4K.
  WriteReg(1, 0x00);

  // leave set-baud mode,
  // and set word length to 8 bits, no parity.
  // 设置字符长度为8
  WriteReg(LCR, LCR_EIGHT_BITS);

  // reset and enable FIFOs.
  //重置并打开FIFO
  WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

  // enable transmit and receive interrupts.
  //再打开中断，IER就是用来打开中断的寄存器,关于IER的所有的寄存器操作都打开
  WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

  initlock(&uart_tx_lock, "uart");
  // 运行完这个uarinit函数之后，原则上UART就可以产生中断了，但是因为我们没有对PLIC编程，所以中断不能够被CPU感知，
}

// add a character to the output buffer and tell the
// UART to start sending if it isn't already.
// blocks if the output buffer is full.
// because it may block, it can't be called
// from interrupts; it's only suitable for use
// by write().
void
uartputc(int c)
{
  acquire(&uart_tx_lock);
  //这个地方如果没有关闭中断，此时这个uartputc拿着这把锁，在uart传输完字符后，就会发送一个中断，到uartintr，也会索要锁，就会造成死锁，同一个cpu对一把锁申请两次
  if(panicked){
    for(;;)
      ;
  }
  //如果读写指针指向同一个位置，说明是空的
  while(uart_tx_w == uart_tx_r + UART_TX_BUF_SIZE){//给定的缓冲区满了
  //因为是一个环形数组，所以r再w的前一个，就说明满了
    // buffer is full.
    // wait for uartstart() to open up space in the buffer.
    sleep(&uart_tx_r, &uart_tx_lock);//满了就休眠
  }
  uart_tx_buf[uart_tx_w % UART_TX_BUF_SIZE] = c;//再UART内部有一个buffer用来发送数据，buffer大小32字符，
  uart_tx_w += 1;//shell被认为是一个生产者,所以会调用这个函数
  uartstart();//调用这个函数来启动设备传输
  release(&uart_tx_lock);
}


// alternate version of uartputc() that doesn't 
// use interrupts, for use by kernel printf() and
// to echo characters. it spins waiting for the uart's
// output register to be empty.
void
uartputc_sync(int c)
{
  push_off();

  if(panicked){
    for(;;)
      ;
  }

  // wait for Transmit Holding Empty to be set in LSR.
  while((ReadReg(LSR) & LSR_TX_IDLE) == 0)
    ;
  WriteReg(THR, c);

  pop_off();
}

// if the UART is idle, and a character is waiting
// in the transmit buffer, send it.
// caller must hold uart_tx_lock.
// called from both the top- and bottom-half.
void
uartstart()//这个函数就是通知设备执行操作，首先就是检查当前设备是否空闲，空闲的话，从buffer里面读数据，
{
  //通常第一个字节都是通过uartputs来调用这个函数，而后面都是通过uartintr中断来调用这个函数的
  while(1){
    if(uart_tx_w == uart_tx_r){
      // transmit buffer is empty.
      // 读写指针指向同一个位置，说明数据都读取完了
      return;
    }
    
    if((ReadReg(LSR) & LSR_TX_IDLE) == 0){
      // the UART transmit holding register is full, 
      // so we cannot give it another byte.
      // it will interrupt when it's ready for a new byte.
      // 数据满了，我们也不能继续给一个字节，THR必须不能是满的
      return;
    }
    //这个锁保证了，在下一个字符被写到缓存里面时，可以处理完缓存里的数据
    int c = uart_tx_buf[uart_tx_r % UART_TX_BUF_SIZE];//从缓冲区里面读数据，这里面的数据都是我们需要进行输出到显示机器上的
    uart_tx_r += 1;//读指针向后走，一直循环往后发送，直到缓存没数据或者发送reg满了
    
    // maybe uartputc() is waiting for space in the buffer.
    wakeup(&uart_tx_r);
    
    WriteReg(THR, c);//如果有数据的话，就把他写到THR里面，发送给寄存器，相当于告诉设备，这里有一个字节需要发送，一旦发送到设备，应用程序的shell就能继续执行
    //这里锁确保了THR寄存器只有一个写入者
  }
}

// read one input character from the UART.
// return -1 if none is waiting.
int
uartgetc(void)
{
  if(ReadReg(LSR) & 0x01){
    // input data is ready.
    return ReadReg(RHR);//从里面获取数据
  } else {
    return -1;
  }
}

// handle a uart interrupt, raised because input has
// arrived, or the uart is ready for more output, or
// both. called from devintr().
void
uartintr(void)
{
  // read and process incoming characters.
  while(1){
    int c = uartgetc();//从uart里面读取数据，把这个读取的数据再放到UART
    if(c == -1)
      break;
    consoleintr(c);
  }
  //因为我们现在键盘里面还没有输入任何东西，所以直接就跳转到这里
  // send buffered characters.
  acquire(&uart_tx_lock);//这个地方获得锁，是因为在printf调用时，也会运行uartstart，我们要确保THR寄存器只有一个写入者，所以这里也需要上锁，和上面从putc进去成为相同的情况
  uartstart();
  release(&uart_tx_lock);
}

//bottom部分通常都是中断处理方法，当一个中断被送到CPU时，设置CPU接收到这个中断，cpu会调用相应的方法，这个不会运行在某个进程的上下文中
//top是用户进程，给内核其他部分调用的接口