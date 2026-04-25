#include "service_test.h"
#include "tx_api.h"
#include "tx_queue.h"
#include "log.h"
#include "service_motor.h"

/*线程参数*/

#define  CFG_TASK_TEST_PRIO                          3u
#define  CFG_TASK_TEST_STK_SIZE                    1024u
static  TX_THREAD   task_test_tcb;
static  ULONG64     task_test_stk[CFG_TASK_TEST_STK_SIZE/8];
static  void        task_test          (ULONG thread_input);

/*队列参数*/
#define  QUEUE_TEST_MAX_NUM                          8
static TX_QUEUE g_test_queue = {0};
static UINT g_test_queue_addr[QUEUE_TEST_MAX_NUM] = {0};


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

#if 0
static int do_handler(MSG_MGR_T* msg)
{
    logdbg("[%d] recv %d bytes\n", msg_times, msg->len);
    loghex((unsigned char*) msg->buf , msg->len); 

    return 0;
}
#endif

static  void  task_test (ULONG thread_input)
{
	(void)thread_input;

#if 0	
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

#if 1
    tx_thread_sleep(3000);

    ext_motor_start(0,10);

    tx_thread_sleep(1000 * 5); /*5s后开始变频*/
    ext_motor_speed(30); /*电机变频*/
    
    tx_thread_sleep(1000 * 5); /*5s后开始变频*/
    ext_motor_speed(80); /*电机变频*/

    tx_thread_sleep(1000 * 5); /*5s后开始变频*/
    ext_motor_speed(50); /*电机变频*/

    tx_thread_sleep(1000 * 5); /*5s后开始变频*/
    ext_motor_speed(10); /*电机变频*/

    tx_thread_sleep(1000 * 5); /*5s后开始换向*/
    ext_motor_reverse(); /*电机换向*/

    tx_thread_sleep(1000 * 5); /*5s后开始变频*/
    ext_motor_speed(50); /*电机变频*/

    tx_thread_sleep(1000 * 5); /*5s后开始变频*/
    ext_motor_brake(); /*电机刹车*/
    
#endif
	while(1)
	{
        tx_thread_sleep(500);
	}

#endif
}