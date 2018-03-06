#include "brdDef.h"
#include "brdClock.h"
#include "brdLed.h"
#include "brdEthernet.h"
#include "brdUtils.h"

#ifdef USE_BOARD_VE_8
  #include "spec.h"
#endif  

//--------      SELECT PAIR  ---------

#define USE_AUTONEG

//--------      SELECT PAIR  ---------

#if   defined (USE_MDR1986VE1T)
  #define IS_STARTER
  #define ETHERNET_X        MDR_ETHERNET1
  
#elif defined (USE_MDR1986VE3)
  //#define IS_STARTER
  #define ETHERNET_X        MDR_ETHERNET1
  
#elif defined (USE_BOARD_VE_8)
  //#define IS_STARTER
  #define ETHERNET_X        MDR_ETH0
  
#endif

//--------      ETHERNET SETTINGS  ---------
uint8_t  MAC1[] = {0x1c, 0x1b, 0x0d, 0x49, 0xe2, 0x14};
uint8_t  MAC2[] = {0x1c, 0x34, 0x56, 0x78, 0x9a, 0xbc};

#ifdef IS_STARTER
  #define MAC_DEST    MAC1
  #define MAC_SRC     MAC2
#else
  #define MAC_DEST    MAC2
  #define MAC_SRC     MAC1
#endif

#define FRAME_LEN       150


//--------      LED Status  ---------
#define LED_TX            BRD_LED_1
#define LED_RX            BRD_LED_2
#define LED_RX_IND_ERR    BRD_LED_3
#define LED_MISSED_F_ERR  BRD_LED_4

#define LED_FRAME_PERIOD  1000

int32_t LED_IndexTX = 0;
int32_t LED_IndexRX = 0;

void LED_CheckSwitch(uint32_t ledSel, int32_t * ledIndex);

//--------      ETHERNET Status  ---------
uint32_t FrameIndexWR = 0;
uint32_t FrameIndexRD = 0;

uint32_t Missed_F_Count = 0;


