#include "_can.h"

void CanBox::CanInit(){
    CAN_FilterTypeDef sFilterConfig;

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

    HAL_CAN_ConfigFilter(this->Can, &sFilterConfig);
    if(HAL_CAN_ActivateNotification(this->Can, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK){
        HAL_CAN_Start(this->Can);
    }
}