#ifndef _BRD_LED__H
#define _BRD_LED__H

#include <stdint.h>

//  Led Pin Definitions
#ifdef USE_BOARD_VE_8 
  #include  <mdr32f8_port.h>
#else  
  #include  <MDR32F9Qx_port.h>
#endif

void BRD_LEDs_Init(void);
void BRD_LED_Switch(uint32_t LEDs_Sel);
void BRD_LED_Set(uint32_t LEDs_Sel, uint32_t isOn);


#endif //_BRD_LED__H
