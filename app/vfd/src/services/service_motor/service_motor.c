#include "service_motor.h"
#include "tx_api.h"
#include "tx_queue.h"
#include "log.h"
#include "cordic.h"

/*线程参数*/

#define  CFG_TASK_MOTOR_PRIO                          1u
#define  CFG_TASK_MOTOR_STK_SIZE                    1024u
static  TX_THREAD   task_motor_tcb;
static  ULONG64     task_motor_stk[CFG_TASK_MOTOR_STK_SIZE/8];
static  void        task_motor          (ULONG thread_input);

/*队列参数*/
#define  QUEUE_MOTOR_MAX_NUM                          8
static TX_QUEUE g_motor_queue = {0};
static UINT g_motor_queue_addr[QUEUE_MOTOR_MAX_NUM] = {0};



void service_motor_start(void)
{
    nx_msg_queue_create(&g_motor_queue, "motor queue",(VOID *)g_motor_queue_addr, sizeof(g_motor_queue_addr));
    tx_thread_create(&task_motor_tcb,
                     "task motor",
                     task_motor,
                     0,
                     &task_motor_stk[0],
                     CFG_TASK_MOTOR_STK_SIZE,
                     CFG_TASK_MOTOR_PRIO,
                     CFG_TASK_MOTOR_PRIO,
                     TX_NO_TIME_SLICE,
                     TX_AUTO_START);
}

msg_addr service_motor_get_addr(void)
{
    return &g_motor_queue;
}

#if 0
static int do_handler(MSG_MGR_T* msg)
{
    //logdbg("[%d] recv %d bytes\n", msg_times, msg->len);
    //loghex((unsigned char*) msg->buf , msg->len); 

    return 0;
}
#endif

float sin = 0.0f;
float rad = 0.0f;
unsigned int times = 0;
float angle = 0.0f;



static  void  task_motor (ULONG thread_input)
{
	(void)thread_input;

    cordic_init();
    
		
	while(1)
	{
        cordic_sin(rad, &sin);
        angle = 57.29578f * public_rad_convert(rad);
        rad += 0.01f;
        times++;
        logdbg("[%d] sin[%d] = %d \n", times, (int)(angle * 100) ,(int)(sin * 10000));
        tx_thread_sleep(1000);
	}

}