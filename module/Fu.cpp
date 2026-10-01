/**
 * @file Fu.cpp
 * @brief 大符（能量机关）靶环与靶框 WS2812 灯带驱动。
 *
 * 显示原理：把每颗 WS2812 的 24 bit 颜色数据逐位展开成 24 个 PWM
 * 比较值写入静态缓冲区，再由定时器 DMA 依次搬运输出；缓冲区尾部
 * 预留的 224 项为低电平，用于产生协议要求的复位（RESET）信号。
 * 通道分配：靶环 -> TIM2_CH1，上靶框 -> TIM3_CH1，下靶框 -> TIM2_CH2。
 */
#include "Fu.h"
#include "tim.h"
#include "ArmLayout.h"
// #include <algorithm>

/* 各定时器 DMA 通道句柄，由 CubeMX 在 tim.c / stm32f1xx_hal_msp.c 中定义并初始化 */
extern DMA_HandleTypeDef hdma_tim1_ch1;
extern DMA_HandleTypeDef hdma_tim1_ch2;
extern DMA_HandleTypeDef hdma_tim2_ch1;
extern DMA_HandleTypeDef hdma_tim2_ch2_ch4;
extern DMA_HandleTypeDef hdma_tim3_ch1_trig;
extern DMA_HandleTypeDef hdma_tim4_ch1;

/* WS2812 发送缓冲区：元素是定时器比较值（占空比），不是颜色值。
 * 每颗 LED 占 24 项（G/R/B 各 8 bit，高位先发），末尾 224 项保持低电平作为复位段。 */
int16_t WS::Buf_frame_up[NumUp] = {0};
int16_t WS::Buf_frame_down[NumDown] = {0};
int16_t WS::Buf_Circle[NumCircle] = {0};
int16_t WS::Buf_strip[NumStrip] = {0};
int16_t WS::Buf_arm[NumArm] = {0};

#define ARM_ARROW_W      3                              /* 箭头每行亮起的格子数 */
#define ARM_ARROW_GAP    2                              /* 同一行相邻箭头之间的空格数 */
#define ARM_FLOW_PERIOD  (ARM_ARROW_W + ARM_ARROW_GAP)  /* 5：沿列方向的重复周期 */
#define ARM_FLOW_STEP_MS 100                            /* 每 0.5s 向左走一格 */
#define CROSS_REFRESH_MS 50                             /* 十字每 50ms 重发一帧 */

/* 把 R/G/B 三个分量按 WS2812 的 GRB 顺序打包成一个 24 bit 数值 */
uint32_t WS::WS281x_Color(uint32_t red, uint32_t green, uint32_t blue){
    /* G 占高 8 位、R 占中间 8 位、B 占低 8 位 */
    return ((uint32_t)green) << 16 | ((uint32_t)red) << 8 | ((uint32_t)blue);
}

