/**
  ******************************************************************************
  * @file           : start.cpp
  * @brief          : Main program body
  ******************************************************************************
  * @author Miriko H    HFUT
  * @link https://github.com/MirikoHi/2026-Energy-Mechine-cmake/
  * 
  ******************************************************************************
*/
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

#define BOARD_READY 1

CanBox canbox(CanID, TxCanID);

#include "Fu.h"

Fu_t Fu;

#include "dma.h"
#include "tim.h"
#include "gpio.h"

#define BLUE 1
#define RED 0

// #define __DE_BUG

int16_t resetcnt = 0;

/* 最后执行（应用）的主控命令序号，随 20ms 反馈帧回显给主板 */
uint8_t lastSeq = 0;

/* 上一次真正渲染到 WS2812 显示的颜色（0xFF 表示当前无显示）；
   用于“重复点亮改色”：已亮时收到不同颜色的点亮命令就按新颜色重绘 */
static uint8_t shownColor = 0xFF;

void startup(void){
    canbox.CanInit();

    Fu.hitState = false;
    Fu.stayState = false;
    Fu.lightState = false;

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

        // TxData处理（反馈帧：字节0=在线，字节1=命中，字节2=实际亮起，字节3=命中环显示，字节4=最后执行命令序号）
        canbox.TxData[0] = BOARD_READY;
        canbox.TxData[1] = Fu.hitState;
        canbox.TxData[2] = Fu.lightState;
        canbox.TxData[3] = Fu.stayState;
        canbox.TxData[4] = lastSeq;
        canbox.Send();

        if(!Fu.hitState){
            Fu.hitRing = Fu.gotHitRing();
        }

        // RxData处理
#ifndef __DE_BUG
        while(!canbox.RxData[0]){
            canbox.Send();
            HAL_Delay(50);
        }; // 上位机标志位 1 = ready
        Fu.hitEnable = canbox.RxData[1];
        Fu.color = canbox.RxData[2] == BLUE ? blue : red;
        Fu.twinkleState = canbox.RxData[3];
        uint8_t rxSeq = canbox.RxData[4];  // 本圈解析到的主控命令序号
#endif /* __DE_BUG */

        // if-else状态机
        if(Fu.twinkleState == false){
            uint8_t curColor = (Fu.color == blue) ? BLUE : RED; // 本圈命令要求的颜色

            if(Fu.hitEnable == true && Fu.hitState == false){
                if(Fu.lightState == false){
                    Fu.color == blue ? Fu.lightenCross(0, 0, 150) : Fu.lightenCross(150, 0, 0);
                    HAL_Delay(10);
                    Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                    HAL_Delay(10);
                    Fu.lightState = true;
                    shownColor = curColor;
                }
                else if(shownColor != curColor){
                    /* 已点亮且颜色变化：保持点亮状态按新颜色重绘（重复点亮改色） */
                    Fu.color == blue ? Fu.lightenCross(0, 0, 150) : Fu.lightenCross(150, 0, 0);
                    HAL_Delay(10);
                    Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                    HAL_Delay(10);
                    shownColor = curColor;
                }
            }
            if(Fu.hitEnable == true && Fu.hitState == true){
                if(Fu.stayState == false){
                    Fu.closeCircle();
                    HAL_Delay(10);
                    Fu.closeFrame();
                    Fu.color == blue ? Fu.lightenRing(Fu.hitRing, 0, 0, 150) : Fu.lightenRing(Fu.hitRing, 150, 0, 0);
                    WS::WS_Load_Circle();
                    HAL_Delay(10);
                    Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                    Fu.stayState = true;
                    shownColor = curColor;
                }
                else if(shownColor != curColor){
                    /* 命中环常亮中收到不同颜色的点亮命令：保持显示并换色 */
                    Fu.color == blue ? Fu.lightenRing(Fu.hitRing, 0, 0, 150) : Fu.lightenRing(Fu.hitRing, 150, 0, 0);
                    WS::WS_Load_Circle();
                    HAL_Delay(10);
                    Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                    shownColor = curColor;
                }
            }
            if(Fu.hitEnable == false){
                Fu.hitState = false;
                Fu.stayState = false;
                Fu.lightState = false;
                Fu.closeCircle();
                HAL_Delay(10);
                Fu.closeFrame();
                shownColor = 0xFF; // 无显示，下一次点亮按当前颜色渲染
            }
        }
        else if(Fu.twinkleState == true){
            /* 闪烁庆祝：期间不支持换色（此分支内不解析新命令，颜色保持进入时的值） */
            uint8_t cnt = 0;
            while(cnt++ < 3){
                Fu.closeCircle();
                HAL_Delay(10);
                Fu.closeFrame();
                HAL_Delay(400);
                Fu.color == blue ? Fu.lightenRing(Fu.hitRing, 0, 0, 150) : Fu.lightenRing(Fu.hitRing, 150, 0, 0);
                WS::WS_Load_Circle();
                HAL_Delay(10);
                Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                HAL_Delay(400);
            }
            Fu.closeFrame();
            HAL_Delay(10);
            Fu.closeCircle();
            // 关闭所有中断
            __set_FAULTMASK(1); 
            // 复位
            NVIC_SystemReset();
            HAL_Delay(200);
        }

#ifndef __DE_BUG
        /* 状态机执行完毕后记录序号：表示“这条命令本圈已被应用” */
        lastSeq = rxSeq;
#endif /* __DE_BUG */

        HAL_Delay(20);
    }
}
