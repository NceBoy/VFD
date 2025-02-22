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

/*创建定时器，用来实时检查反向时刻*/
static VOID motor_reverse_tmr_cb(ULONG);
static tmr_t g_motor_reverse_tmr = {0};
static float g_motor_current_freq = 0.0f;

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
            logdbg("motor start in dir = %d , frequency = %d\n",data[0],data[1]);
        } break; /*变频*/
        case MSG_ID_MOTOR_VF      : {
            assert(msg->len == 4);
            unsigned int freq = 0;
            memcpy(&freq,msg->buf,msg->len);
            motor_target_freq_update((float) freq);
            logdbg("motor change frequency to %d\n",freq);
        } break; /*变频*/
        case MSG_ID_MOTOR_REVERSE : {
            g_motor_current_freq = motor_current_freq_get();
            motor_target_freq_update(5.0);
            tmr_start(&g_motor_reverse_tmr);
            motor_status_set(motor_in_reverse);
            logdbg("motor start reverse, current frequency = %d\n",(int)g_motor_current_freq);
        } break; /*换向*/
        case MSG_ID_MOTOR_BREAK : {
            motor_target_freq_update(5.0);
            tmr_start(&g_motor_reverse_tmr);
            motor_status_set(motor_in_break);
            logdbg("motor start break\n");
        } break; /*刹车*/        
        default:break;
    }
    return 0;
}


static  void  task_motor (ULONG thread_input)
{
	(void)thread_input;

    MSG_MGR_T *msg = NULL;

    tmr_init(&g_motor_reverse_tmr ,"motor reverse" , motor_reverse_tmr_cb,5);

    tmr_create(&g_motor_reverse_tmr);

    cordic_init();

    motor_start(0,50.0);
    
	while(1)
	{
        if(nx_msg_recv(&g_motor_queue, &msg , 1000) == 0)
        {
            do_msg_handler(msg);
            nx_msg_free(msg);
        }
	}
}

VOID motor_reverse_tmr_cb(ULONG para)
{
    (void)para;
    if(motor_arrive_freq(5.0)) /*电机到达换向/刹车频率*/
    {
        motor_status_enum status = motor_status_get();
        if(status == motor_in_reverse)
        {
            motor_reverse(); /*反向*/
            motor_target_freq_update(g_motor_current_freq); /*反向后恢复之前的频率*/
        } 
        else if(status == motor_in_break)
        {
            motor_break();
        }
        tmr_stop(&g_motor_reverse_tmr);
    }
        

}