#include "service_hmi.h"
#include "tx_api.h"
#include "tx_queue.h"
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


static  void  task_hmi (ULONG thread_input)
{
	(void)thread_input;

    hmi_init();

	while(1)
	{
        hmi_scan_key();
        tx_thread_sleep(50);
	}
}