#include "start.h"

#include "_can.h"
/**
 * @brief 板的编号，同时决定CAN_ID
 */
#define BoardNum 0
#if BoardNum == 0
    #define CanID 0x101
    #define TxCanID 0x301
#endif
#if BoardNum == 1
    #define CanID 0x102
    #define TxCanID 0x302
#endif
#if BoardNum == 2
    #define CanID 0x103
    #define TxCanID 0x303
#endif
#if BoardNum == 3
    #define CanID 0x104
    #define TxCanID 0x304
#endif
#if BoardNum == 4
    #define CanID 0x105
    #define TxCanID 0x305
#endif

CanBox canbox(CanID, TxCanID);

#include "Fu.h"

Fu_t Fu;

#include "dma.h"
#include "tim.h"
#include "gpio.h"


void startup(void){
    while(1){

    }
}