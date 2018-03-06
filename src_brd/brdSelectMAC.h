#ifndef _BRD_SELECT_MAC_H
#define _BRD_SELECT_MAC_H

uint8_t  MAC1[] = {0x44, 0x44, 0x45, 0x55, 0x55, 0x66};
uint8_t  MAC2[] = {0x11, 0x11, 0x12, 0x22, 0x22, 0x33};

#ifdef USE_MDR1986VE3
  #define MAC_DEST    MAC1
  #define MAC_SRC     MAC2
#else
  #define MAC_DEST    MAC2
  #define MAC_SRC     MAC1
#endif

#define FRAME_LEN       150


#endif //_BRD_SELECT_MAC_H