int main()
{
  ETH_InitTypeDef ETH_InitStruct;
  ETH_StatusPacketReceptionTypeDef StatusRX;
  
  uint32_t doFrameSend;
  volatile uint32_t inpFrameInd;
  volatile uint32_t ethStatus;
  uint8_t *ptrDataTX;
  uint16_t dataCount;
  uint16_t i;
  uint8_t *ptr8;
  uint32_t frameLen = FRAME_LEN;
  
  
#if   defined (USE_MDR1986VE1T)
  BRD_Clock_Init_HSE_PLL(RST_CLK_CPU_PLLmul16); // Clock Max 128 MHz
  
#elif defined (USE_MDR1986VE3)
  BRD_Clock_Init_HSE_PLL(RST_CLK_CPU_PLLmul10); // Clock Max 80 MHz
  
#elif defined (USE_BOARD_VE_8)
  POR_disable();
  BRD_Clock_Init_HSE_PLL(8);                   // Clock Max 100 MHz   
#endif
  
  //  LEDs
  BRD_LEDs_Init();
  
  //  Ethernet
  BRD_ETH_StructInitDef(&ETH_InitStruct, MAC_SRC);
  
#ifdef USE_AUTONEG  
  ETH_InitStruct.ETH_PHY_Mode = ETH_PHY_MODE_AutoNegotiation;
#else  
  ETH_InitStruct.ETH_PHY_Mode = ETH_PHY_MODE_100BaseT_Full_Duplex;
#endif    
  
  BRD_ETH_Init(ETHERNET_X, &ETH_InitStruct); 
  // BRD_ETH_InitIRQ(ETHERNET_X, ETH_MAC_IT_MISSED_F);  // Непрерывная генерация прерывания в rev4. В rev6 не наблюдается
  BRD_ETH_Start(ETHERNET_X);
  
  //  Wait Autonegotiation or Link status
  BRD_LED_Set(LED_TX | LED_RX | LED_RX_IND_ERR, 1);   // Show wait status
#ifdef USE_AUTONEG  
  //  Wait autonegotiation
  BRD_ETH_WaitAutoneg_Completed(ETHERNET_X);
#else  
  BRD_ETH_WaitLink(ETHERNET_X);
#endif   
  BRD_LED_Set(LED_TX | LED_RX | LED_RX_IND_ERR | LED_MISSED_F_ERR, 0); // Show wait completed


#ifdef IS_STARTER
  doFrameSend = 1;    //  Стартер - Запускает обмен: Посылает фрейм к "Приемнику" и ждет от него ответный  
#else
  doFrameSend = 0;    //  Приемник - Отвечает: Ждет фрейм от "Стартера" и посылается ему ответный
#endif

  Missed_F_Count = 0; 
  while(1)
  {
    ethStatus = ETH_GetMACStatusRegister(ETHERNET_X);
    
    //  ------------  Receive Frame -------------
    if (BRD_ETH_TryReadFrame(ETHERNET_X, &StatusRX))
    {     
      ptr8 = (uint8_t*) (FrameRX) + FRAME_HEAD_SIZE;
      inpFrameInd = (uint32_t) (ptr8[0] | (ptr8[1] << 8) | (ptr8[2] << 16) | (ptr8[3] << 24));
      
      //  Индикация ошибки пропуска начальных фреймов
      if (inpFrameInd != FrameIndexRD)
        BRD_LED_Set(LED_RX_IND_ERR, 1);

      //  Индикация ошибки MISSED_F
      if ((ETH_GetMACITStatus(ETHERNET_X, ETH_MAC_IT_MISSED_F) == SET) || Missed_F_Count)
        BRD_LED_Set(LED_MISSED_F_ERR, 1);      
      
      //  Count Input frames
      FrameIndexRD++;
      LED_CheckSwitch(LED_RX, &LED_IndexRX);
     
      ETHERNET_X->ETH_STAT = 0;
      doFrameSend = 1;
    }  
    
    //  ------------  Send Frame ---------
    if (doFrameSend)
    {
      // Fill Frame
      ptrDataTX = BRD_ETH_Init_FrameTX(MAC_DEST, MAC_SRC, frameLen, &dataCount);

      ptrDataTX[0] =  FrameIndexWR & 0xFF;
      ptrDataTX[1] = (FrameIndexWR >> 8)  & 0xFF;
      ptrDataTX[2] = (FrameIndexWR >> 16) & 0xFF;
      ptrDataTX[3] = (FrameIndexWR >> 24) & 0xFF;
      
      for (i = 4; i < dataCount; ++i)
      {
        ptrDataTX[i] = FRAME_HEAD_SIZE + 4 + i;
      }
      
      //  Check Buff_TX and out
			while (ETH_GetFlagStatus(ETHERNET_X, ETH_MAC_FLAG_X_HALF) == SET);
			ETH_SendFrame(ETHERNET_X, (uint32_t *) FrameTX, frameLen);
     
      // Count output frames
      FrameIndexWR++;
      LED_CheckSwitch(LED_TX, &LED_IndexTX);
        
      doFrameSend = 0;
    }  
  }
}  

void LED_CheckSwitch(uint32_t ledSel, int32_t * ledIndex)
{
  (*ledIndex)--;
  if ((*ledIndex) < 0)
  {  
    (*ledIndex) = LED_FRAME_PERIOD;
    BRD_LED_Switch(ledSel);    
  }  
}  

void ETHERNET_IRQHandler(void)
{
  volatile uint32_t status;
  
#ifdef USE_BOARD_VE_8
  NVIC_ClearPendingIRQ(ETH0_IRQn);
  
 	status = ETHERNET_X->IFR;   
	ETHERNET_X->IFR = status; 
#else
  NVIC_ClearPendingIRQ(ETHERNET_IRQn);  
  
  status = ETHERNET_X->ETH_IFR;
	ETHERNET_X->ETH_IFR = status; 
#endif  
  
  Missed_F_Count++;
}  