/* 把第 n 颗 LED 的颜色展开写入指定区域的缓冲区：pos 选区域，n 为灯珠序号 */
void WS::WS281x_SetPixelRGB(uint8_t pos, uint16_t n, uint8_t red, uint8_t green, uint8_t blue){
    /* 按区域选择对应的缓冲区 */
    switch(pos){
        case circle:{
            /* 靶环区域：写入 Buf_Circle，24 bit 数据逐位展开（i = 0 为最高位） */
            for (uint8_t i = 0; i < 24; ++i)
            {
                /* 左移 i 位后取最高位：为 1 写逻辑 1 占空比，为 0 写逻辑 0 占空比 */
                Buf_Circle[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
            }
            break;
		}
        case up:{
            /* 上靶框区域：写入 Buf_frame_up，展开方式与靶环一致 */
            for (uint8_t i = 0; i < 24; ++i)
            {
                Buf_frame_up[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
            }
            break;
		}
        case down:{
            /* 下靶框区域：写入 Buf_frame_down，展开方式与靶环一致 */
            for (uint8_t i = 0; i < 24; ++i)
            {
                Buf_frame_down[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
            }
            break;
		}
		case strip:{
            /* 条纹区域：写入 Buf_strip，展开方式与靶环一致 */
			for (uint8_t i = 0; i < 24; ++i)
            {
                Buf_strip[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
            }
            break;
		}
		case arm:{
            /* 灯臂区域：写入 Buf_arm，展开方式与靶环一致 */
			for (uint8_t i = 0; i < 24; ++i)
            {
                Buf_arm[24 * n + i] = (((WS281x_Color(red, green, blue) << i) & 0X800000) ? WS1 : WS0);
            }
            break;
		}
        default:{
            /* 区域非法：不写任何缓冲区 */
            break;
		}
    }
}

/* 启动/刷新靶环的 PWM-DMA 传输（TIM2_CH1），长度为缓冲区的元素个数 */
void WS::WS_Load_Circle(){
	// __HAL_DMA_DISABLE_IT(&hdma_tim2_ch1, DMA_IT_HT);
    /* 由 HAL 配置 DMA 并把缓冲区内容逐项送入比较寄存器，实现波形输出 */
    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *)WS::Buf_Circle, NumCircle);
}

/* 启动/刷新上靶框的 PWM-DMA 传输（TIM3_CH1） */
void WS::WS_Load_Frame_up(){
	// __HAL_DMA_DISABLE_IT(&hdma_tim3_ch1_trig, DMA_IT_HT);
    /* 发送 Buf_frame_up 中的全部比较值 */
    HAL_TIM_PWM_Start_DMA(&htim3, TIM_CHANNEL_1, (uint32_t *)WS::Buf_frame_up, NumUp);
}

/* 启动/刷新下靶框的 PWM-DMA 传输（TIM2_CH2） */
void WS::WS_Load_Frame_down(){
	// __HAL_DMA_DISABLE_IT(&hdma_tim2_ch2_ch4, DMA_IT_HT);
    /* 发送 Buf_frame_down 中的全部比较值 */
    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_2, (uint32_t *)WS::Buf_frame_down, NumDown);
}

/* 启动/刷新灯带的 PWM-DMA 传输（TIM1_CH1） */
void WS::WS_Load_Strip(){
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)WS::Buf_strip, NumStrip);
}

/* 启动/刷新灯臂的 PWM-DMA 传输（TIM1_CH2） */
void WS::WS_Load_Arm(){
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2, (uint32_t *)WS::Buf_arm, NumArm);
}

/* 说明：下面两个是 PWM-DMA 发送完成/过半回调，本功能不需要额外处理，故函数体留空 */
/* DMA接收中断回调函数 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) { }

/* DMA接收过半中断回调函数 */
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) { }

/* 点亮十字图案：括号中的数字是灯珠在灯带上的序号，四个方向各 8 颗，
 * 并同时点亮十字经过的二环、六环、九环 */
void Fu_t::lightenCross(uint8_t red, uint8_t green, uint8_t blue){
	for(uint16_t i = 0; i < NumCircle; i++){
		WS::Buf_Circle[i] = 0;
	}
	for(uint16_t i = 0; i < 24*275; i++){
		WS::Buf_Circle[i] = WS0;
	}
    // 右十字
	WS::WS281x_SetPixelRGB(circle, 0,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 48,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 90,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 134,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 174,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 210,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 230,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 250,red, green, blue);

	// 上十字
	WS::WS281x_SetPixelRGB(circle, 12,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 59,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 101,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 144,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 183,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 215,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 235,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 253,red, green, blue);

	// 左十字
	WS::WS281x_SetPixelRGB(circle, 24,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 70,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 113,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 155,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 192,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 220,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 240,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 256,red, green, blue);

	// 下十字
	WS::WS281x_SetPixelRGB(circle, 36,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 80,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 124,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 165,red, green, blue);
	WS::WS281x_SetPixelRGB(circle, 201,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 225,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 245,red, green, blue);
	// WS::WS281x_SetPixelRGB(circle, 259,red, green, blue);

	/* 十字图案同时点亮二环、六环、九环 */
	lightenRing(2, red, green, blue);
	lightenRing(6, red, green, blue);
	lightenRing(9, red, green, blue);

	/* 记录颜色并开启常亮刷新，主循环靠它重发同一帧 */
	crossRed = red;
	crossGreen = green;
	crossBlue = blue;
	if (crossOn == false)
	{
		crossOn = true;
		crossLastTick = HAL_GetTick();
	}

	/* 像素全部写完后统一刷新一次 DMA，避免同一帧内多次启动传输 */
    WS::WS_Load_Circle();
}

