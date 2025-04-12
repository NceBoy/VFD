#include "main.h"
#include "bsp_io.h"
#include "inout.h"
#include "service_motor.h"
#include "vfd_param.h"

#define EFFECTIVE_POLARITY_LOW              (0)
#define EFFECTIVE_POLARITY_HIGH             (1)

#define ERROR_LEFT_KEY                      0x01            //左限位长时间触发
#define ERROR_RIGHT_KEY                     0x02            //右限位长时间触发
#define ERROR_DOUBLE_KEY                    0x04            //左右限位同时触发
//#define ERROR_LEFT          0x08
//#define ERROR_LEFT          0x10
//#define ERROR_LEFT          0x20
//#define ERROR_LEFT          0x40
//#define ERROR_LEFT          0x80

typedef enum
{
    IO_ID_CTRL_MODE = 0,                //点动or四键控制
    IO_ID_EXCEED_POLARITY,              //超程极性
    IO_ID_END_POLARITY,                 //加工结束极性
    IO_ID_LR_POLARITY,                  //左右限位极性

    IO_ID_SP0,                          //速度调节0
    IO_ID_SP1,                          //速度调节1
    IO_ID_SP2,                          //速度调节2
    IO_ID_DEBUG,                        //调试模式

    IO_ID_LIMIT_LEFT,                   //左限位
    IO_ID_LIMIT_RIGHT,                  //右限位
    IO_ID_LIMIT_EXCEED,                 //超程限位
    IO_ID_END,                          //加工结束

    IO_ID_MOTOR_START,                  //开运丝
    IO_ID_MOTOR_STOP,                   //关运丝
    IO_ID_PUMP_START,                   //开水泵
    IO_ID_PUMP_STOP,                    //关水泵

    IO_ID_MAX,
}io_id_t;

typedef enum
{
    CTRL_MODE_JOG,          /*点动控制*/
    CTRL_MODE_FOUR_KEY,     /*四建控制*/
}ctl_mode_t;


typedef struct
{
    io_id_t         io_id;
    GPIO_TypeDef    *port;
    uint32_t        pin;
    uint32_t        now_ticks;
    uint32_t        debounce_ticks;
    uint32_t        exceed_ticks;
    uint32_t        active_polarity;
}vfd_io_t;



typedef struct 
{
    ctl_mode_t      ctl;
    uint8_t         flag[IO_ID_MAX];
    uint8_t         sp;
    uint8_t         err;
}vfd_ctrl_t;

static vfd_ctrl_t g_vfd_ctrl = {0};

static vfd_io_t g_vfd_io_tab[IO_ID_MAX] = {

    {IO_ID_CTRL_MODE            ,GPIOA, GPIO_PIN_12 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},//控制模式设置，点动or四键
    {IO_ID_EXCEED_POLARITY      ,GPIOA, GPIO_PIN_11 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},//超程极性控制
    {IO_ID_END_POLARITY         ,GPIOA, GPIO_PIN_10 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},//加工结束极性
    {IO_ID_LR_POLARITY          ,GPIOA, GPIO_PIN_9 ,    0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},//左右限位极性

    {IO_ID_SP0                  ,GPIOA, GPIO_PIN_3 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},
    {IO_ID_SP1                  ,GPIOA, GPIO_PIN_4 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},
    {IO_ID_SP2                  ,GPIOA, GPIO_PIN_5 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},
    {IO_ID_DEBUG                ,GPIOA, GPIO_PIN_6 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},//调试，有效电平0


    {IO_ID_LIMIT_LEFT           ,GPIOC, GPIO_PIN_0 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW}, //左限位NPN(根据常开常闭设置有效电平)
    {IO_ID_LIMIT_RIGHT          ,GPIOC, GPIO_PIN_1 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW}, //右限位NPN(根据常开常闭设置有效电平)
    {IO_ID_LIMIT_EXCEED         ,GPIOC, GPIO_PIN_2 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW}, //超程NPN接(根据常开常闭设置有效电平)
    {IO_ID_END                  ,GPIOC, GPIO_PIN_3 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW}, //加工结束(根据常开常闭设置有效电平)

    {IO_ID_MOTOR_START          ,GPIOB, GPIO_PIN_11 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},//开丝，常开开关，上拉，有效电平0
    {IO_ID_MOTOR_STOP           ,GPIOB, GPIO_PIN_12 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_HIGH},//关丝，常闭开关，下拉，有效电平1
    {IO_ID_PUMP_START           ,GPIOB, GPIO_PIN_13 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_LOW},//开水，常开开关，上拉，有效电平0
    {IO_ID_PUMP_STOP            ,GPIOB, GPIO_PIN_14 ,   0 , 100 , IO_MAX_TIME_MS ,EFFECTIVE_POLARITY_HIGH},//关水，常闭开关，下拉，有效电平1
};

