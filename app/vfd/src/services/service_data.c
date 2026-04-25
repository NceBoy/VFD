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
#include "pending_msg.h"

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
TX_TIMER report_check_timer;
static void report_timer_expire(ULONG id);

/*ack 应答全局缓冲区*/
static uint8_t g_ack_buf[1024];

void service_data_start(void)
{
    tx_timer_create(&report_check_timer , "report_check_timer", report_timer_expire, 0, 20, 500, TX_AUTO_ACTIVATE);
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
    packet pkt = {0};
    packet ack_pkt = {0};
    int ack_len = 0;

    if(buf2packet((const uint8_t*) buf, len , &pkt))
    {
        int ret = data_process(&pkt , &ack_pkt);
        packet_body_free(&pkt);
        if(ret < 0)
            return ret;
        ack_len = packet2buf((const packet*) &ack_pkt, g_ack_buf);
        bsp_uart_send(g_ack_buf , ack_len);
        //logdbg("ack_len: %d\n", ack_len);
        //loghex(g_ack_buf,ack_len);
    }
    return 0;
}

static void report_timer_expire(ULONG id)
{
    (void)id;
    nx_msg_send(NULL, &g_data_queue, MSG_ID_UART_REPORT_CHECK, NULL, 0);
}

static const char* get_status_str(unsigned short status)
{
    switch(status)
    {
        case 0x0001 : return "speed change";
        case 0x0002 : return "pump change";
        case 0x0004 : return "start change";
        case 0x0008 : return "direction change";
        case 0x0010 : return "mode change";
        case 0x0020 : return "high freq change";
        default: return "unknow status";
    }
}

static const char* get_err_str(int index)
{
    switch(index)
    {
        case 0: return "wire break";
        case 1: return "left key trigger timeout";
        case 2: return "right key trigger timeout";
        case 3: return "double key trigger sync";
        case 4: return "exceed trigger";
        case 5: return "over voltage";
        case 6: return "under voltage";
        case 7: return "ipm error";
        case 8: return "reverse timeout";
        default:return "unknow";
    }
}
static void uart_report_status(unsigned char* buf , int len)
{
    /* 上报状态*/
    packet out = {0};
    uint32_t tid = get_next_tid();
    create_packet(&out, ACTION_REPORT, TYPE_VFD, tid, 0x1234, 0x0001, 0xFE00, buf, len);
    int ack_len = packet2buf((const packet*) &out, g_ack_buf);
    bsp_uart_send(g_ack_buf , ack_len);
    uint16_t err = buf[1] << 8 | buf[0];
    uint16_t status = buf[3] << 8 | buf[2];
    if(status != 0){ //上报状态，status==0上报的是错误
        logdbg("report status[0x%04x], tid = %d, %s : %d %d\n",status,tid,get_status_str(status),buf[4],buf[5]);
    }
    else
    {
        logdbg("report err[0x%04x], tid = %d\n",err,tid);
        for(int i = 0; i < 16; i++ )
        {
            if(err & (1 << i))
            {
                logdbg("err:%s\n",get_err_str(i));
            }
        }
    }
    //loghex(g_ack_buf,ack_len);
    
    /*将上报消息加入待发送队列*/
    pending_msg_add(tid, g_ack_buf, ack_len);
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
        case MSG_ID_UART_REPORT_CHECK:
            pending_msg_check();
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



void ext_send_buf_to_data(int from_id , unsigned char* buf , int len , int type)
{
    nx_msg_send((TX_QUEUE*)from_id, &g_data_queue, (MSG_ID)type, buf, len);
}


void ext_send_report_err(int from_id , unsigned short err)
{
    unsigned char buf[6] = {0};
    buf[0] = err & 0xFF;
    buf[1] = err >> 8 & 0xFF;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    nx_msg_send((TX_QUEUE*)from_id, &g_data_queue, MSG_ID_UART_REPORT_DATA, buf, sizeof(buf));
}

extern unsigned short inout_get_err(void);

void ext_send_report_status(int from_id , unsigned short status , unsigned char value1 , unsigned char value2 )
{
    /*上报状态时需要把错误码一起上报，否则手持会认为错误消失*/
    unsigned short err = inout_get_err();
    unsigned char buf[6] = {0};
    buf[0] = err & 0xFF; /*错误码*/
    buf[1] = err >> 8 & 0xFF;
    buf[2] = status & 0xFF;
    buf[3] = status >> 8 & 0xFF;
    buf[4] = value1;
    buf[5] = value2;
    nx_msg_send((TX_QUEUE*)from_id, &g_data_queue, MSG_ID_UART_REPORT_DATA, buf, sizeof(buf));
    //logdbg("report status: %s : %d %d\n",get_status_str(status),value1,value2);
}
