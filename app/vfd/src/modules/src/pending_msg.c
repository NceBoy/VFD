#include <assert.h>
#include "tx_api.h"
#include "tx_queue.h"
#include "tx_byte_pool.h"
#include "service_data.h"
#include "log.h"
#include "pending_msg.h"
#include "nx_malloc.h"
#include "bsp_uart.h"

#define MAX_RETRIES                 3
#define MAX_PENDING_MSGS            10
#define PENDING_MSG_TIMEOUT         (TX_TIMER_TICKS_PER_SECOND)

typedef struct {
    unsigned int msg_id;     // 消息唯一标识
    unsigned char* buf;      // 消息内容
    int len;                 // 消息长度
    unsigned int send_time;         // 发送时间（系统滴答数）
    int retry_count;         // 已重试次数
} pending_msg_t;


static pending_msg_t g_pending_msgs[MAX_PENDING_MSGS];
static int g_pending_count = 0;

void pending_msg_add(unsigned int msg_id, unsigned char* buf, int len)
{
    if((buf == NULL) || (len == 0))
        return;
    if (g_pending_count >= MAX_PENDING_MSGS) {
        logdbg("Pending queue is full, dropping message.\n");
        return;
    }
    
    pending_msg_t* msg = &g_pending_msgs[g_pending_count++];
    msg->msg_id = msg_id;
    msg->buf = (unsigned char*)nx_malloc(len);
    if(msg->buf == NULL)
    {
        logdbg("malloc pending msg buf failed.\n");
        return;
    }
    memcpy(msg->buf, buf, len);
    msg->len = len;
    msg->send_time = tx_time_get();  // 获取当前系统时间
    msg->retry_count = 0;
}

void pending_msg_remove(int index)
{
    if (index < 0 || index >= g_pending_count) {
        return;
    }

    // 释放内存
    if(g_pending_msgs[index].buf != NULL)
    {
        nx_free(g_pending_msgs[index].buf);
        g_pending_msgs[index].buf = NULL;        
    }

    // 前移后续元素
    for (int i = index; i < g_pending_count - 1; ++i) {
        g_pending_msgs[i] = g_pending_msgs[i + 1];
    }

    --g_pending_count;
}

int pending_msg_find_and_remove(unsigned int msg_id)
{
    for (int i = 0; i < g_pending_count; ++i) 
    {
        if (g_pending_msgs[i].msg_id == msg_id) 
        {
            // 找到匹配的消息，删除它
            pending_msg_remove(i);
            logdbg("Message ID %d found and removed.\n", msg_id);
            return 0;  // 成功删除
        }
    }

    // 没有找到匹配的消息
    logdbg("Message ID %d not found.\n", msg_id);
    return -1;  // 未找到
}

void pending_msg_check(void)
{
    if(g_pending_count == 0)
        return;
    ULONG current_time = tx_time_get();
    for (int i = 0; i < g_pending_count; ++i) 
    {
        pending_msg_t* msg = &g_pending_msgs[i];

        // 超时判断（假设超时时间为 5 秒）
        if ((current_time - msg->send_time) > (PENDING_MSG_TIMEOUT)) 
        {
            if (msg->retry_count < MAX_RETRIES) 
            {
                // 重发消息
                bsp_uart_send(msg->buf , msg->len);
                msg->send_time = current_time;
                msg->retry_count++;
                logdbg("Retrying message ID %d, retry count: %d\n", msg->msg_id, msg->retry_count);
            } 
            else 
            {
                // 达到最大重试次数，丢弃消息
                logdbg("Max retries reached for message ID %d, dropping.\n", msg->msg_id);
                pending_msg_remove(i);
                --i;  // 队列前移，调整索引
            }
        }
    }
}