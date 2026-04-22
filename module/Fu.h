#ifndef __FU_H__
#define __FU_H__

#include "start.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

#define WS1 69 // WS逻辑高
#define WS0 27 // WS逻辑低
#define NumCircle 24*270 + 224
#define NumUp 24*88 + 2244
#define NumDown 24*92 + 224

enum Color{
    red,
    blue
};

class WS{
public:
    static uint8_t Buf_frame_up[NumUp];
    static uint8_t Buf_frame_down[NumDown];
    static uint8_t Buf_Circle[NumCircle];
public:
    static uint32_t WS281x_Color(uint8_t red, uint8_t green, uint8_t blue);
    static void WS281x_SetPixelRGB(uint16_t n, uint8_t red, uint8_t green, uint8_t blue);
    static void WS_Load_Circle(void);
    static void WS_Load_Frame_up(void);
    static void WS_Load_Frame_down(void);
};

class Fu_t{
public:
    /**
     * @brief 大符颜色
     */
    Color color = blue;

    /**
     * @brief 大符激活/未激活状态位
     */
    bool hitEnable = 0;

    /**
     * @brief 大符击中/未击中状态位
     */
    bool hitState = 0;

public:
    /**
     * @brief 点亮十字靶环
     * @return void
     * @param void
     */
    void lightenCross();

    /**
     * @brief 点亮符环
     * @return void
     * @param void
     */
    void lightenRing(uint8_t ringNum);

    /**
     * @brief 点亮靶框
     * @return void
     * @param void
     */
    void lightenFrame();

    /**
     * @brief 击中靶环
     * @return uint8_t
     * @param void
     */
    uint8_t getHitRing();
};

extern Fu_t Fu;

#endif