#ifndef __CANH__
#define __CANH__

#ifdef __cplusplus
// extern "C"{
#endif

#include "start.h"
#include "can.h"

extern CAN_HandleTypeDef hcan;

/**
 * @brief CAN 封装类，提供发送/接收及中断处理接口
 */
class CanBox{
public:
    /**
     * @brief 指向 HAL 的 CAN 句柄
     */
    CAN_HandleTypeDef* Can = &hcan;

    /**
     * @brief 默认的标准 ID
     */
    uint32_t stdId;

    /**
     * @brief 接收缓冲（最多 8 字节）
     */
    uint8_t RxData[8];

    /**
     * @brief 发送缓冲（最多 8 字节）
     */
    uint8_t TxData[8];

    /**
     * @brief 作为成员复用的发送头，避免频繁在栈上构造
     */
    CAN_TxHeaderTypeDef txHeader;

    /**
     * @brief 作为成员复用的接收头，用于从 FIFO 读取消息信息
     */
    CAN_RxHeaderTypeDef rxHeader;

    /**
     * @brief HAL 返回的发送邮箱索引
     */
    uint32_t txMailbox = 0;
    
public:
    /**
     * @brief 配置 CAN 过滤器并启动 CAN
     * @param stdId 可选：设置类默认的标准帧 ID（非必要）
     * @return void
     */
    CanBox(uint32_t stdId);
    CanBox(uint32_t stdId, uint32_t txStdId);
    // void CanInit(uint32_t stdId);

    /**
     * @brief 发送默认数据（使用内部 TxData）
     * @param stdId 标准帧 ID
     * @param len 要发送的数据长度（最大 8）
     * @return true 发送成功加入邮箱，false 失败
     */
    bool Send(uint32_t stdId, uint8_t len = 0x08);

    /**
     * @brief 发送指定数据缓冲
     * @param stdId 标准帧 ID
     * @param txdata 指向要发送的数据（长度由 len 指定）
     * @param len 数据长度（最大 8）
     * @return true 发送成功加入邮箱，false 失败
     */
    bool Send(uint32_t stdId, uint8_t *txdata ,uint8_t len = 0x08);

    /**
     * @brief 由 HAL 回调调用读取 FIFO 消息到内部缓冲
     * @note 该函数只负责从 HAL 获取消息并保存到成员变量
     */
    void Receive();
};
/**
 * @brief 全局 CAN 对象实例
 */
extern CanBox canbox;


#ifdef __cplusplus
// }
#endif

#endif