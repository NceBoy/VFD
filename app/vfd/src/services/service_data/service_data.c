#include "service_data.h"
#include "tx_api.h"
#include "tx_queue.h"
#include "log.h"
#include "service_motor.h"
#include "nx_msg.h"
#include "protocol.h"
#include "data.h"
#include "ble.h"
#include "bsp_uart.h"

/*线程参数*/

#define  CFG_TASK_DATA_PRIO                          2u
#define  CFG_TASK_DATA_STK_SIZE                    1024u
static  TX_THREAD   task_data_tcb;
static  ULONG64     task_data_stk[CFG_TASK_DATA_STK_SIZE/8];
static  void        task_data          (ULONG thread_input);

/*队列参数*/
#define  QUEUE_DATA_MAX_NUM                          10
static TX_QUEUE g_data_queue = {0};
static UINT g_data_queue_addr[QUEUE_DATA_MAX_NUM] = {0};


/*ack 应答全局缓冲区*/
static uint8_t g_ack_buf[1024];

void service_data_start(void)
{
    
    nx_msg_queue_create(&g_data_queue, "data queue",(VOID *)g_data_queue_addr, sizeof(g_data_queue_addr));
    tx_thread_create(&task_data_tcb,
                     "task data",
                     task_data,
                     0,
                     &task_data_stk[0],
                     CFG_TASK_DATA_STK_SIZE,
                     CFG_TASK_DATA_PRIO,
                     CFG_TASK_DATA_PRIO,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);
}

static int protocol_process(unsigned char* buf , int len)
{ 
    Packet pkt = {0};
    Packet ack_pkt = {0};
    int ack_len = 0;

    if(deserialize_packet((const uint8_t*) buf, len , &pkt))
    {
        int ret = data_process(&pkt , &ack_pkt);
        packet_body_free(&pkt);
        if(ret < 0)
            return ret;
        ack_len = serialize_packet((const Packet*) &ack_pkt, g_ack_buf);
        bsp_uart_send(g_ack_buf , ack_len);
        //loghex(g_ack_buf,ack_len);
    }
    return 0;
}

static void uart_report_status(unsigned char* buf , int len)
{
    /* 上报状态*/
    Packet out = {0};
    create_packet(&out, ACTION_REPORT, TYPE_VFD, 0, ble_get_id(), 0xFE00, buf, len);
    int ack_len = serialize_packet((const Packet*) &out, g_ack_buf);
    bsp_uart_send(g_ack_buf , ack_len);
    logdbg("report:");
    loghex(g_ack_buf,ack_len);
}


static int do_handler(MSG_MGR_T* msg)
{
    switch(msg->mtype)
    {
        case MSG_ID_UART_RECV_DATA:
            protocol_process(msg->buf , msg->len);
            break;
        case MSG_ID_UART_REPORT_DATA:
            uart_report_status(msg->buf , msg->len);
            break;
        case MSG_ID_BLE_RECONNECT:
            ble_set_state(0);
            ble_connect();
            ble_set_state(1);
            break;
        default:
            loginfo("unknow msg type %d",msg->mtype);
            break;
    }
    return 0;
}


static  void  task_data (ULONG thread_input)
{
	(void)thread_input;
    
    protocol_mem_init();    

	MSG_MGR_T* recv_info  = NULL;
	
	while(1)
	{
		if(nx_msg_recv(&g_data_queue,&recv_info,5000) == 0)
		{
			/*handle the message and free it*/
            do_handler(recv_info);
			nx_msg_free(recv_info);
		}
	}

}



void ext_send_buf_to_data(int from_id , unsigned char* buf , int len)
{
    nx_msg_send((TX_QUEUE*)from_id, &g_data_queue, MSG_ID_UART_RECV_DATA, buf, len);
}

/*
*  上报数据
*  @param from_id: 源id
*  @param d1: err
*  @param d2: 速度
*  @param d3: 水泵  0-关 1-运行
*  @param d4: 启停  0-停止 1-运行
*  @param d5: 方向  0-正转 1-反转
*  @param d6: 模式  0-正常 1-调试
*/
void ext_send_report_to_data(int from_id , unsigned char d1, unsigned char d2, 
    unsigned char d3, unsigned char d4, unsigned char d5, unsigned char d6)
{
    unsigned char buf[6] = {d1,d2,d3,d4,d5,d6};
    nx_msg_send((TX_QUEUE*)from_id, &g_data_queue, MSG_ID_UART_REPORT_DATA, buf, sizeof(buf));
}

void ext_send_notify_to_data(int from_id)
{
    nx_msg_send((TX_QUEUE*)from_id, &g_data_queue, MSG_ID_BLE_RECONNECT, NULL, 0);
}