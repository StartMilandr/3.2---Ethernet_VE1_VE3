#ifndef _BRD_CLOCK_H
#define _BRD_CLOCK_H

#include <stdint.h>

#ifdef USE_BOARD_VE_8 
  #include  <mdr32f8_clkctrl.h>
#else  
  #include <MDR32F9Qx_port.h>
  #include  <MDR32F9Qx_rst_clk.h>
#endif

void BRD_Clock_Init_HSE_PLL(uint32_t PLL_Mul);

#ifndef USE_MDR1986VE3
  void BRD_Clock_Init_HSE_dir(void);
#endif

#endif  //_BRD_CLOCK_H
