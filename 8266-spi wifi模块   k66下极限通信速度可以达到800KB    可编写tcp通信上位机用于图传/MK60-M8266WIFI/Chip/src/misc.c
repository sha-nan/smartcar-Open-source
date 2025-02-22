#include "common.h"
#include "MK60_uart.h"


/*!
 *  @brief      设置中断向量表地址
 *  @param      vtor    新的中断向量表地址
 *  @since      v5.0
 *  @author     飞思卡尔公司
 *  Sample usage:       write_vtor ((uint32)__VECTOR_RAM);  //新的中断向量地址
 */
void write_vtor (int vtor)
{
    ASSERT(vtor % 0x200 == 0);   //Vector Table base offset field. This value must be a multiple of 0x200.

    /* Write the VTOR with the new value */
    SCB->VTOR = vtor;
}



/*!
 *  @brief      设置中断向量表里的中断复位函数
 *  @since      v5.0
 *  @warning    只有中断向量表位于icf指定的RAM区域时，此函数才有效
 *  Sample usage:       set_vector_handler(UART3_RX_TX_VECTORn , uart3_handler);    //把 uart3_handler 函数添加到中断向量表
 */
void set_vector_handler(VECTORn_t vector , void pfunc_handler(void))
{
    extern uint32 __VECTOR_RAM[];

    ASSERT(SCB->VTOR == (uint32)__VECTOR_RAM);  //断言，检测中断向量表是否在 RAM 里

    __VECTOR_RAM[vector] = (uint32)pfunc_handler;
}

void common_cpy( uint8 *dst, uint8 *src, uint32 count)
{
    uint32 n;
    if(count != 0)
    {
        //printf("-");

        n = (count + 7 ) / 8 ;
        switch (count % 8 )
        {
            do
            {
            case 0 :
                * dst ++ = * src ++ ;
            case 7 :
                * dst ++ = * src ++ ;
            case 6 :
                * dst ++ = * src ++ ;
            case 5 :
                * dst ++ = * src ++ ;
            case 4 :
                * dst ++ = * src ++ ;
            case 3 :
                * dst ++ = * src ++ ;
            case 2 :
                * dst ++ = * src ++ ;
            case 1 :
                * dst ++ = * src ++ ;
            }
            while ( -- n >    0 );
        }
    }
}

