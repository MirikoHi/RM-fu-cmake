#include "_can.h"
#include <algorithm>

CanBox::CanBox(uint32_t stdId){
    CAN_FilterTypeDef sFilterConfig;

    // 接受全部 ID 的过滤器（掩码/ID 全 0）
    sFilterConfig.FilterIdHigh = 0;
    sFilterConfig.FilterIdLow = 0;
    sFilterConfig.FilterMaskIdHigh = 0;
    sFilterConfig.FilterMaskIdLow = 0;
    sFilterConfig.FilterFIFOAssignment = CAN_FilterFIFO0;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 0;
    
    this->txHeader.StdId = stdId;
    this->stdId = stdId;
    this->txHeader.ExtId = 0;
    this->txHeader.IDE = CAN_ID_STD;
    this->txHeader.RTR = CAN_RTR_DATA;

    HAL_CAN_ConfigFilter(this->Can, &sFilterConfig);

    // 激活接收中断并启动 CAN
    if(HAL_CAN_ActivateNotification(this->Can, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK){
        HAL_CAN_Start(this->Can);
    }
}

CanBox::CanBox(uint32_t stdId, uint32_t txStdId){
    CAN_FilterTypeDef sFilterConfig;

    // 接受全部 ID 的过滤器（掩码/ID 全 0）
    sFilterConfig.FilterIdHigh = 0;
    sFilterConfig.FilterIdLow = 0;
    sFilterConfig.FilterMaskIdHigh = 0;
    sFilterConfig.FilterMaskIdLow = 0;
    sFilterConfig.FilterFIFOAssignment = CAN_FilterFIFO0;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 0;
    
    this->txHeader.StdId = txStdId;
    this->stdId = stdId;
    this->txHeader.ExtId = 0;
    this->txHeader.IDE = CAN_ID_STD;
    this->txHeader.RTR = CAN_RTR_DATA;

    HAL_CAN_ConfigFilter(this->Can, &sFilterConfig);

    // 激活接收中断并启动 CAN
    if(HAL_CAN_ActivateNotification(this->Can, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK){
        HAL_CAN_Start(this->Can);
    }
}

bool CanBox::Send(uint32_t stdId, uint8_t len){
    if(len > 8){
        return false;
    }

    this->txHeader.DLC = len;

#if defined(HAL_CAN_MODULE_PRIV_DEF) || defined(CAN_TXMAILBOX_0)
    this->txHeader.TransmitGlobalTime = DISABLE;
#endif

    if(HAL_CAN_AddTxMessage(this->Can, &this->txHeader, (uint8_t*)this->TxData, &this->txMailbox) == HAL_OK){
        return true;
    }
    return false;
}

bool CanBox::Send(uint32_t stdId, uint8_t *txdata ,uint8_t len){
    if(len > 8){
        return false;
    }

    this->txHeader.DLC = len;

#if defined(HAL_CAN_MODULE_PRIV_DEF) || defined(CAN_TXMAILBOX_0)
    this->txHeader.TransmitGlobalTime = DISABLE;
#endif

    if(HAL_CAN_AddTxMessage(this->Can, &this->txHeader, (uint8_t*)txdata, &this->txMailbox) == HAL_OK){
        return true;
    }
    return false;
}

void CanBox::Receive(){
    uint8_t rxbuf[8];
    if(HAL_CAN_GetRxMessage(this->Can, CAN_RX_FIFO0, &this->rxHeader, rxbuf) == HAL_OK){
        if(this->rxHeader.StdId == this->stdId){
            std::copy(rxbuf,rxbuf + 8, this->RxData);
        }
    }
}


extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){
    if(hcan == canbox.Can){
        canbox.Receive();
    }
}