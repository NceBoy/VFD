#include "nx_msg.h"
#include "service_hmi.h"
#include "tx_api.h"
#include "tx_queue.h"
#include "tx_thread.h"
#include "log.h"
#include "hmi.h"

/*线程参数*/

#define  CFG_TASK_HMI_PRIO                          3u
#define  CFG_TASK_HMI_STK_SIZE                    1024u
static  TX_THREAD   task_hmi_tcb;
static  ULONG64     task_hmi_stk[CFG_TASK_HMI_STK_SIZE/8];
static  void        task_hmi          (ULONG thread_input);

/*队列参数*/
#define  QUEUE_HMI_MAX_NUM                          8
static TX_QUEUE g_hmi_queue = {0};
static UINT g_hmi_queue_addr[QUEUE_HMI_MAX_NUM] = {0};

/*按键扫描定时器*/
TX_TIMER key_scan_timer;
static void key_scan_timer_expire(ULONG id);

void service_hmi_start(void)
{
    nx_msg_queue_create(&g_hmi_queue, "hmi queue",(VOID *)g_hmi_queue_addr, sizeof(g_hmi_queue_addr));
    tx_thread_create(&task_hmi_tcb,
                     "task hmi",
                     task_hmi,
                     0,
                     &task_hmi_stk[0],
                     CFG_TASK_HMI_STK_SIZE,
                     CFG_TASK_HMI_PRIO,
                     CFG_TASK_HMI_PRIO,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);
}

msg_addr service_hmi_get_addr(void)
{
    return &g_hmi_queue;
}

static int do_msg_handler(MSG_MGR_T* msg)
{
    switch(msg->mtype)
    {
        case MSG_ID_KEY_SCAN :{
            hmi_scan_key();
        }break;
        case MSG_ID_STOP_CODE :{
            int code = *(int*)msg->buf;
            hmi_stop_code(code);
        }break;
        default: break;
    }
    return 0;
}
static  void  task_hmi (ULONG thread_input)
{
	(void)thread_input;

    hmi_init();

    MSG_MGR_T *msg = NULL;

    tx_timer_create(&key_scan_timer , "key_scan_timer", key_scan_timer_expire, 0, 50, 50, TX_AUTO_ACTIVATE);

	while(1)
	{
        if(nx_msg_recv(&g_hmi_queue, &msg , 1000) == 0)
        {
            do_msg_handler(msg);
            nx_msg_free(msg);
        }
	}
}

void ext_notify_stop_code(unsigned char code)
{
    int stop_code = code;
    nx_msg_send(NULL, &g_hmi_queue, MSG_ID_STOP_CODE, &stop_code, 4);
}


static void key_scan_timer_expire(ULONG id)
{
    nx_msg_send(NULL, &g_hmi_queue, MSG_ID_KEY_SCAN, NULL, 0);
}