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
        if(ret <= 0)
            return -1;
        ack_len = serialize_packet((const Packet*) &ack_pkt, g_ack_buf);
        bsp_uart_send(g_ack_buf , ack_len);
    }
    return 0;
}


static int do_handler(MSG_MGR_T* msg)
{
    loghex((unsigned char*) msg->buf , msg->len); 

    switch(msg->mtype)
    {
        case MSG_ID_UART_DATA:
            protocol_process(msg->buf , msg->len);
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



void ext_send_to_data(int from_id , unsigned char* buf , int len)
{
    nx_msg_send((TX_QUEUE*)from_id, &g_data_queue, MSG_ID_UART_DATA, buf, len);
}