/* 点亮整个靶框：上靶框 88 颗、下靶框 92 颗，写完各自缓冲区后分别启动 DMA */
void Fu_t::lightenFrame(uint16_t red, uint8_t green, uint8_t blue){
    /* 逐颗写入上靶框的 88 颗灯珠 */
    for(uint16_t n =  0 ; n < 88; n++){
		WS::WS281x_SetPixelRGB(up, n, red, green, blue);
		// HAL_Delay(2);
	}
    /* 上靶框数据就绪，启动 DMA 输出 */
    WS::WS_Load_Frame_up();

    /* 逐颗写入下靶框的 92 颗灯珠 */
    for(uint16_t n =  0 ; n < 92; n++){
		WS::WS281x_SetPixelRGB(down, n, red, green, blue);
		// HAL_Delay(2);
	}
    /* 下靶框数据就绪，启动 DMA 输出 */
    WS::WS_Load_Frame_down();

}

/* 点亮指定环：环号 1~9 对应由内到外的环，把该环的灯珠序号区间写入缓冲区 */
void Fu_t::lightenRing(uint8_t ringNum, uint8_t red, uint8_t green, uint8_t blue){
	/* 按环号选择该环的灯珠序号区间 */
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

	/* 此处不启动 DMA：调用方（如 lightenCross）会统一刷新，避免一帧内重复启动传输 */
	// WS::WS_Load_Circle();
}

void Fu_t::lightenRingSync(uint8_t ringNum, uint8_t red, uint8_t green, uint8_t blue){
	crossOn = false;
	for(uint16_t i = 0; i < NumCircle; i++){
		WS::Buf_Circle[i] = 0;
	}
	for(uint16_t i = 0; i < 24*275; i++){
		WS::Buf_Circle[i] = WS0;
	}
	lightenRing(ringNum, red, green, blue);
	WS::WS_Load_Circle();
}
/* 点亮灯带：把该灯带的灯珠序号区间写入缓冲区（R 标由 lightenRIcon() 单独负责） */
void Fu_t::lightenStrip(uint8_t red, uint8_t green, uint8_t blue){
	#if BoardNum == 0
    for(uint16_t n = 0; n < 48; n++){
		WS::WS281x_SetPixelRGB(strip, n, red, green, blue);
	}
	#else
    for(uint16_t n = 0; n < 80; n++){
		WS::WS281x_SetPixelRGB(strip, n, red, green, blue);
	}
	#endif
	/* 灯带数据就绪，启动 DMA 输出 */
	WS::WS_Load_Strip();
}

/* 只刷新常亮的 R 标（颜色跟随当前命令颜色），灯带其余部分不动 */
void Fu_t::lightenRIcon(void){
	#if BoardNum == 0
	uint8_t Rred = Fu.color == blue ? 0 : 255;
	uint8_t Rblue = Fu.color == blue ? 255 : 0;
	for(uint16_t n = 48; n < 48 + NumRIcon; n++){
		WS::WS281x_SetPixelRGB(strip, n, Rred, 0, Rblue);
	}
	/* R 标数据就绪，启动 DMA 输出 */
	WS::WS_Load_Strip();
	#endif
}

