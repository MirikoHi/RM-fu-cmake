/**
 * @file _can.cpp
 * @brief CAN 通信封装（CanBox）实现。
 *
 * 发送：把帧头和数据交给 HAL 的发送邮箱，由 CAN 硬件按总线仲裁规则发出，
 *       一帧最多 8 个数据字节；没有空闲邮箱时立即返回失败，不阻塞等待。
 * 接收：FIFO0 收到消息会进中断，回调里调用 Receive() 把数据搬进 RxData，
 *       主循环随时读 RxData 即可，不需要自己调用 Receive()。
 */
#include "_can.h"
#include <algorithm> // std::copy

/* 构造：只指定一个 ID，收发都用它。
 * txHeader 是发送帧头，这里顺便把固定项填好：扩展 ID 置 0（本工程只用标准帧）、
 * 标识符类型为标准帧（IDE）、帧类型为数据帧而不是远程帧（RTR）。 */
CanBox::CanBox(uint32_t stdId){
    this->txHeader.StdId = stdId;
    this->stdId = stdId;
    this->txHeader.ExtId = 0;
    this->txHeader.IDE = CAN_ID_STD;
    this->txHeader.RTR = CAN_RTR_DATA;

}

/* 构造：接收用 stdId，发送用 txStdId，收发 ID 可以不同 */
CanBox::CanBox(uint32_t stdId, uint32_t txStdId){
    this->txHeader.StdId = txStdId;
    this->stdId = stdId;
    this->txHeader.ExtId = 0;
    this->txHeader.IDE = CAN_ID_STD;
    this->txHeader.RTR = CAN_RTR_DATA;
}

