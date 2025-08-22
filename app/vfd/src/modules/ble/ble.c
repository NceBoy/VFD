#include "bsp_uart.h"
#include "ble.h"
#include "ringbuffer.h"
#include <string.h>
#include "main.h"

static uint8_t ble_response[512];
static uint32_t ble_id = 0;
static uint8_t ble_state = 0;

static int ble_send_cmd(const char *cmd) 
{ 
    unsigned char* pcmd = (unsigned char*)cmd;
    bsp_uart_send(pcmd, strlen(cmd));
    return 0;
}

int ble_get_state(void)
{
    return ble_state;
}


void ble_set_state(int state)
{
    ble_state = state;
}

/**
 * 发送BLE AT指令并等待预期响应
 * 该函数会发送AT指令并等待指定的响应，支持重试机制和超时控制
 * 
 * @param cmd: 要发送的AT指令字符串
 * @param expected_response: 期望收到的响应字符串
 * @param retry: 最大重试次数
 * @param timeout: 单次等待响应的超时时间(毫秒)
 * @return: 0表示成功收到预期响应，-1表示超时或失败
 */
int ble_send_and_wait(const char *cmd, const char *expected_response , uint8_t retry , uint32_t timeout) { 

    if (!cmd || !expected_response) {
        return -1;
    }

    uint8_t retry_count = 0;
    uint32_t send_time = 0;

    while (retry_count < retry) { 

        memset(ble_response, 0, sizeof(ble_response));

        bsp_uart_clear();

        ble_send_cmd(cmd);

        send_time = HAL_GetTick();

        while(HAL_GetTick() - send_time < timeout) { 
            
            size_t bytes_read = bsp_uart_recv_all(ble_response, sizeof(ble_response) - 1);

            if (bytes_read > 0) {
                // 确保字符串以null结尾
                ble_response[bytes_read] = '\0';
                
                // 查找预期响应
                if (strstr((char *)ble_response, expected_response) != NULL) {
                    return 0; // 成功收到预期响应
                }
            }
            HAL_Delay(10);
        }
        retry_count++;
    }
    return -1;
}

/**
 * 等待指定的响应字符串
 * 该函数在不发送任何数据的情况下，持续监听并接收ring buffer中的数据，
 * 直到找到指定的响应字符串或超时为止
 * 
 * @param expected_response: 期望等待的响应字符串
 * @param timeout: 等待超时时间(毫秒)
 * @return: 0表示成功收到预期响应，-1表示超时
 */
int ble_wait(const char *expected_response, uint32_t timeout) {

    if (!expected_response) {
        return -1;
    }
    
    memset(ble_response, 0, sizeof(ble_response));

    uint32_t start_time = HAL_GetTick();
    
    while (HAL_GetTick() - start_time < timeout) {

        size_t bytes_read = bsp_uart_recv_all(ble_response, sizeof(ble_response) - 1);
        
        if (bytes_read > 0) {
            // 确保字符串以null结尾
            ble_response[bytes_read] = '\0';
            
            // 查找预期响应
            if (strstr((char *)ble_response, expected_response) != NULL) {
                return 0; // 成功收到预期响应
            }
        }
        
        // 短暂延时，避免过度占用CPU
        HAL_Delay(10);
    }
    
    // 超时未收到预期响应
    return -1;
}

void ble_id_get(void)
{
    ble_id = HAL_GetUIDw0() + HAL_GetUIDw1() + HAL_GetUIDw2();
}

int ble_init(void)
{
    bsp_uart_init();
    /*获取一个蓝牙名称,使用芯片的ID*/
    ble_id_get();
    ble_set_state(1);
    return 0;
}

int ble_connect(void)
{
start:
    /*复位*/
    
    /*查询模组名称*/
    if(ble_send_and_wait("AT+BLENAME?\r\n", "VFD" , 1 , 100) != 0)/*模组名称不对*/
    {
        char send_buf[32] = {0};
        snprintf(send_buf , sizeof(send_buf) , "AT+BLENAME=VFD%08X\r\n" , ble_id);
        ble_send_and_wait(send_buf, "OK" , 1 , 100);
        goto start;
    }
    ble_send_and_wait("AT+BLEMTU=244\r\n", "OK" , 1 , 100);
    return 0;
}