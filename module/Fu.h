#ifndef __FU_H__
#define __FU_H__

#include "start.h"

enum Color{
    red,
    blue
};


class Fu_t{
public:
    /**
     * @brief 大符颜色
     */
    Color color;

    /**
     * @brief 大符激活/未激活状态位
     */
    bool hitEnable;

    /**
     * @brief 大符击中/未击中状态位
     */
    bool hitState;

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