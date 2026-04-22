#ifndef __CANH__
#define __CANH__

#ifdef __cplusplus
// extern "C"{
#endif

#include "start.h"
#include "can.h"

extern CAN_HandleTypeDef hcan;

class CanBox{
public:
    CAN_HandleTypeDef* Can = &hcan;
    uint32_t stdId;
    uint8_t RxData[8];
    uint8_t TxData[8];

    CAN_TxHeaderTypeDef txHeader;
    CAN_RxHeaderTypeDef rxHeader;
    uint32_t txMailbox = 0;
    
public:
    void CanInit(uint32_t stdId);
    // 发送标准帧数据（阻塞式，调用 HAL_CAN_AddTxMessage）
    bool Send(uint32_t stdId, uint8_t len = 0x08);
    bool Send(uint32_t stdId, uint8_t *txdata ,uint8_t len = 0x08);

    // 内部由中断/回调调用，放为 public 以便 extern "C" 回调访问
    void Receive();
};
extern CanBox canbox;


#ifdef __cplusplus
// }
#endif

#endif