#include "main.h"
#include "bsp_io.h"
#include "tx_api.h"
#include "tmr.h"

#define EFFECTIVE_POLARITY_LOW              (0)
#define EFFECTIVE_POLARITY_HIGH             (1)

#define TMR_DEFINE(name) \
    static tmr_t        name##_tmr; \
    static VOID         name##_tmr_cb(ULONG para);

typedef enum
{
    IO_ID_LIMIT_LEFT = 0,
    IO_ID_LIMIT_RIGHT,
    IO_ID_LIMIT_OVER,
    IO_ID_SHUTDOWN,
//    IO_ID_SP0,
//    IO_ID_SP1,
//    IO_ID_SP2,
//    IO_ID_DEBUG,
    IO_ID_MOTOR_START,
    IO_ID_MOTOR_STOP,
    IO_ID_PUMP_START,
    IO_ID_PUMP_STOP,
    IO_ID_MAX,
}io_id_t;

typedef enum
{
    CTRL_MODE_JOG,          /*点动控制*/
    CTRL_MODE_FOUR_KEY,     /*四建控制*/
}ctl_mode_t;

typedef struct
{
    io_id_t         io_id;          /*增加ID属性，便于查找，而不使用name匹配*/
    char            io_name[16];
    tmr_t*          debounce_tmr;
    tmr_callback    debounce_tmr_cb;
    unsigned int    debounce_ticks;
    unsigned int    effective_polarity;
}io_t;

static ctl_mode_t g_ctl_mode = CTRL_MODE_JOG;

TMR_DEFINE(left)
TMR_DEFINE(right)
TMR_DEFINE(over)
TMR_DEFINE(shutdown)
//TMR_DEFINE(sp0)
//TMR_DEFINE(sp1)
//TMR_DEFINE(sp2)
//TMR_DEFINE(debug)
TMR_DEFINE(motor_start)
TMR_DEFINE(motor_stop)
TMR_DEFINE(pump_start)
TMR_DEFINE(pump_stop)


static io_t g_vfd_io[IO_ID_MAX] = {
    {IO_ID_LIMIT_LEFT,  "limit_left",   &left_tmr,      left_tmr_cb ,           5,EFFECTIVE_POLARITY_LOW}, //左限位NPN(根据常开常闭设置有效电平)
    {IO_ID_LIMIT_RIGHT, "limit_right",  &right_tmr,     right_tmr_cb,           5,EFFECTIVE_POLARITY_LOW}, //右限位NPN(根据常开常闭设置有效电平)
    {IO_ID_LIMIT_OVER,  "limit_over",   &over_tmr,      over_tmr_cb,            5,EFFECTIVE_POLARITY_LOW}, //超程NPN接(根据常开常闭设置有效电平)
    {IO_ID_SHUTDOWN,    "shutdown",     &shutdown_tmr,  shutdown_tmr_cb,        5,EFFECTIVE_POLARITY_LOW}, //加工结束(根据常开常闭设置有效电平)
//    {IO_ID_SP0,         "sp0",          &sp0_tmr,       sp0_tmr_cb,             5,EFFECTIVE_POLARITY_LOW},
//    {IO_ID_SP1,         "sp1",          &sp1_tmr,       sp1_tmr_cb,             5,EFFECTIVE_POLARITY_LOW},
//    {IO_ID_SP2,         "sp2",          &sp2_tmr,       sp2_tmr_cb,             5,EFFECTIVE_POLARITY_LOW},
//    {IO_ID_DEBUG,       "debug",        &debug_tmr,     debug_tmr_cb,           5,EFFECTIVE_POLARITY_LOW},//调试，有效电平0
    {IO_ID_MOTOR_START, "motor_start",  &motor_start_tmr,motor_start_tmr_cb,    5,EFFECTIVE_POLARITY_LOW},//开丝，常开开关，上拉，有效电平0
    {IO_ID_MOTOR_STOP,  "motor_stop",   &motor_stop_tmr,motor_stop_tmr_cb,      5,EFFECTIVE_POLARITY_HIGH},//关丝，常闭开关，上拉，有效电平1
    {IO_ID_PUMP_START,  "pump_start",   &pump_start_tmr,pump_start_tmr_cb,      5,EFFECTIVE_POLARITY_LOW},//开水，常开开关，上拉，有效电平0
    {IO_ID_PUMP_STOP,   "pump_stop",    &pump_stop_tmr, pump_stop_tmr_cb,       5,EFFECTIVE_POLARITY_HIGH},//关水，常闭开关，上拉，有效电平1
};

