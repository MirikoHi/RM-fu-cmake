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

/* 引入 CanBox 类声明，下面会用它定义全局对象 canbox */
#include "_can.h"
/**
 * @brief 板的编号，同时决定CAN_ID
 */
/* 每块板烧同一份程序，靠板号区分 CAN 收发 ID：
   0~4 号板分别对应 0x101~0x105（接收）和 0x301~0x305（发送）。换板号就是改这里 */
#define BoardNum 2
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

/* 应答帧字节 0 的取值：1 表示本板已就绪（在线） */
#define BOARD_READY 1

/* 全局 CAN 对象：接收 ID 用 CanID，发送 ID 用 TxCanID（都由板号决定） */
CanBox canbox(CanID, TxCanID);

#include "Fu.h"

/* 全局大符对象：靶环与靶框的显示状态都保存在这里 */
Fu_t Fu;

/* 定时器 / DMA / GPIO 的 HAL 声明（由 CubeMX 生成） */
#include "dma.h"
#include "tim.h"
#include "gpio.h"

/* 上位机命令里表示颜色的取值，需与主控约定一致 */
#define BLUE 1
#define RED 0

/* 打开后会进入自测模式：不解析上位机命令，直接走命中与复位流程（见下方 __DE_BUG 分支） */
// #define __DE_BUG

/* 自测模式下的循环计数，累计超过阈值就熄灯并复位 */
int16_t resetcnt = 0;

/* 最后执行（应用）的主控命令序号，随 20ms 反馈帧回显给主板 */
uint8_t lastSeq = 0;

/* 上一次真正渲染到 WS2812 显示的颜色（0xFF 表示当前无显示）；
   用于“重复点亮改色”：已亮时收到不同颜色的点亮命令就按新颜色重绘 */
static uint8_t shownColor = 0xFF;

