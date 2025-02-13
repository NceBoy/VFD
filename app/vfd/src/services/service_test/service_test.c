#include "service_test.h"
#include "tx_api.h"
#include "tx_queue.h"
#include "log.h"


#define  QUEUE_TEST_MAX_NUM                          8
#define  CFG_TASK_TEST_PRIO                          3u
#define  CFG_TASK_TEST_STK_SIZE                    1024u
static  TX_THREAD   task_test_tcb;
static  ULONG64    task_test_stk[CFG_TASK_TEST_STK_SIZE/8];
static  void  task_test          (ULONG thread_input);
static TX_QUEUE g_test_queue = {0};
static UINT g_test_queue_addr[QUEUE_TEST_MAX_NUM] = {0};
static int msg_times = 0;
void service_test_start(void)
{
    nx_msg_queue_create(&g_test_queue, "test queue",(VOID *)g_test_queue_addr, sizeof(g_test_queue_addr));
    tx_thread_create(&task_test_tcb,
                    "task test",
                    task_test,
                    0,
                    &task_test_stk[0],
                    CFG_TASK_TEST_STK_SIZE,
                    CFG_TASK_TEST_PRIO,
                    CFG_TASK_TEST_PRIO,
                    TX_NO_TIME_SLICE,
                    TX_AUTO_START);
}

msg_addr service_test_get_addr(void)
{
    return &g_test_queue;
}


static int do_handler(MSG_MGR_T* msg)
{
    logdbg("[%d] recv %d bytes\n", msg_times, msg->len);
    loghex((unsigned char*) msg->buf , msg->len); 

    return 0;
}


static  void  task_test (ULONG thread_input)
{
	(void)thread_input;

#if 1	
	MSG_MGR_T* recv_info  = NULL;
	
	while(1)
	{
		if(nx_msg_recv(&g_test_queue,&recv_info,5000) == 0)
		{
            msg_times++;
			/*handle the message and free it*/
            do_handler(recv_info);
			nx_msg_free(recv_info);
		}
	}
#else
    
	while(1)
	{

	}

#endif
}