/* 点亮特定灯带：把该灯带的灯珠序号区间写入缓冲区，灯带编号从零开始 */
void Fu_t::lighten_Special_Strip(uint8_t start_num, uint8_t end_num, uint8_t red, uint8_t green, uint8_t blue){
	for(uint16_t n = 0; n <= end_num; n++){
		if(n < start_num){
			WS::WS281x_SetPixelRGB(strip, n, 0, 0, 0);
		}else{
			WS::WS281x_SetPixelRGB(strip, n, red, green, blue);
		}
	}
	/* 灯带数据就绪，启动 DMA 输出 */
	WS::WS_Load_Strip();
}

/* 每行亮起的起始列，和箭头的 24 个坐标一一对应 */
static const uint8_t ARM_ARROW_START[ARM_ROWS] = {0, 1, 2, 3, 3, 2, 1, 0};

/* 灯臂整条写逻辑 0：只写缓冲，不启动 DMA */
static void Arm_ClearBuffer(){
	for (uint16_t i = 0; i < NumArm; i++)
		WS::Buf_arm[i] = 0;     /* 复位段占空比为 0，全为低电平 */
	for (uint16_t i = 0; i < ARM_LED_NUM * 24; i++)
		WS::Buf_arm[i] = WS0;   /* 数据位全部写逻辑 0 的占空比 */
}

/* 画一帧箭头流水：phase 为参考箭头尖端所在的列（0 ~ ARM_FLOW_PERIOD-1） */
static void Arm_DrawArrowFrame(uint8_t phase, uint8_t red, uint8_t green, uint8_t blue){
	int16_t pos, x;
	uint8_t y, d;

	Arm_ClearBuffer();

	/* 相位只取 0~4，向左向右各铺若干份周期副本，覆盖整块灯臂 */
	for (pos = (int16_t)phase - ARM_FLOW_PERIOD; pos < ARM_COLS; pos += ARM_FLOW_PERIOD)
	{
		for (y = 0; y < ARM_ROWS; y++)
		{
			for (d = 0; d < ARM_ARROW_W; d++)
			{
				x = pos + ARM_ARROW_START[y] + d;
				if (x < 0 || x >= ARM_COLS) continue;  /* 左边超出范围的部分不要，右边板外也一样 */
				WS::WS281x_SetPixelRGB(arm, XY_To_Index((uint8_t)x, y), red, green, blue);
			}
		}
	}

	WS::WS_Load_Arm();
}

/* 灯臂：开始箭头流水（灯板准备被击打），已在跑时只更新颜色 */
void Fu_t::startArmFlow(uint8_t red, uint8_t green, uint8_t blue){
	armRed = red;
	armGreen = green;
	armBlue = blue;

	if (armFlowOn == false)
	{
		armFlowOn = true;
		armPhase = 0;
		armLastTick = HAL_GetTick();
	}

	Arm_DrawArrowFrame(armPhase, armRed, armGreen, armBlue);
}

/* 灯臂：256 颗整块点亮（击打完成） */
void Fu_t::lightenArm(uint8_t red, uint8_t green, uint8_t blue){
	armFlowOn = false;  /* 整块亮时不再推进流水 */

	for (uint16_t n = 0; n < ARM_LED_NUM; n++)
		WS::WS281x_SetPixelRGB(arm, n, red, green, blue);

	WS::WS_Load_Arm();
}

/* 灯臂：整块熄灭（主控未允许击打） */
void Fu_t::closeArm(void){
	armFlowOn = false;
	Arm_ClearBuffer();
	WS::WS_Load_Arm();
}

/* 灯臂：主循环每拍调用一次，满 500ms 向左推进一格并重画 */
void Fu_t::armFlowTick(void){
	uint32_t now;

	if (armFlowOn == false) return;

	now = HAL_GetTick();
	if ((uint32_t)(now - armLastTick) < ARM_FLOW_STEP_MS) return;

	armLastTick += ARM_FLOW_STEP_MS;
	//armPhase = (uint8_t)((armPhase + ARM_FLOW_PERIOD - 1) % ARM_FLOW_PERIOD);
	armPhase++;
	if(armPhase >= ARM_FLOW_PERIOD) armPhase = 0;
	Arm_DrawArrowFrame(armPhase, armRed, armGreen, armBlue);
}

