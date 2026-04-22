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
    uint8_t Data[8];

public:
    void CanInit();
};


#ifdef __cplusplus
// }
#endif

#endif