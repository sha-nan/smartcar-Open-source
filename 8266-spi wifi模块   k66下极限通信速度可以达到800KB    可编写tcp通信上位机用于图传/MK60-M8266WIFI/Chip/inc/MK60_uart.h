#ifndef __MK60_UART_H__
#define __MK60_UART_H__

typedef enum
{
    UART0,
    UART1,
    UART2,
    UART3,
    UART4,
    UART5,

    UART_MAX,
} UARTn_e;

extern volatile struct UART_MemMap *UARTN[UART_MAX];
/********************************************************************/


//初始化
extern void uart_init      (UARTn_e, uint32 baud);             //初始化uartx模块

//接收相关代码
extern void uart_getchar   (UARTn_e, char *ch);                //等待接受1个字节

extern char uart_querychar (UARTn_e, char *ch);                //查询接收1个字符
extern char uart_querystr  (UARTn_e, char *str, uint32 max_len); //查询接收字符串
extern char uart_querybuff (UARTn_e, char *str, uint32 max_len); //查询接收buff

extern char uart_query    (UARTn_e);                          //查询是否接收到一个字节


//发送相关代码
extern void uart_putchar   (UARTn_e, char ch);                 //发送1个字节
extern void uart_putbuff   (UARTn_e , uint8 *buff, uint32 len);//发送len个字节buff
extern void uart_putstr    (UARTn_e , const uint8 *str);       //发送字符串



//中断相关代码
extern void uart_rx_irq_en (UARTn_e);                          //开串口接收中断
extern void uart_tx_irq_en (UARTn_e);                          //开串口发送中断
extern void uart_txc_irq_en (UARTn_e);                         //开串口发送完成中断

extern void uart_rx_irq_dis(UARTn_e);                          //关串口接收中断
extern void uart_tx_irq_dis(UARTn_e);                          //关串口发送中断
extern void uart_txc_irq_dis(UARTn_e);                         //关串口发送完成中断

extern void uart3_test_handler(void);                        //中断复位函数，仅供参考（需用户自行实现）

/********************************************************************/

#endif /* __MK60_UART_H__ */
