#include <assert.h>
#include "service_motor.h"
#include "tx_api.h"
#include "tx_queue.h"
#include "log.h"
#include "cordic.h"
#include "motor.h"
#include "tmr.h"

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


static int do_msg_handler(MSG_MGR_T* msg)
{
    switch(msg->mtype)
    {
        case MSG_ID_MOTOR_START      : {
            assert(msg->len == 8);
            unsigned int data[2] = {0};
            memcpy(data,msg->buf,msg->len);
            motor_start(data[0],(float)data[1]);

        } break; 

        case MSG_ID_MOTOR_VF      : {
            assert(msg->len == 4);
            unsigned int freq = 0;
            memcpy(&freq,msg->buf,msg->len);
            motor_target_info_update((float) freq);

        } break; 

        case MSG_ID_MOTOR_REVERSE : {
            motor_reverse_start();

        } break; 

        case MSG_ID_MOTOR_BREAK : {
            motor_break_start();

        } break;      
        default:break;
    }
    return 0;
}


static  void  task_motor (ULONG thread_input)
{
	(void)thread_input;

    MSG_MGR_T *msg = NULL;

    cordic_init();

    motor_init();
    
	while(1)
	{
        if(nx_msg_recv(&g_motor_queue, &msg , 1000) == 0)
        {
            do_msg_handler(msg);
            nx_msg_free(msg);
        }
	}
}