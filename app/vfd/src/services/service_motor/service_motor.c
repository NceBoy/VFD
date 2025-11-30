#include <assert.h>
#include "service_motor.h"
#include "service_data.h"
#include "bsp_adc.h"
#include "nx_msg.h"
#include "tx_api.h"
#include "tx_queue.h"
#include "log.h"
#include "cordic.h"
#include "motor.h"
#include "inout.h"
#include "param.h"

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
            int freq = *(int*)msg->buf;
            motor_target_info_update((float) freq);

        } break; 

        case MSG_ID_MOTOR_REVERSE : {
            motor_reverse_start();

        } break; 

        case MSG_ID_MOTOR_BRAKE : {
            motor_brake_start();

        } break;
        default:break;
    }
    return 0;
}


static  void  task_motor (ULONG thread_input)
{
	(void)thread_input;

    MSG_MGR_T *msg = NULL;

    param_load();

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


void ext_motor_start(unsigned int dir , unsigned int target)
{
    if(motor_is_working() == 1)
        return ;
    unsigned int data[2] = {dir,target};
    nx_msg_send(NULL, &g_motor_queue, MSG_ID_MOTOR_START, data, sizeof(data));
    ext_send_report_status(0,STATUS_START_CHANGE,1);
}

void ext_motor_speed(unsigned int speed)
{
    if(motor_is_running() == 0)
        return ;
    unsigned int freq = speed;
    nx_msg_send(NULL, &g_motor_queue, MSG_ID_MOTOR_VF, &freq, 4);
}

void ext_motor_reverse(void)
{
    if(motor_is_running() == 0)
        return ;
    nx_msg_send(NULL, &g_motor_queue, MSG_ID_MOTOR_REVERSE, NULL, 0);
    uint8_t dir = motor_target_current_dir() ? 0 : 1;
    ext_send_report_status(0,STATUS_DIRECTION_CHANGE,dir);
}

void ext_motor_brake(void)
{
    if(motor_is_running() == 0)
        return ;
    nx_msg_send(NULL, &g_motor_queue, MSG_ID_MOTOR_BRAKE, NULL, 0);
    ext_send_report_status(0,STATUS_START_CHANGE,0);
}