/* 初始化：配置接收过滤器（放行所有 ID）-> 打开 FIFO0 收消息中断 -> 启动 CAN 外设 */
void CanBox::CanInit()
{
    /* 过滤器配置结构体，下面逐项填写 */
    CAN_FilterTypeDef sFilterConfig;

    // 接受全部 ID 的过滤器（掩码/ID 全 0）
    /* 期望 ID 的高 16 位与低 16 位都填 0 */
    sFilterConfig.FilterIdHigh = 0;
    sFilterConfig.FilterIdLow = 0;
    /* 掩码同样全 0：掩码位为 0 表示“这一位不关心”，因此任何 ID 都能通过 */
    sFilterConfig.FilterMaskIdHigh = 0;
    sFilterConfig.FilterMaskIdLow = 0;
    /* 通过过滤器的报文放进 FIFO0 */
    sFilterConfig.FilterFIFOAssignment = CAN_FilterFIFO0;
    /* 使用 0 号过滤器组（STM32F1 的 CAN1 有 0~13 组） */
    sFilterConfig.FilterBank = 0;
    /* 掩码模式：ID 配合掩码使用，而不是逐条 ID 精确匹配 */
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    /* 一个过滤器组按 32 位使用（也可选 16 位拆成两个） */
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    /* 使能这组过滤器 */
    sFilterConfig.FilterActivation = ENABLE;
    /* 双 CAN 时用来指定从机的起始过滤器组；本工程只有一路 CAN，填 0 即可 */
    sFilterConfig.SlaveStartFilterBank = 0;
    
    /* 把上面的配置写入 CAN 硬件 */
    HAL_CAN_ConfigFilter(this->Can, &sFilterConfig);

    // 激活接收中断并启动 CAN
    /* 打开“FIFO0 收到新消息”中断，返回 HAL_OK 才继续启动 CAN */
    if(HAL_CAN_ActivateNotification(this->Can, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK){
        /* 启动 CAN，进入正常收发状态 */
        HAL_CAN_Start(this->Can);
    }
    else{
        /* 打开中断失败：此处不做处理（空语句）；CAN 没启动，后续发送会失败 */
        void();
    }
}

/* 发送：用成员 TxData 里的数据，帧 ID 用传入的 stdId */
bool CanBox::Send(uint32_t stdId, uint8_t len){
    /* 长度检查：CAN 一帧最多 8 字节 */
    if(len > 8){
        return false;
    }

    /* 本次发送的数据长度（DLC）和帧 ID */
    this->txHeader.DLC = len;
    this->txHeader.StdId = stdId;

#if defined(HAL_CAN_MODULE_PRIV_DEF) || defined(CAN_TXMAILBOX_0)
    /* 部分版本 HAL 要求显式关闭“全局时间戳”功能 */
    this->txHeader.TransmitGlobalTime = DISABLE;
#endif

    /* 把帧放进空闲发送邮箱，成功即返回 true */
    if(HAL_CAN_AddTxMessage(this->Can, &this->txHeader, (uint8_t*)this->TxData, &this->txMailbox) == HAL_OK){
        return true;
    }
    /* 邮箱全满或出错 */
    return false;
}

/* 发送：数据由调用方传入（txdata），帧 ID 用 stdId */
bool CanBox::Send(uint32_t stdId, uint8_t *txdata ,uint8_t len){
    /* 长度检查：CAN 一帧最多 8 字节 */
    if(len > 8){
        return false;
    }

    /* 本次发送的数据长度（DLC）和帧 ID */
    this->txHeader.DLC = len;
    this->txHeader.StdId = stdId;

#if defined(HAL_CAN_MODULE_PRIV_DEF) || defined(CAN_TXMAILBOX_0)
    /* 部分版本 HAL 要求显式关闭“全局时间戳”功能 */
    this->txHeader.TransmitGlobalTime = DISABLE;
#endif

    /* 把帧放进空闲发送邮箱，成功即返回 true */
    if(HAL_CAN_AddTxMessage(this->Can, &this->txHeader, (uint8_t*)txdata, &this->txMailbox) == HAL_OK){
        return true;
    }
    /* 邮箱全满或出错 */
    return false;
}

/* 发送：数据由调用方传入，帧 ID 沿用帧头里已有的值（构造函数设的 txStdId） */
bool CanBox::Send(uint8_t *txdata, uint8_t len){
    /* 长度检查：CAN 一帧最多 8 字节 */
    if(len > 8){
        return false;
    }

    /* 只更新数据长度，帧 ID 保持不动 */
    this->txHeader.DLC = len;

#if defined(HAL_CAN_MODULE_PRIV_DEF) || defined(CAN_TXMAILBOX_0)
    /* 部分版本 HAL 要求显式关闭“全局时间戳”功能 */
    this->txHeader.TransmitGlobalTime = DISABLE;
#endif

    /* 把帧放进空闲发送邮箱，成功即返回 true */
    if(HAL_CAN_AddTxMessage(this->Can, &this->txHeader, (uint8_t*)txdata, &this->txMailbox) == HAL_OK){
        return true;
    }
    /* 邮箱全满或出错 */
    return false;
}

/* 发送：用成员 TxData，帧 ID 沿用帧头里已有的值；主循环里的反馈帧走这个重载 */
bool CanBox::Send(uint8_t len){
    /* 长度检查：CAN 一帧最多 8 字节 */
    if(len > 8){
        return false;
    }

    /* 只更新数据长度，帧 ID 保持不动 */
    this->txHeader.DLC = len;

#if defined(HAL_CAN_MODULE_PRIV_DEF) || defined(CAN_TXMAILBOX_0)
    /* 部分版本 HAL 要求显式关闭“全局时间戳”功能 */
    this->txHeader.TransmitGlobalTime = DISABLE;
#endif

    /* 把帧放进发送邮箱，成功即返回 true */
    if(HAL_CAN_AddTxMessage(this->Can, &this->txHeader, (uint8_t*)this->TxData, &this->txMailbox) == HAL_OK){
        return true;
    }
    /* 邮箱全满或出错 */
    return false;
}

/* 由中断回调调用：从 FIFO0 取一帧，只保存 ID 与本机相符的数据 */
void CanBox::Receive(){
    /* HAL 会把 8 个数据字节写进这个临时缓冲区 */
    uint8_t rxbuf[8];
    if(HAL_CAN_GetRxMessage(this->Can, CAN_RX_FIFO0, &this->rxHeader, rxbuf) == HAL_OK){
        /* 只收本机 ID 的帧（此处固定拷贝 8 字节，未按 DLC 截断），其余丢弃 */
        if(this->rxHeader.StdId == this->stdId){
            std::copy(rxbuf,rxbuf + 8, this->RxData);
        }
    }
}


/* HAL 中断回调：FIFO0 收到新消息时被调用（保持 C 链接，函数名不能改） */
extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan){
    /* 只处理本机 CAN 外设（本工程只有一路） */
    if(hcan == canbox.Can){
        canbox.Receive();
    }
}