/* 十字：主循环每拍调用一次，满 50ms 重发一次当前画面 */
void Fu_t::crossRefreshTick(void){
	if (crossOn == false) return;

	if ((uint32_t)(HAL_GetTick() - crossLastTick) < CROSS_REFRESH_MS) return;

	crossLastTick += CROSS_REFRESH_MS;
	lightenCross(crossRed, crossGreen, crossBlue);
}

/* 灯臂：一次点亮第 0 列至第 x 列（列号从 0 开始，含两端），其余列保持熄灭 */
void Fu_t::lighten_special_arm(uint8_t x, uint8_t red, uint8_t green, uint8_t blue){
	uint8_t col, y;
	if (x >= ARM_COLS) x = ARM_COLS - 1;
	Arm_ClearBuffer();
	for (col = 0; col <= x; col++)
		for (y = 0; y < ARM_ROWS; y++)
			WS::WS281x_SetPixelRGB(arm, XY_To_Index(col, y), red, green, blue);

	WS::WS_Load_Arm();
}

/* 灯臂用的暗一档颜色：减 50 并夹到 0，避免 uint8_t 回绕 */
static uint8_t Arm_DimColor(uint8_t c){
	return (c > 50) ? (uint8_t)(c - 50) : 0;
}

void Fu_t::lighten_step(uint8_t start_num, uint8_t end_num, uint8_t step, uint8_t red, uint8_t green, uint8_t blue){
	// HAL_Delay(15);
	lighten_Special_Strip(start_num, end_num, red, green, blue);
	HAL_Delay(15);
	lighten_special_arm(step, Arm_DimColor(red), Arm_DimColor(green), Arm_DimColor(blue));
	HAL_Delay(15);
}

void Fu_t::lighten_steps(uint8_t step, uint8_t red, uint8_t green, uint8_t blue){
    /* 先清零灯带和灯臂：进度可加可减，step=0 即全灭 */
    //closeStrip();
    //closeArm();
	//armFlowOn = false;

    /* 按步骤点亮灯带 */
    switch(step)
	{
	case 0:
		/* 熄灭all：上面已清零 */
		break;
	case 1:
		/* 点亮1/5, 到灯带上1/4处 */
		#if BoardNum == 0
		lighten_step(15, 31, 5, red, green, blue);
		#endif
		#if BoardNum == 1
		lighten_step(15, 31, 5, red, green, blue);
		#endif
		#if BoardNum == 2
		lighten_step(16, 32, 6, red, green, blue);
		#endif
		#if BoardNum == 3
		lighten_step(16, 33, 5, red, green, blue);
		#endif
		#if BoardNum == 4
		lighten_step(14, 31, 7, red, green, blue);
		#endif
		break;
	case 2:
		/* 点亮2/5, 到灯带上2/4处 */
		#if BoardNum == 0
		lighten_step(10, 36, 14, red, green, blue);
		#endif
		#if BoardNum == 1
		lighten_step(10, 36, 15, red, green, blue);
		#endif
		#if BoardNum == 2
		lighten_step(12, 39, 15, red, green, blue);
		#endif
		#if BoardNum == 3
		lighten_step(10, 39, 15, red, green, blue);
		#endif
		#if BoardNum == 4
		lighten_step(10, 36, 15, red, green, blue);
		#endif
		break;
	case 3:
		/* 点亮3/5, 到灯带上3/4处 */
		#if BoardNum == 0
		lighten_step(5, 41, 25, red, green, blue);
		#endif
		#if BoardNum == 1
		lighten_step(5, 41, 26, red, green, blue);
		#endif
		#if BoardNum == 2
		lighten_step(7, 42, 26, red, green, blue);
		#endif
		#if BoardNum == 3
		lighten_step(5, 44, 27, red, green, blue);
		#endif
		#if BoardNum == 4
		lighten_step(4, 40, 26, red, green, blue);
		#endif
		break;
	case 4:
		/* 点亮4/5, 灯带全部 */
		#if BoardNum == 0
		lighten_step(0, 47, 31, red, green, blue);
		#endif
		#if BoardNum == 1
		lighten_step(0, 49, 31, red, green, blue);
		#endif
		#if BoardNum == 2
		lighten_step(0, 46, 39, red, green, blue);
		#endif
		#if BoardNum == 3
		lighten_step(0, 49, 31, red, green, blue);
		#endif
		#if BoardNum == 4
		lighten_step(0, 46, 31, red, green, blue);
		#endif
		break;
	}
}