/*外部IO的错误信息处理*/
static void update_ctrl(void)
{  
    
}

static void update_active_polarity(void)
{
    g_vfd_ctrl.ctl = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_CTRL_MODE].port, g_vfd_io_tab[IO_ID_CTRL_MODE].pin) == GPIO_PIN_SET) ? \
                    CTRL_MODE_JOG : CTRL_MODE_FOUR_KEY;

    g_vfd_io_tab[IO_ID_LIMIT_EXCEED].active_polarity = \
        (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_EXCEED_POLARITY].port, g_vfd_io_tab[IO_ID_EXCEED_POLARITY].pin) == GPIO_PIN_SET) ? \
        EFFECTIVE_POLARITY_LOW : EFFECTIVE_POLARITY_HIGH;

    g_vfd_io_tab[IO_ID_END].active_polarity = \
        (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_END_POLARITY].port, g_vfd_io_tab[IO_ID_END_POLARITY].pin) == GPIO_PIN_SET) ? \
        EFFECTIVE_POLARITY_LOW : EFFECTIVE_POLARITY_HIGH;

    if(HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_LR_POLARITY].port, g_vfd_io_tab[IO_ID_LR_POLARITY].pin) == GPIO_PIN_SET){
        g_vfd_io_tab[IO_ID_LIMIT_LEFT].active_polarity = EFFECTIVE_POLARITY_LOW;
        g_vfd_io_tab[IO_ID_LIMIT_RIGHT].active_polarity = EFFECTIVE_POLARITY_LOW;
    }       
    else{
        g_vfd_io_tab[IO_ID_LIMIT_LEFT].active_polarity = EFFECTIVE_POLARITY_HIGH;
        g_vfd_io_tab[IO_ID_LIMIT_RIGHT].active_polarity = EFFECTIVE_POLARITY_HIGH;        
    }
}

static void update_debug(void)
{
    g_vfd_ctrl.flag[IO_ID_DEBUG] = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_DEBUG].port, g_vfd_io_tab[IO_ID_DEBUG].pin) == GPIO_PIN_SET) ? 0 : 1;
}

static void ctrl_speed(void)
{
    if(g_vfd_ctrl.flag[IO_ID_SP0] != 0) /*用手控盒控制过速度*/
    {   /*通知手控盒取消相关显示*/

    }
    uint8_t speed = 0;
    pullOneItem(PARAM0X01, g_vfd_ctrl.sp, &speed);
    ext_motor_speed(speed);  
}

static void update_speed(void)
{  
    uint8_t sp0 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP0].port, g_vfd_io_tab[IO_ID_SP0].pin) == GPIO_PIN_SET) ? 1 : 0;
    uint8_t sp1 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP1].port, g_vfd_io_tab[IO_ID_SP1].pin) == GPIO_PIN_SET) ? 1 : 0;
    uint8_t sp2 = (HAL_GPIO_ReadPin(g_vfd_io_tab[IO_ID_SP2].port, g_vfd_io_tab[IO_ID_SP2].pin) == GPIO_PIN_SET) ? 1 : 0;

    uint8_t sp = (sp2 << 2) | (sp1 << 1) | sp0 ;
    if(sp != g_vfd_ctrl.sp)
    {
        g_vfd_ctrl.sp = sp;
        ctrl_speed();
    }
}

