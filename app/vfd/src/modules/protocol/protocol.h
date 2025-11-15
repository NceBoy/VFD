#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "tx_api.h"

// 消息包结构体
#pragma pack(1)
typedef struct {
    uint8_t header;                  // 消息包头 (0xBB)
    uint8_t type;                    // 消息类型
    uint8_t action;                  // 消息动作
    uint8_t reserved;                // 预留字段
    uint16_t source_id;              // 源设备ID (小端)
    uint16_t target_id;              // 目标设备ID (小端)
    uint16_t subtype;                // 消息子类型 (小端)
    uint16_t body_length;            // 消息体长度 (n) (小端)
    uint8_t* body;                   // 消息体 (n bytes)
    uint16_t crc;                    // CRC校验 (小端)
    uint8_t footer;                  // 消息包尾 (0xCC)
} Packet;
#pragma pack()

// 协议消息动作代码
typedef enum {
    ACTION_REPORT = 0x01,             // 状态上报
    ACTION_GET = 0x02,                // 主动查询
    ACTION_SET = 0x03,                // 主动设置
    ACTION_REPLY = 0x04,              // 消息回复
    ACTION_NET = 0x05,                // 组网
    ACTION_ERROR = 0xFF               // 异常错误
} ActionCode;

// 协议消息类型代码
typedef enum {
    TYPE_DEFAULT = 0x00,             // 默认通用
    TYPE_BATTERY = 0x01,             // 电池管理
    TYPE_VFD = 0x02                  // 线切割变频器
} MessageType;

// 其他相关宏定义
#define HEADER_VALUE 0xBB            // 消息包头值
#define FOOTER_VALUE 0xCC            // 消息包尾值
#define MAX_BODY_LENGTH 256          // 消息体最大长度（可根据需求调整）

#define PROTOCOL_MEM_POOL_BYTES          2 * 1024
extern uint32_t protocol_mem[PROTOCOL_MEM_POOL_BYTES / sizeof(uint32_t)];

#define PROTOCOL_MALLOC(size) \
    ({ \
        VOID* ptr; \
        UINT status; \
        status = tx_byte_allocate(&g_protocol_mem_pool, &ptr, (size), TX_NO_WAIT); \
        (status == TX_SUCCESS) ? ptr : NULL; \
    })

#define PROTOCOL_FREE(ptr) \
    do { \
        UINT status; \
        if (ptr != NULL) { \
            status = tx_byte_release(ptr); \
            /* 如果需要处理错误，可以在此处添加错误处理代码 */ \
        } \
    } while (0)

typedef int (*CallbackRead)(uint8_t* buff, uint16_t len, uint32_t timeout);

// 辅助函数声明
void create_packet(Packet* packet, ActionCode action, MessageType type, uint16_t source_id, uint16_t target_id, uint16_t subtype, uint8_t* body, uint16_t body_length);
int  serialize_packet(const Packet* packet, uint8_t* buffer);
uint16_t deserialize_packet(const uint8_t* buffer, uint16_t length , Packet* packet);
uint16_t deserialize_packet_byte(CallbackRead funcRead, int timeout, Packet* packet);
uint16_t calculate_modbus_crc(const uint8_t* data, uint16_t length);
uint16_t calculate_modbus_crc_2(const uint8_t* data,uint8_t* data_body, uint16_t length,uint16_t lenth_body);
void print_packet(const Packet* packet);
void protocol_mem_init();
Packet* packet_allocate(uint16_t body_length);
void packet_free(Packet* packet);
void packet_body_free(Packet* packet);
#endif // PROTOCOL_H