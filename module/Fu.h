#ifndef __FU_H__
#define __FU_H__

#include "start.h"


/**
 * @brief HAL 定时器句柄（在 C 区域声明以便 HAL 回调可见）
 */
extern "C" {
    extern TIM_HandleTypeDef htim1;
    extern TIM_HandleTypeDef htim2;
    extern TIM_HandleTypeDef htim3;
    extern TIM_HandleTypeDef htim4;
}

#define WS1 69 // WS逻辑高
#define WS0 27 // WS逻辑低
#define NumCircle 24*270 + 224 // 靶环bit数
#define NumUp 24*88 + 2244 // 上边框bit数
#define NumDown 24*92 + 224 // 下边框bit数

enum Color{
    red,
    blue
};

/**
 * @brief 大符区域枚举
 */
enum {
    circle,
    up,
    down
};

/**
 * @brief 各环按键状态容器
 */
struct Rings{
    uint8_t one_ring;
	uint8_t two_ring;
	uint8_t three_ring;
	uint8_t four_ring;
	uint8_t five_ring;
	uint8_t six_ring;
	uint8_t seven_ring;
	uint8_t eight_ring;
	uint8_t nine_ring;
};

/**
 * @brief WS2812 行为与 DMA 封装
 *
 * 提供像素写入到静态缓冲并触发对应的 DMA 传输函数。上层
 * 不应直接操作 HAL DMA，而应使用这些封装函数。
 */
class WS{
public:
    static uint8_t Buf_frame_up[NumUp];
    static uint8_t Buf_frame_down[NumDown];
    static uint8_t Buf_Circle[NumCircle];
public:
    /**
     * @brief 将 RGB 转换为 GRB 32-bit 值（WS2812 协议）
     * @param red 红分量
     * @param green 绿分量
     * @param blue 蓝分量
     * @return 32-bit GRB 数据
     */
    static uint32_t WS281x_Color(uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief 写入指定位置的像素数据到静态缓冲
     * @param pos 目标区域（circle/up/down）
     * @param n 索引
     */
    static void WS281x_SetPixelRGB(uint8_t pos, uint16_t n, uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief 启动/刷新靶环的 DMA 传输
     */
    static void WS_Load_Circle();

    /**
     * @brief 启动/刷新上边框的 DMA 传输
     */
    static void WS_Load_Frame_up();

    /**
     * @brief 启动/刷新下边框的 DMA 传输
     */
    static void WS_Load_Frame_down();
};

/**
 * @brief 大符控制类：颜色、激活、击中与显示控制
 *
 * 负责在上层逻辑中封装靶环/边框的显示操作，并提供简单的
 * 命中状态查询接口。通过调用 WS 的静态方法实现显示。
 */
class Fu_t{
public:
    /** @brief 大符颜色 */
    Color color = blue;

    /** @brief 大符激活/未激活状态 */
    bool hitEnable = 0;

    /** @brief 大符击中/未击中状态，外部事件或传感器设置 */
    bool hitState = 0;

    /** @brief 九环按键 */
    Rings rings = {1,1,1,1,1,1,1,1,1};

    /** @brief 存储击打的环数 */
    uint8_t hitRing;
public:
    Fu_t() = default;

    Fu_t(Color color){
        this->color = color;
    }

    /**
     * @brief 点亮十字靶环，将对应像素写入并启动 DMA
     */
    void lightenCross(uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief 点亮指定靶环
     * @param ringNum 环号，1 表示一环，9 表示九环
     */
    void lightenRing(uint8_t ringNum, uint8_t red, uint8_t green, uint8_t blue);

    /**
     * @brief 点亮靶框
     */
    void lightenFrame(uint16_t red, uint8_t green, uint8_t blue);

    /**
     * @brief 熄灭所有靶环
     */
    void closeCircle();

    /**
     * @brief 熄灭所有靶框
     */
    void closeFrame();

    /**
     * @brief 检测并返回命中环号
     * @return 被击中的环号（1..9），无命中时返回 0
     */
    uint8_t gotHitRing();
};

/** 全局 Fu 实例 */
extern Fu_t Fu;

#endif