static void ctrl_dir(void)
{

}

static void update_direction(void)
{  
    for(int i = IO_ID_LIMIT_LEFT ; i <= IO_ID_END ; i++)
    {
        if(HAL_GPIO_ReadPin(g_vfd_io_tab[i].port, g_vfd_io_tab[i].pin) == g_vfd_io_tab[i].active_polarity)
        {
            if(g_vfd_io_tab[i].now_ticks < IO_MAX_TIME_MS)
                g_vfd_io_tab[i].now_ticks += IO_SCAN_INTERVAL;
        }
        else
        {
            g_vfd_ctrl.flag[i] = 0;
            g_vfd_io_tab[i].now_ticks = 0;
        }
            

        if(g_vfd_io_tab[i].now_ticks > g_vfd_io_tab[i].debounce_ticks)
        {
            if(g_vfd_ctrl.flag[i] == 0)
            {
                g_vfd_ctrl.flag[i] = 1;
                /*通知电机换向或者结束加工*/
                ctrl_dir();
            }   
        }
    }
    //左限位长时间触发，异常
    if(g_vfd_io_tab[IO_ID_LIMIT_LEFT].now_ticks >= g_vfd_io_tab[IO_ID_LIMIT_LEFT].exceed_ticks) 
        g_vfd_ctrl.err |= ERROR_LEFT_KEY;
        
    //右限位长时间触发，异常
    if(g_vfd_io_tab[IO_ID_LIMIT_RIGHT].now_ticks >= g_vfd_io_tab[IO_ID_LIMIT_RIGHT].debounce_ticks) 
        g_vfd_ctrl.err |= ERROR_RIGHT_KEY;

    /*左右限位同时触发*/
    if((g_vfd_ctrl.flag[IO_ID_LIMIT_LEFT] == 1) && (g_vfd_ctrl.flag[IO_ID_LIMIT_RIGHT] == 1))
        g_vfd_ctrl.err |= ERROR_DOUBLE_KEY;

}

static void ctrl_onoff(void)
{

}

static void update_onoff(void)
{  
    for(int i = IO_ID_MOTOR_START ; i <= IO_ID_PUMP_STOP ; i++)
    {
        if(HAL_GPIO_ReadPin(g_vfd_io_tab[i].port, g_vfd_io_tab[i].pin) == g_vfd_io_tab[i].active_polarity)
        {
            if(g_vfd_io_tab[i].now_ticks < IO_MAX_TIME_MS)
                g_vfd_io_tab[i].now_ticks += IO_SCAN_INTERVAL;
        }
        else
        {
            g_vfd_io_tab[i].now_ticks = 0;
            g_vfd_ctrl.flag[i] = 0;
        }
            

        if(g_vfd_io_tab[i].now_ticks > g_vfd_io_tab[i].debounce_ticks)
        {
            if(g_vfd_ctrl.flag[i] == 0)
            {
                g_vfd_ctrl.flag[i] = 1;
                /*通知开或者关*/
                ctrl_onoff();
            }
        }       
    }
}

/*手控盒控速时，同步速度*/
void inout_sp_sync_from_ext(uint8_t sp)
{
    g_vfd_ctrl.sp = sp;
    g_vfd_ctrl.flag[IO_ID_SP0] = 1;
}




void inout_init(void) 
{
    /*16个输入信号线*/
    bsp_io_init_input();
    /*2个输出信号线*/
    bsp_io_init_output();

    return ;
}

void inout_scan(void)
{
    /*step 1 . 极性*/
    update_active_polarity();
    /*step 2 . 速度及调试模式*/
    update_debug();
    update_speed();
    /*step 3 . 限位*/
    update_direction();
    /*step 4 . 开关运丝和水泵*/
    update_onoff();

    /*step 5 . 根据上述结果控制电机*/
    update_ctrl();
}