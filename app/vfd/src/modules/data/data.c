#include <stdint.h>
#include <string.h>
#include "ble.h"
#include "nx_msg.h"
#include "bsp_uart.h"
#include "service_data.h"
#include "data.h"
#include "utility.h"

// UART接收缓冲区
static uint8_t g_uart_buffer[256];
static uint16_t g_uart_buffer_length;
static uint8_t g_connect_ok = 0;
static void uart_process(char* buf, int len);

typedef int (*protocol_cb)(Packet* in , Packet* out);

#define CMD_BUILD(action,main_type,sub_type)        (action << 16 | sub_type << 8 | main_type)

#define CMD_PARAM_SET           CMD_BUILD(ACTION_SET,0xFF,0x00)
#define CMD_MOTOR_SET           CMD_BUILD(ACTION_SET,0x04,0x01)

#define CMD_PARAM_GET           CMD_BUILD(ACTION_GET,0xFF,0x00)

typedef struct
{
    uint32_t            cmd;
    protocol_cb         cb;
}data_item_t;

static int vfd_param_set(Packet* in , Packet* out);
static int vfd_param_get(Packet* in , Packet* out);
static int vfd_motor_set(Packet* in , Packet* out);


static const data_item_t data_tab[] = {
    {CMD_PARAM_SET , vfd_param_set},
    {CMD_PARAM_GET , vfd_param_get},
    {CMD_MOTOR_SET , vfd_motor_set},
};


/**
 * @brief 在缓冲区中查找指定标志位
 * @param buf 缓冲区指针
 * @param len 缓冲区长度
 * @param flag 要查找的标志位
 * @return 找到标志位的索引，未找到返回-1
 */
static int search_flag(char* buf, int len, char flag)
{
    for(int i = 0; i < len; i++)
    {
        if(buf[i] == flag)
            return i;
    }
    return -1;
}

/**
 * @brief 计算数据包总长度
 * @param buf 数据包缓冲区
 * @return 数据包总长度
 */
static int get_total_packet_length(char* buf)
{
    int data_len = (int)(buf[10] | buf[11] << 8);
    return (data_len + 15);
}



/**
 * @brief 检查数据包有效性
 * @param buf 数据包缓冲区
 * @param len 数据包长度
 * @return 0表示有效，-1表示无效
 */
static int check_packet_validity(char* buf, int len)
{
    uint16_t calculated_crc = modbus_crc16((unsigned char*)buf, len - 3);
    uint16_t packet_crc = buf[len - 3] | buf[len - 2] << 8;
    
    if(packet_crc != calculated_crc)
        return -1;
    
    return 0;
}

/**
 * @brief 解析数据流，提取有效数据包
 * @param buf 输入缓冲区
 * @param len 输入缓冲区长度
 * @param start_ptr 指向合法数据包起始位置的指针
 * @param real_length 指向合法数据包长度的指针
 * @param offset_ptr 指向需要偏移的字节数的指针
 * @return 解析结果：0-成功，负数表示各种错误
 */
static int parse_stream(uint8_t *buf, int len, char **start_ptr, int *real_length, int *offset_ptr)
{
    int total_length = 0;
    char* current_ptr = (char*)buf;
    int offset = search_flag(current_ptr, len, 0xBB);
    
    if(offset < 0)  // 没有找到起始标志0xBB
    {
        *offset_ptr = -1;
        return -1;
    }
    
    current_ptr += offset;
    
    // 检查是否能获取到长度字段
    if(len < (offset + 12))
    {
        *offset_ptr = offset;
        return -2;
    }
    
    total_length = get_total_packet_length(current_ptr);
    
    // 检查完整数据包是否都在缓冲区内
    if ((offset + total_length) > len)
    {
        *offset_ptr = offset;
        return -3;
    }
    
    // 检查结束符是否正确
    if (current_ptr[total_length-1] != 0xCC)
    {
        *offset_ptr = offset + 1;
        return -4;
    }

    // 校验数据包
    int ret = check_packet_validity(current_ptr, total_length);
    if (ret < 0)
    {
        *offset_ptr = offset + total_length;
        return -5;
    }

    *start_ptr = current_ptr;
    *real_length = total_length;
    *offset_ptr = (offset + total_length);
    return 0;
}

/**
 * @brief 通信数据处理函数
 */
static void comm_data_handler(void)
{
    int ret = 0, offset = 0;
    char* data_ptr = NULL;
    uint16_t data_length = 0;
  
    while(g_uart_buffer_length >= 15)/*消息体为空时，长度为15字节*/
    {
        if(g_connect_ok == 0)
        {
            if(strstr((char*)g_uart_buffer , "CONNECT") != NULL)
                g_connect_ok = 1;            
        }

        ret = parse_stream(g_uart_buffer, g_uart_buffer_length, &data_ptr, (int*)&data_length, &offset);
        if(ret == 0)
        {
            /* 协议处理 */
            uart_process(data_ptr, data_length);
        }
        else
        {
            if (offset < 0)
            {
                g_uart_buffer_length = 0;
                break;
            }
            else if (0 == offset)
            {
                break; 
            }
        }
        
        g_uart_buffer_length -= offset;
        memmove(g_uart_buffer, g_uart_buffer + offset, g_uart_buffer_length);
    }
    return;	
}

/**
 * @brief 数据轮询处理函数
 * @return 0表示成功
 */
int data_poll(void)
{
    if(ble_get_state() == 0) /*蓝牙重连时，不接收数据*/
        return 0;

    int bytes_received = 0;

    bytes_received = bsp_uart_recv_all(g_uart_buffer + g_uart_buffer_length, 
                                 sizeof(g_uart_buffer) - g_uart_buffer_length);
    
    g_uart_buffer_length += bytes_received;
    if (g_uart_buffer_length < 15)
        return 0;	
    
    
    comm_data_handler();	
    
    return 0;
}

static void uart_process(char* buf, int len)
{
    ext_send_to_data(NULL, (unsigned char*)buf, len);
}


int data_process(Packet* in , Packet* out)
{
    if(in->type != TYPE_VFD)
        return -1;
    uint32_t cmd = (in->action << 16) | in->subtype ;
    for(int i = 0 ; i < sizeof(data_tab) / sizeof(data_tab[0]) ; i++)
    {
        if((data_tab[i].cmd == cmd) && (data_tab[i].cb != NULL))
        {
            return data_tab[i].cb(in , out);
        }   
    }
    return -1;
}


static int vfd_param_set(Packet* in , Packet* out)
{
    return 0;
}
static int vfd_param_get(Packet* in , Packet* out)
{
    return 0;
}
static int vfd_motor_set(Packet* in , Packet* out)
{
    return 0;
}