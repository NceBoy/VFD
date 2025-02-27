#include "protocol.h"
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "tx_byte_pool.h"

uint32_t protocol_mem[PROTOCOL_MEM_POOL_BYTES / sizeof(uint32_t)] = {0};
static TX_BYTE_POOL g_protocol_mem_pool = {0};

void* protocol_malloc(uint32_t len) {
    uint8_t ret = 0;
    uint8_t* ptr;
    ret = tx_byte_allocate(&g_protocol_mem_pool, (VOID **)&ptr, len, TX_NO_WAIT);
    if (ret != TX_SUCCESS)
        return NULL;
    return ptr;
}

void protocol_free(void* ptr) {
    uint8_t ret;
    ret = tx_byte_release(ptr);
    if (ret != TX_SUCCESS) {
        // 内存释放失败，可能需要进一步处理
        // printf("Memory release failed with status: %d\n", ret);
    }
}

void create_packet(Packet* packet, ActionCode action, MessageType type, uint16_t source_id, uint16_t target_id, uint16_t subtype, uint8_t* body, uint16_t body_length) {
    packet->header = HEADER_VALUE;
    packet->action = (uint8_t)action;
    packet->type = (uint8_t)type;
    packet->reserved = 0; // 预留字段
    packet->source_id = source_id; // 小端存储
    packet->target_id = target_id; // 小端存储
    packet->subtype = subtype; // 小端存储
    packet->body_length = body_length; // 小端存储
    packet->body = (uint8_t*)protocol_malloc(body_length);
    memcpy(packet->body, body, body_length);
    // 计算CRC时，从 header 到 crc 之前的字段都需要参与计算
    uint8_t* data = (uint8_t*)packet;
    uint16_t crc = calculate_modbus_crc(data, packet->body_length+12);
    packet->crc = crc; // 存储为小端
    packet->footer = FOOTER_VALUE;
}

void serialize_packet(const Packet* packet, uint8_t* buffer) {
    uint8_t* temp = buffer;
    *temp++ = packet->header;
    *temp++ = packet->action;
    *temp++ = packet->type;
    *temp++ = packet->reserved; // 预留字段
    // 源设备ID和目标设备ID以小端存储
    *temp++ = (packet->source_id >> 8) & 0xFF;  // 高字节
    *temp++ = (packet->source_id >> 0) & 0xFF;  // 低字节
    *temp++ = (packet->target_id >> 8) & 0xFF;  // 高字节
    *temp++ = (packet->target_id >> 0) & 0xFF;  // 低字节
    *temp++ = (packet->subtype >> 8) & 0xFF;    // 高字节
    *temp++ = (packet->subtype >> 0) & 0xFF;    // 低字节
    *temp++ = (packet->body_length >> 8) & 0xFF; // 高字节
    *temp++ = (packet->body_length >> 0) & 0xFF; // 低字节
    memcpy(temp, packet->body, packet->body_length);
    temp += packet->body_length;
    *temp++ = (packet->crc >> 8) & 0xFF;        // 高字节
    *temp++ = (packet->crc >> 0) & 0xFF;        // 低字节
    *temp++ = packet->footer;
}

uint16_t deserialize_packet_byte(CallbackRead funcRead, int timeout, Packet* packet) {
    uint8_t* data = (uint8_t*)packet;
    uint16_t expected_crc;
    if (funcRead == NULL)
        goto exit2;

    // 读取并判断包头
    if (funcRead(&packet->header, 1, timeout) != 1)
        goto exit2;
    if (packet->header != HEADER_VALUE)
        goto exit2;

    // 读取动作、类型、预留字段、源设备ID、目标设备ID、源设备ID和目标设备ID
    if (funcRead((uint8_t*)&packet->action, 11, timeout) != 11) // 包括 action, type, reserved, source_id, target_id,subtype, body_length
        goto exit2;

    // 读取消息体
    packet->body = (uint8_t*)protocol_malloc(packet->body_length);
    if (packet->body == NULL)
        goto exit2;
    if (funcRead(packet->body, packet->body_length, timeout) != packet->body_length)
        goto exit1;

    // 读取CRC和包尾
    uint8_t crc_buffer[2]; // 缓存两个字节的CRC
    if (funcRead(crc_buffer, 2, timeout) != 2)
        goto exit1;
    packet->crc = (crc_buffer[1] << 8) | crc_buffer[0]; 
    // 验证CRC，无需转换大小端
    data = (uint8_t*)packet;
    expected_crc = calculate_modbus_crc(data, packet->body_length+12);
    if (packet->crc != expected_crc)
        goto exit1;

    if (funcRead(&packet->footer, 1, timeout) != 1)
        goto exit1;

    // 验证包头和包尾
    if (packet->footer != FOOTER_VALUE)
        goto exit1;

    return packet->body_length + 15; // 包头、动作、类型、预留、源设备ID、目标设备ID、子类型、消息体长度、CRC、包尾
exit1:
    protocol_free(packet->body);
exit2:
    return 0;
}

uint16_t deserialize_packet(const uint8_t* buffer, Packet* packet) {
    const uint8_t* temp = buffer;
    packet->header = *temp++;
    packet->action = *temp++;
    packet->type = *temp++;
    packet->reserved = *temp++; // 预留字段

    // 源设备ID和目标设备ID
    packet->source_id = (temp[0] << 8) | temp[1];
    temp += 2;
    packet->target_id = (temp[0] << 8) | temp[1];
    temp += 2;

    // 子类型和消息体长度
    packet->subtype = (temp[0] << 8) | temp[1];
    temp += 2;
    packet->body_length = (temp[0] << 8) | temp[1];
    temp += 2;

    packet->body = (uint8_t*)protocol_malloc(packet->body_length);
    memcpy(packet->body, temp, packet->body_length);
    temp += packet->body_length;

    // CRC和包尾
    packet->crc = (temp[0] << 8) | temp[1];
    temp += 2;
    packet->footer = *temp++;

    // 验证包头和包尾
    if (packet->header != HEADER_VALUE || packet->footer != FOOTER_VALUE) {
        protocol_free(packet->body);
        return 0;
    }

    // 验证CRC
    uint8_t* data = (uint8_t*)packet;
    uint16_t expected_crc = calculate_modbus_crc(data, packet->body_length+12);
    if (packet->crc != expected_crc) { 
        protocol_free(packet->body);
        return 0;
    }

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

void print_packet(const Packet* packet) {
    //printf("Packet:\n");
    //printf("  Header: 0x%02X\n", packet->header);
    //printf("  Action: 0x%02X\n", packet->action);
    //printf("  Type: 0x%02X\n", packet->type);
    //printf("  Reserved: 0x%02X\n", packet->reserved);
    //printf("  Source ID: 0x%04X\n", packet->source_id);
    //printf("  Target ID: 0x%04X\n", packet->target_id);
    //printf("  Subtype: 0x%04X\n", packet->subtype);
    //printf("  Body Length: %d\n", packet->body_length);
    //printf("  Body: ");
    for (uint16_t i = 0; i < packet->body_length; i++) {
        //printf("%02X ", packet->body[i]);
    }
    //printf("\n");
    //printf("  CRC: 0x%04X\n", packet->crc);
    //printf("  Footer: 0x%02X\n", packet->footer);
}

void protocol_mem_init() {
    uint32_t ret = 0;
    ret = tx_byte_pool_create(&g_protocol_mem_pool, "protocol_mem", (VOID*)protocol_mem, PROTOCOL_MEM_POOL_BYTES);
    assert(ret == TX_SUCCESS);
}