/* 读取九个环的击中检测引脚（低电平表示被击中），按由外到内的顺序返回环号并更新 hitState */
uint8_t Fu_t::gotHitRing(){
    /* 一次性采集九个环对应的 GPIO 电平（0 表示该环被击中） */
    rings.one_ring = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13);
    rings.two_ring = HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_14);
    rings.three_ring = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0);
    rings.four_ring = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1);
    rings.five_ring = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2);
    rings.six_ring = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3);
    rings.seven_ring = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);
    rings.eight_ring = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
    rings.nine_ring = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_6);

    /* 命中判定：从九环（最外）到一环（最内）依次判断，最先命中的环优先返回 */
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
        /* 九个环都没有检测到击中信号 */
        hitState =  0;
    }

    /* 返回 0 表示本次没有环被击中 */
    return 0;
}

/* 熄灭靶环：所有灯珠数据位写逻辑 0，其余复位段保持低电平，最后刷新 DMA */
void Fu_t::closeCircle(){
	crossOn = false;
	/* 其余 224 项为复位段，占空比置 0 以保持低电平 */
	for (uint16_t i = 0; i < NumCircle; i++)
		WS::Buf_Circle[i] = 0; // 占空比为0，全为低电平
	/* 前 275*24 项为 275 颗灯珠的数据位，全部写逻辑 0 占空比即为熄灭 */
	for (uint16_t i = 0; i < 275 * 24; i++)
		WS::Buf_Circle[i] = WS0; // 写入逻辑0的占空比

	
	/* 刷新 DMA，把熄灭后的缓冲区下发给灯带 */
	WS::WS_Load_Circle();
}

/* 熄灭靶框：上、下靶框的数据位写逻辑 0，补足复位段后分别刷新 DMA */
void Fu_t::closeFrame(){
	/* 上靶框剩余 224 项为复位段 */
	for (uint16_t i = 0; i < NumUp; i++)
		WS::Buf_frame_up[i] = 0; // 占空比为0，全为低电平
	/* 上靶框：88 颗灯珠的数据位全部写逻辑 0 */
	for (uint16_t i = 0; i < 88 * 24; i++)
		WS::Buf_frame_up[i] = WS0; // 写入逻辑0的占空比

	
	/* 上靶框熄灭结果下发 */
	WS::WS_Load_Frame_up();

	/* 下靶框剩余 224 项为复位段 */
	for (uint16_t i = 0; i < NumDown; i++)
		WS::Buf_frame_down[i] = 0; // 占空比为0，全为低电平
	/* 下靶框：92 颗灯珠的数据位全部写逻辑 0 */
	for (uint16_t i = 0; i < 92 * 24; i++)
		WS::Buf_frame_down[i] = WS0; // 写入逻辑0的占空比

	
	/* 下靶框熄灭结果下发 */
	WS::WS_Load_Frame_down();
}

void Fu_t::closeStrip(){
	// /* 灯带：80 颗灯珠的数据位全部写逻辑 0 */
	// for (uint16_t i = 0; i < 80 * 24; i++)
	// 	WS::Buf_strip[i] = WS0; // 写入逻辑0的占空比
	// /* 灯带剩余 224 项为复位段 */
	// for (uint16_t i = 80 * 24; i < NumStrip; i++)
	// 	WS::Buf_strip[i] = 0; // 占空比为0，全为低电平
	
	// /* 灯带熄灭结果下发 */
	// WS::WS_Load_Strip();
	Fu_t::lightenStrip(0, 0, 0);
}