static void init_all_io_tmr(void)
{
    io_t* pio = NULL;
    for(int i = 0 ; i < IO_ID_MAX ; i++)
    {
        pio = &g_vfd_io[i];
        tmr_init(pio->debounce_tmr ,pio->io_name , pio->debounce_tmr_cb, pio->io_id, pio->debounce_ticks);
        tmr_create(pio->debounce_tmr);
    }
}

static void io_interrupt_process(io_id_t id)
{
    if(id >= IO_ID_MAX)
        return ;   
    io_t* pio = &g_vfd_io[id];
    tmr_start(pio->debounce_tmr);
}

void io_init(void) /*总共16个输入信号线*/
{
    /*step 1 . init all input and output pin*/
    bsp_io_init_input();
    bsp_io_init_output();
    /*step2 . get input set and effective polarity*/


    /*step3 . init debounce timer*/
    init_all_io_tmr();

    /*step4 . modify the polarity according to the hardware.*/
    return ;
}

/*左限位*/
static VOID left_tmr_cb(ULONG para)
{
    io_id_t id = (io_id_t)para;
    io_t* pio = &g_vfd_io[id];
    tmr_stop(pio->debounce_tmr);

    return;
}

/*右限位*/
static VOID right_tmr_cb(ULONG para)
{
    io_id_t id = (io_id_t)para;
    io_t* pio = &g_vfd_io[id];
    tmr_stop(pio->debounce_tmr);

    return;
}

/*超程*/
static VOID over_tmr_cb(ULONG para)
{
    io_id_t id = (io_id_t)para;
    io_t* pio = &g_vfd_io[id];
    tmr_stop(pio->debounce_tmr);

    return;
}

/*结束*/
static VOID shutdown_tmr_cb(ULONG para)
{
    io_id_t id = (io_id_t)para;
    io_t* pio = &g_vfd_io[id];
    tmr_stop(pio->debounce_tmr);

    return;
}


/*开运丝*/
static VOID motor_start_tmr_cb(ULONG para)
{
    io_id_t id = (io_id_t)para;
    io_t* pio = &g_vfd_io[id];
    tmr_stop(pio->debounce_tmr);

    return;
}


/*关运丝*/
static VOID motor_stop_tmr_cb(ULONG para)
{
    io_id_t id = (io_id_t)para;
    io_t* pio = &g_vfd_io[id];
    tmr_stop(pio->debounce_tmr);

    return;
}

/*开水泵*/
static VOID pump_start_tmr_cb(ULONG para)
{
    io_id_t id = (io_id_t)para;
    io_t* pio = &g_vfd_io[id];
    tmr_stop(pio->debounce_tmr);

    return;
}

/*关水泵*/
static VOID pump_stop_tmr_cb(ULONG para)
{
    io_id_t id = (io_id_t)para;
    io_t* pio = &g_vfd_io[id];
    tmr_stop(pio->debounce_tmr);

    return;
}


 void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
 {
    switch(GPIO_Pin)
    {
        case GPIO_PIN_0 : io_interrupt_process(IO_ID_LIMIT_LEFT); break;
        case GPIO_PIN_1 : io_interrupt_process(IO_ID_LIMIT_RIGHT); break;
        case GPIO_PIN_2 : io_interrupt_process(IO_ID_LIMIT_OVER); break;
        case GPIO_PIN_3 : io_interrupt_process(IO_ID_SHUTDOWN); break;

        case GPIO_PIN_11 : io_interrupt_process(IO_ID_MOTOR_START); break;
        case GPIO_PIN_12 : io_interrupt_process(IO_ID_MOTOR_STOP); break;
        case GPIO_PIN_13 : io_interrupt_process(IO_ID_PUMP_START); break;
        case GPIO_PIN_14 : io_interrupt_process(IO_ID_PUMP_STOP); break;

        default : break;
    }

 }