/* 主循环入口：初始化 CAN 和显示，之后每 20ms 跑一圈“反馈 -> 解析命令 -> 状态机” */
void startup(void){
    /* CAN 初始化：配过滤器、开接收中断、启动外设 */
    canbox.CanInit();

    /* 清空状态机初值，避免上电后残留上一次的状态 */
    Fu.hitState = false;
    Fu.stayState = false;
    Fu.lightState = false;

    /* 上电先熄灭靶环和靶框，两次之间留出延时等 PWM-DMA 发送完 */
    Fu.closeCircle();
    HAL_Delay(10);
    Fu.closeFrame();
    HAL_Delay(100);
    Fu.closeStrip();
    HAL_Delay(100);
    Fu.closeStrip();
    HAL_Delay(100);

    /* 灯臂：上电先整块熄灭 */
    Fu.closeArm();
    HAL_Delay(100);

    while(1){

        /* 灯臂箭头流水：非阻塞推进，满 500ms 左移一格 */
        Fu.armFlowTick();

#ifdef __DE_BUG
        /* 自测模式：直接打开命中检测，灯保持点亮 */
        Fu.hitEnable = true;
        while(Fu.hitState == true){
            /* 命中状态下每 50ms 计一次数，超过 20 次（约 1 秒）就熄灯并复位 */
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
        /* 每圈发送一次应答帧，主控据此判断本板在线并同步命令序号 */
        canbox.Send();

        /* 当前没有命中信号时才去扫描环，避免覆盖已经锁定的命中结果 */
        if(!Fu.hitState){
            Fu.hitRing = Fu.gotHitRing();
        }

        // RxData处理
#ifndef __DE_BUG
        /* 等主控把“就绪”标志（字节 0）置 1；等待期间每 50ms 重发一次应答帧 */
        while(!canbox.RxData[0]){
            canbox.Send();
            HAL_Delay(50);
        }; // 上位机标志位 1 = ready
        /* 解析本圈命令：字节1 是否允许命中，字节2 颜色，字节3 是否闪烁，字节4 命令序号 */
        Fu.hitEnable = canbox.RxData[1];
        Fu.color = canbox.RxData[2] == BLUE ? blue : red;
        Fu.twinkleState = canbox.RxData[3];
        uint8_t rxSeq = canbox.RxData[4];  // 本圈解析到的主控命令序号
        // uint8_t fu_mode = canbox.RxData[5];  // 大/小能量机关模式
        uint8_t fu_mode = 0;
#endif /* __DE_BUG */

        // if-else状态机
        /* 分支一：正常显示（不需要闪烁庆祝） */
        if(Fu.twinkleState == false){
            uint8_t curColor = (Fu.color == blue) ? BLUE : RED; // 本圈命令要求的颜色

            /* 允许命中但还没被打中：保持十字与靶框点亮 */
            if(Fu.hitEnable == true && Fu.hitState == false){
                if(Fu.lightState == false){
                    /* 第一次点亮：按当前颜色绘制十字和靶框 */
                    Fu.color == blue ? Fu.lightenCross(0, 0, 150) : Fu.lightenCross(150, 0, 0);
                    HAL_Delay(10);
                    /* 灯板准备被击打：灯臂亮箭头流水 */
                    Fu.color == blue ? Fu.startArmFlow(0, 0, 150) : Fu.startArmFlow(150, 0, 0);
                    // Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                    // HAL_Delay(10);
                    /* 记录已点亮，并记住这次渲染用的颜色 */
                    Fu.lightState = true;
                    shownColor = curColor;
                }
                else if(shownColor != curColor){
                    /* 已点亮且颜色变化：保持点亮状态按新颜色重绘（重复点亮改色） */
                    Fu.color == blue ? Fu.lightenCross(0, 0, 150) : Fu.lightenCross(150, 0, 0);
                    HAL_Delay(10);
                    /* 灯板准备被击打：灯臂箭头流水按新颜色重绘 */
                    Fu.color == blue ? Fu.startArmFlow(0, 0, 150) : Fu.startArmFlow(150, 0, 0);
                    // Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                    // HAL_Delay(10);
                    shownColor = curColor;
                }
            }
            /* 已经被打中：熄灭全部，只让命中的那一环常亮，并点亮灯带 */
            if(Fu.hitEnable == true && Fu.hitState == true && fu_mode == 0){
                if(Fu.stayState == false){
                    /* 首次进入命中显示：先全灭，再单独点亮命中的环 */
                    Fu.closeCircle();
                    HAL_Delay(10);
                    Fu.closeFrame();
                    Fu.color == blue ? Fu.lightenRing(Fu.hitRing, 0, 0, 150) : Fu.lightenRing(Fu.hitRing, 150, 0, 0);
                    /* lightenRing 只改缓冲区，这里手动刷新一次 DMA 才会显示出来 */
                    WS::WS_Load_Circle();
                    HAL_Delay(10);
                    // Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                    /* 记录进入常亮状态以及当前颜色 */
                    Fu.closeFrame();
                    HAL_Delay(10);
                    Fu.color == blue ? Fu.lightenStrip(0, 0, 150) : Fu.lightenStrip(150, 0, 0);
                    /* 击打完成：灯臂整块点亮 */
                    Fu.color == blue ? Fu.lightenArm(0, 0, 150) : Fu.lightenArm(150, 0, 0);
                    Fu.stayState = true;
                    shownColor = curColor;
                }
                else if(shownColor != curColor){
                    /* 命中环常亮中收到不同颜色的点亮命令：保持显示并换色 */
                    Fu.color == blue ? Fu.lightenRing(Fu.hitRing, 0, 0, 150) : Fu.lightenRing(Fu.hitRing, 150, 0, 0);
                    /* 同上，换色后要刷新 DMA */
                    WS::WS_Load_Circle();
                    HAL_Delay(10);
                    Fu.color == blue ? Fu.lightenStrip(0, 0, 150) : Fu.lightenStrip(150, 0, 0);
                    /* 击打完成：灯臂整块点亮（换色重绘） */
                    Fu.color == blue ? Fu.lightenArm(0, 0, 150) : Fu.lightenArm(150, 0, 0);
                    HAL_Delay(10);
                    // Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                    shownColor = curColor;
                }
            }
            /* 主控不再允许命中：熄灯并把状态机复位，等下一条点亮命令 */
            if(Fu.hitEnable == false){
                Fu.hitState = false;
                Fu.stayState = false;
                Fu.lightState = false;
                Fu.closeCircle();
                HAL_Delay(10);
                Fu.closeFrame();
                HAL_Delay(10);
                Fu.closeStrip();
                Fu.closeArm();
                shownColor = 0xFF; // 无显示，下一次点亮按当前颜色渲染
            }
        }
        /* 分支二：闪烁庆祝，闪三下后复位单片机（表示本回合结束） */
        else if(Fu.twinkleState == true){
            /* 闪烁庆祝：期间不支持换色（此分支内不解析新命令，颜色保持进入时的值） */
            uint8_t cnt = 0;
            /* 一轮 = 熄灭约 400ms + 点亮约 400ms（各另有 10ms 等待发送），共三轮 */
            while(cnt++ < 3){
                Fu.closeCircle();
                HAL_Delay(10);
                Fu.closeFrame();
                HAL_Delay(10);
                Fu.closeStrip();
                Fu.closeArm();
                HAL_Delay(400);
                if(fu_mode == 0){
                    Fu.color == blue ? Fu.lightenRing(Fu.hitRing, 0, 0, 150) : Fu.lightenRing(Fu.hitRing, 150, 0, 0);
                } else if(fu_mode == 1){
                    Fu.color == blue ? Fu.lightenRing(9, 0, 0, 150) : Fu.lightenRing(9, 150, 0, 0);
                }
                WS::WS_Load_Circle();
                HAL_Delay(10);
                Fu.color == blue ? Fu.lightenFrame(0, 0, 150) : Fu.lightenFrame(150, 0, 0);
                HAL_Delay(10);
                Fu.color == blue ? Fu.lightenStrip(0, 0, 150) : Fu.lightenStrip(150, 0, 0);
                /* 庆祝闪烁：灯臂跟着闪 */
                Fu.color == blue ? Fu.lightenArm(0, 0, 150) : Fu.lightenArm(150, 0, 0);
                HAL_Delay(400);
            }
            /* 三轮闪完：先熄灯，再复位单片机，回到 startup 开头重新开始 */
            Fu.closeFrame();
            HAL_Delay(10);
            Fu.closeCircle();
            HAL_Delay(10);
            Fu.closeStrip();
            Fu.closeArm();
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

        /* 固定 20ms 一拍：既是应答帧周期，也决定状态机的响应速度 */
        HAL_Delay(20);
    }
}
