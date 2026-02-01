#include "protocol.h"
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "tx_byte_pool.h"
#include "nx_malloc.h"

static uint32_t g_tid = 0;

uint32_t get_next_tid(void)
{
    return g_tid++;
}



void create_packet( packet* packet, 
                    ActionCode action, 
                    DevType dtype, 
                    uint32_t tid,
                    uint16_t source_id, 
                    uint16_t target_id, 
                    uint16_t mtype, 
                    uint8_t* body, 
                    uint16_t body_length) 
{
    packet->header = HEADER_VALUE;
    packet->action = (uint8_t)action;
    packet->dtype = (uint8_t)dtype;
    packet->reserved = 0; // 预留字段
    packet->tid = tid; // 小端存储
    packet->source_id = source_id; // 小端存储
    packet->target_id = target_id; // 小端存储
    packet->mtype = mtype; // 小端存储
    packet->body_length = body_length; // 小端存储
    if(packet->body_length != 0)
    {
        packet->body = (uint8_t*)nx_malloc(body_length);
        if(packet->body)
            memcpy(packet->body, body, body_length);  
        else 
            packet->body_length = 0;
    }

    // 计算CRC时，从 header 到 crc 之前的字段都需要参与计算
    uint8_t* data = (uint8_t*)packet;
    uint16_t crc = calculate_modbus_crc_2(data,body, 12,packet->body_length);
    packet->crc = crc; // 存储为小端
    packet->footer = FOOTER_VALUE;
}

int packet2buf(const packet* packet, uint8_t* buffer)
{
    uint8_t* temp = buffer;
    *temp++ = packet->header;
    *temp++ = packet->dtype;
    *temp++ = packet->action;
    *temp++ = packet->reserved; // 预留字段

    /*消息唯一流水号*/
    *temp++ = (packet->tid >> 0) & 0xFF;  // 低字节
    *temp++ = (packet->tid >> 8) & 0xFF;  // 高字节
    *temp++ = (packet->tid >> 16) & 0xFF;  // 高字节
    *temp++ = (packet->tid >> 24) & 0xFF;  // 高字节
    // 源设备ID和目标设备ID以小端存储
    *temp++ = (packet->source_id >> 0) & 0xFF;  // 低字节
    *temp++ = (packet->source_id >> 8) & 0xFF;  // 高字节

    *temp++ = (packet->target_id >> 0) & 0xFF;  // 低字节
    *temp++ = (packet->target_id >> 8) & 0xFF;  // 高字节

    *temp++ = (packet->mtype >> 0) & 0xFF;    // 低字节
    *temp++ = (packet->mtype >> 8) & 0xFF;    // 高字节

    *temp++ = (packet->body_length >> 0) & 0xFF; // 低字节
    *temp++ = (packet->body_length >> 8) & 0xFF; // 高字节

    if(packet->body_length)
    {
        memcpy(temp, packet->body, packet->body_length);
        nx_free(packet->body);
    }
    temp += packet->body_length;
    
    *temp++ = (packet->crc >> 0) & 0xFF;        // 低字节
    *temp++ = (packet->crc >> 8) & 0xFF;        // 高字节

    *temp++ = packet->footer;
    return temp - buffer;
}


uint16_t buf2packet(const uint8_t* buffer, uint16_t length , packet* packet) 
{
    if(length < 19)
        return 0;

    const uint8_t* temp = buffer;
    uint16_t body_length = temp[14] | (temp[15] << 8);
    //验证包长
    if(length != (body_length + 19))
        return 0;
    //验证包头和包尾
    if (temp[0] != HEADER_VALUE || temp[length -1] != FOOTER_VALUE) 
        return 0;
    //验证CRC
    uint16_t crc_pack = temp[16 + body_length] | (temp[17 + body_length] << 8);
    uint16_t expected_crc = calculate_modbus_crc(temp, 16 + body_length);
    if (crc_pack != expected_crc)
        return 0;

    packet->header = *temp++;
    packet->dtype = *temp++;
    packet->action = *temp++;

    packet->reserved = *temp++; // 预留字段

    // 消息唯一流水号
    packet->tid = (temp[3] << 24) | (temp[2] << 16) | (temp[1] << 8) | temp[0];
    temp += 4;

    // 源设备ID和目标设备ID
    packet->source_id = (temp[1] << 8) | temp[0];
    temp += 2;
    packet->target_id = (temp[1] << 8) | temp[0];
    temp += 2;

    // 子类型和消息体长度
    packet->mtype = (temp[1] << 8) | temp[0];
    temp += 2;
    packet->body_length = (temp[1] << 8) | temp[0];
    temp += 2;

    if(packet->body_length != 0)
    {
        packet->body = (uint8_t*)nx_malloc(packet->body_length);
        memcpy(packet->body, temp, packet->body_length);        
    }

    temp += packet->body_length;

    // CRC和包尾
    packet->crc = (temp[1] << 8) | temp[0];
    temp += 2;
    packet->footer = *temp++;


    return 1;
}

uint16_t calculate_modbus_crc(const uint8_t* data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    uint16_t i, j;
    for (i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    // 返回小端格式的CRC值（十六进制字节顺序：低字节在前）
    return crc;
}

uint16_t calculate_modbus_crc_2(const uint8_t* data,uint8_t* data_body, uint16_t length,uint16_t lenth_body) {
    uint16_t crc = 0xFFFF;
    uint16_t i, j;
    for (i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    for (i = 0; i < lenth_body; i++) {
        crc ^= (uint16_t)data_body[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    // 返回小端格式的CRC值（十六进制字节顺序：低字节在前）
    return crc;
}



packet* packet_allocate(uint16_t body_length) 
{
    if (body_length == 0) {
        return NULL;
    }
    
    packet* pkt = (packet*)nx_malloc(sizeof(packet));
    if (pkt == NULL) {
        return NULL;
    }
    pkt->body = NULL;
    return pkt;
}

// 新增函数：释放Packet内存
void packet_free(packet* packet) 
{
    if (packet == NULL) {
        return;
    }
    if(packet->body != NULL)
        nx_free(packet->body);
    nx_free(packet);
}

void packet_body_free(packet* packet)
{
    if(packet->body != NULL)
        nx_free(packet->body);
}
