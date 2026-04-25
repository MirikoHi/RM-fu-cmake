#include "Fu.h"
#include "tim.h"
// #include <algorithm>

extern DMA_HandleTypeDef hdma_tim1_ch1;
extern DMA_HandleTypeDef hdma_tim1_ch2;
extern DMA_HandleTypeDef hdma_tim2_ch1;
extern DMA_HandleTypeDef hdma_tim2_ch2_ch4;
extern DMA_HandleTypeDef hdma_tim3_ch1_trig;
extern DMA_HandleTypeDef hdma_tim4_ch1;

int16_t WS::Buf_frame_up[NumUp] = {0};
int16_t WS::Buf_frame_down[NumDown] = {0};
int16_t WS::Buf_Circle[NumCircle] = {0};

uint32_t WS::WS281x_Color(uint32_t red, uint32_t green, uint32_t blue){
    return ((uint32_t)green) << 16 | ((uint32_t)red) << 8 | ((uint32_t)blue);
}

void WS::WS281x_SetPixelRGB(uint8_t pos, uint16_t n, uint8_t red, uint8_t green, uint8_t blue){
    switch(pos){
        case circle:{
            for (uint8_t i = 0; i < 24; ++i)
            {
                Buf_Circle[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
            }
            break;
		}
        case up:{
            for (uint8_t i = 0; i < 24; ++i)
            {
                Buf_frame_up[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
            }
            break;
		}
        case down:{
            for (uint8_t i = 0; i < 24; ++i)
            {
                Buf_frame_down[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
            }
            break;
		}
        default:{
            break;
		}
    }
}

void WS::WS_Load_Circle(){
	// __HAL_DMA_DISABLE_IT(&hdma_tim2_ch1, DMA_IT_HT);
    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *)WS::Buf_Circle, NumCircle);
}

void WS::WS_Load_Frame_up(){
	// __HAL_DMA_DISABLE_IT(&hdma_tim3_ch1_trig, DMA_IT_HT);
    HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t *)WS::Buf_frame_up, NumUp);
}

void WS::WS_Load_Frame_down(){
	// __HAL_DMA_DISABLE_IT(&hdma_tim2_ch2_ch4, DMA_IT_HT);
    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_2, (uint32_t *)WS::Buf_frame_down, NumDown);
}

/* DMA接收中断回调函数 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) { }

/* DMA接收过半中断回调函数 */
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) { }

void Fu_t::lightenCross(uint8_t red, uint8_t green, uint8_t blue){
    // 右十字
	WS::WS281x_SetPixelRGB(circle, 0,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 48,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 90,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 134,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 174,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 210,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 230,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 250,red, green, blue);

	// 上十字
	WS::WS281x_SetPixelRGB(circle, 12,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 59,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 101,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 144,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 183,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 215,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 235,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 253,red, green, blue);

	// 左十字
	WS::WS281x_SetPixelRGB(circle, 24,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 70,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 113,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 155,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 192,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 220,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 240,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 256,red, green, blue);

	// 下十字
	WS::WS281x_SetPixelRGB(circle, 36,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 80,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 124,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 165,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 201,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 225,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 245,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 259,red, green, blue);

	lightenRing(2, red, green, blue);
	lightenRing(6, red, green, blue);
	lightenRing(9, red, green, blue);

    WS::WS_Load_Circle();
}

void Fu_t::lightenFrame(uint16_t red, uint8_t green, uint8_t blue){
    for(uint16_t n =  0 ; n < 88; n++){
		WS::WS281x_SetPixelRGB(up, n, red, green, blue);
		// HAL_Delay(2);
	}
    WS::WS_Load_Frame_up();

    for(uint16_t n =  0 ; n < 92; n++){
		WS::WS281x_SetPixelRGB(down, n, red, green, blue);
		// HAL_Delay(2);
	}
    WS::WS_Load_Frame_down();

}

void Fu_t::lightenRing(uint8_t ringNum, uint8_t red, uint8_t green, uint8_t blue){
	switch(ringNum){
		// 九环
		case 9:
			for(uint16_t n = 262; n <= 270; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		// 八环
		case 8:
			for(uint16_t n = 251; n < 262; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		// 七环
		case 7:
			for(uint16_t n = 231; n < 251; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		// 六环
		case 6:
			for(uint16_t n = 211; n < 231; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		// 五环
		case 5:
			for(uint16_t n = 175; n < 211; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		// 四环
		case 4:
			for(uint16_t n = 135; n < 175; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		// 三环
		case 3:
			for(uint16_t n = 91; n < 135; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		// 二环
		case 2:
			for(uint16_t n = 49; n < 91; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		// 一环
		case 1:
			for(uint16_t n = 0; n < 49; n++){
				WS::WS281x_SetPixelRGB(circle, n, red, green, blue);
			}
			break;

		default:
			break;
	}

	// WS::WS_Load_Circle();
}

uint8_t Fu_t::gotHitRing(){
    rings.one_ring = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13);
    rings.two_ring = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_14);
    rings.three_ring = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);
    rings.four_ring = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1);
    rings.five_ring = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2);
    rings.six_ring = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3);
    rings.seven_ring = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
    rings.eight_ring = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
    rings.nine_ring = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_6);

    if(rings.nine_ring == 0){
        hitState =  1;
        return 9;
    }
    else if(rings.eight_ring == 0){
        hitState =  1;
        return 8;
    }
    else if(rings.seven_ring == 0){
        hitState =  1;
        return 7;
    }
    else if(rings.six_ring == 0){
        hitState =  1;
        return 6;
    }
    else if(rings.five_ring == 0){
        hitState =  1;
        return 5;
    }
    else if(rings.four_ring == 0){
        hitState =  1;
        return 4;
    }
    else if(rings.three_ring == 0){
        hitState =  1;
        return 3;
    }
    else if(rings.two_ring == 0){
        hitState =  1;
        return 2;
    }
    else if(rings.one_ring == 0){
        hitState =  1;
        return 1;
    }
    else{
        hitState =  0;
    }

    return 0;
}

void Fu_t::closeCircle(){
	for (uint16_t i = 0; i < 275 * 24; i++)
		WS::Buf_Circle[i] = WS0; // 写入逻辑0的占空比
	for (uint16_t i = 275 * 24; i < NumCircle; i++)
		WS::Buf_Circle[i] = 0; // 占空比为0，全为低电平
	
	WS::WS_Load_Circle();
}

void Fu_t::closeFrame(){
	for (uint16_t i = 0; i < 88 * 24; i++)
		WS::Buf_frame_up[i] = WS0; // 写入逻辑0的占空比
	for (uint16_t i = 88 * 24; i < NumUp; i++)
		WS::Buf_frame_up[i] = 0; // 占空比为0，全为低电平
	
	WS::WS_Load_Frame_up();

	for (uint16_t i = 0; i < 92 * 24; i++)
		WS::Buf_frame_down[i] = WS0; // 写入逻辑0的占空比
	for (uint16_t i = 92 * 24; i < NumDown; i++)
		WS::Buf_frame_down[i] = 0; // 占空比为0，全为低电平
	
	WS::WS_Load_Frame_down();
}