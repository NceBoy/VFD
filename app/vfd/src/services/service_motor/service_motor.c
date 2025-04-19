#include <assert.h>
#include "service_motor.h"
#include "nx_msg.h"
#include "tx_api.h"
#include "tx_queue.h"
#include "log.h"
#include "cordic.h"
#include "motor.h"
#include "inout.h"

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
/*IO扫描定时器*/
static VOID io_tmr_cb(ULONG);
static TX_TIMER g_io_tmr = {0};

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
    
    inout_init();

    cordic_init();

    motor_init();
    
    /*创建一个定时器，以100ms的间隔采集电压电流*/
    tx_timer_create(&g_io_tmr,"io scan",io_tmr_cb,0,100,5,TX_AUTO_ACTIVATE); 
    
    
	while(1)
	{
        if(nx_msg_recv(&g_motor_queue, &msg , 1000) == 0)
        {
            do_msg_handler(msg);
            nx_msg_free(msg);
        }
	}
}


void ext_motor_start(unsigned int dir , unsigned int target)
{
    if(motor_is_working() == 1)
        return ;
    unsigned int data[2] = {dir,target};
    nx_msg_send(NULL, &g_motor_queue, MSG_ID_MOTOR_START, data, sizeof(data));
}

void ext_motor_speed(unsigned int speed)
{
    if(motor_is_working() == 0)
        return ;
    unsigned int freq = speed;
    nx_msg_send(NULL, &g_motor_queue, MSG_ID_MOTOR_VF, &freq, 4);
}

void ext_motor_reverse(void)
{
    if(motor_is_working() == 0)
        return ;
    nx_msg_send(NULL, &g_motor_queue, MSG_ID_MOTOR_REVERSE, NULL, 0);
}

void ext_motor_break(void)
{
    if(motor_is_working() == 0)
        return ;
    nx_msg_send(NULL, &g_motor_queue, MSG_ID_MOTOR_BREAK, NULL, 0);
}

static VOID io_tmr_cb(ULONG para)
{
    (void)para;
    //inout_scan();
}