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

#define BLUE 1
#define RED 0

#define __DE_BUG

int16_t resetcnt = 0;

void startup(void){
    Fu.closeCircle();
    HAL_Delay(10);
    Fu.closeFrame();
    HAL_Delay(100);
    while(1){

#ifdef __DE_BUG
        Fu.hitEnable = true;
        while(Fu.hitState == true){
            if(resetcnt++ > 20){
                Fu.closeFrame();
                HAL_Delay(10);
                Fu.closeCircle();
                resetcnt = 0;
                // 关闭所有中断
                __set_FAULTMASK(1); 
                // 复位
                NVIC_SystemReset();
                HAL_Delay(300);
            }
            HAL_Delay(50);
        }
#endif /* __DE_BUG */

        // TxData处理
        canbox.Send();

        Fu.hitRing = Fu.gotHitRing();

        // RxData处理
        // Fu.hitEnable = canbox.RxData[0];
        // Fu.color = canbox.RxData[1] == BLUE ? blue : red;

        // if-else状态机
        if(Fu.hitEnable == true && Fu.hitState == false){
            // Fu.closeCircle();
            // Fu.closeFrame();
            if(Fu.lightState == false){
                Fu.color == blue ? Fu.lightenCross(0, 0, 150) : Fu.lightenCross(150, 0, 0);
                HAL_Delay(10);
                Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                HAL_Delay(10);
                Fu.lightState = true;
            }
        }
        if(Fu.hitEnable == true && Fu.hitState == true){
            Fu.closeCircle();
            HAL_Delay(10);
            Fu.closeFrame();
            Fu.color == blue ? Fu.lightenRing(Fu.hitRing, 0, 0, 150) : Fu.lightenRing(Fu.hitRing, 150, 0, 0);
            WS::WS_Load_Circle();
            HAL_Delay(10);
            Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
        }
        if(Fu.hitEnable == false){
            Fu.hitState = false;
            Fu.closeCircle();
            HAL_Delay(10);
            Fu.closeFrame();
        }

        HAL_Delay(20);
    }
}