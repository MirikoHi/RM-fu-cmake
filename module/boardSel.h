#ifndef __BOARD_SEL_H__
#define __BOARD_SEL_H__

/**
 * @brief 板的编号，同时决定CAN_ID
 */
/* 每块板烧同一份程序，靠板号区分 CAN 收发 ID：
   0~4 号板分别对应 0x101~0x105（接收）和 0x301~0x305（发送）。换板号就是改这里 */
#define BoardNum 2

#endif