/**
 * @file ArmLayout.h
 * @brief 灯臂灯板几何：32 列 × 8 行行优先走线的坐标与灯珠编号互转。
 *
 * 走线：编号 0 从 (0,7) 起步，每一行都从最左列开始向右走，行尾经
 * 回线接到下一行左端。编号铺开如下：
 *
 *   y=7:    0    1    2  ...   31   ->
 *   y=6:   32   33   34  ...   63   ->
 *   y=5:   64   65   66  ...   95   ->
 *   y=4:   96   97   98  ...  127   ->
 *   y=3:  128  129  130  ...  159   ->
 *   y=2:  160  161  162  ...  191   ->
 *   y=1:  192  193  194  ...  223   ->
 *   y=0:  224  225  226  ...  255   ->
 *
 * 每行同向，编号就是行优先展开：编号 = 行序 × ARM_COLS + 列号。
 *
 * 注意：起点行号对应灯板的实际装配方向。灯板若旋转 180° 或镜像
 * 安装，需要把 ARM_START_ROW 一起改掉，否则图案会上下颠倒。上电
 * 自检建议：(0,7) 亮红、(31,7) 亮绿、(0,0) 亮蓝，对实物确认三个
 * 角都对得上再调图案。
 *
 * 滚动图案时偏移量要加在坐标 x 上，不要在编号上做偏移：编号跨过
 * 行尾会直接落到下一行开头，水平光带会在行末断开。
 *
 * 本文件是纯几何换算，不依赖 HAL 与任何全局状态，可单独编译测试。
 */

#ifndef __ARM_LAYOUT_H__
#define __ARM_LAYOUT_H__

#include <stdint.h>

/** @brief 灯板列数（x 方向，0 ~ ARM_COLS-1） */
#define ARM_COLS 32

/** @brief 灯板行数（y 方向，0 ~ ARM_ROWS-1） */
#define ARM_ROWS 8

/** @brief 灯珠总数，与 Fu.h 的 NumArm 中隐含的 256 一致 */
#define ARM_LED_NUM (ARM_COLS * ARM_ROWS)

/** @brief 走线的起点行号，即编号 0 所在的那一行 */
#define ARM_START_ROW (ARM_ROWS - 1)

/**
 * @brief 把灯板坐标换成灯珠编号
 * @param x 列号 0 ~ ARM_COLS-1
 * @param y 行号 0 ~ ARM_ROWS-1
 * @return 灯珠编号 0 ~ ARM_LED_NUM-1
 */
static inline uint16_t XY_To_Index(uint8_t x, uint8_t y)
{
    /* 从起点行往下数了几行 */
    uint8_t r = (uint8_t)(ARM_START_ROW - y);
    /* 每行都同向，行内列号就是编号的低位 */
    return (uint16_t)(r * ARM_COLS + x);
}

/**
 * @brief 把灯珠编号换成灯板坐标
 * @param idx 灯珠编号 0 ~ ARM_LED_NUM-1
 * @param x 输出列号
 * @param y 输出行号
 */
static inline void Index_To_XY(uint16_t idx, uint8_t *x, uint8_t *y)
{
    uint8_t r = (uint8_t)(idx / ARM_COLS);  /* 第几行（从起点行算起） */
    uint8_t c = (uint8_t)(idx % ARM_COLS);  /* 该行内第几颗，也就是列号 */
    *y = (uint8_t)(ARM_START_ROW - r);
    *x = c;
}

#endif
