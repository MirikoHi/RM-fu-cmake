#include "Fu.h"

uint8_t WS::Buf_frame_up[NumUp] = {0};
uint8_t WS::Buf_frame_down[NumDown] = {0};
uint8_t WS::Buf_Circle[NumCircle] = {0};

uint32_t WS::WS281x_Color(uint8_t red, uint8_t green, uint8_t blue){
    return green << 16 | red << 8 | blue;
}

void WS::WS281x_SetPixelRGB(uint16_t n, uint8_t red, uint8_t green, uint8_t blue){
    for (uint8_t i = 0; i < 24; ++i)
    {
        Buf_Circle[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
    }
}

void WS::WS_Load_Circle(void){
    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *)Buf_Circle, NumCircle);
}