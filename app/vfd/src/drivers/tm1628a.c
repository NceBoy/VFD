#include "tm1628a.h"


static gpio_t dio = {GPIOA, GPIO_PIN_4};
static gpio_t clk = {GPIOA, GPIO_PIN_5};
static gpio_t stb = {GPIOA, GPIO_PIN_6};

//static uint8_t code_addr[5] = {0xC0 , 0xC2, 0xC4, 0xC6, 0xC8};
/*共阴极数码管编码表*/
//static uint8_t code_table[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, \
    0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71}; //# 0-F

#define dio_set()       HAL_GPIO_WritePin(dio.port, dio.pin, GPIO_PIN_SET)
#define dio_reset()     HAL_GPIO_WritePin(dio.port, dio.pin, GPIO_PIN_RESET)
#define dio_read()      HAL_GPIO_ReadPin(dio.port, dio.pin)

#define clk_set()       HAL_GPIO_WritePin(clk.port, clk.pin, GPIO_PIN_SET)
#define clk_reset()     HAL_GPIO_WritePin(clk.port, clk.pin, GPIO_PIN_RESET)

#define stb_set()       HAL_GPIO_WritePin(stb.port, stb.pin, GPIO_PIN_SET)
#define stb_reset()     HAL_GPIO_WritePin(stb.port, stb.pin, GPIO_PIN_RESET)


void tm1628a_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_SET);
    /*Configure GPIO pins : PA4 PA5 PA6 */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);   
    
    /*IO脚兼顾输入和输出，配置为开漏上拉*/
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);     

    stb_set();
    clk_set();
    dio_set();

    HAL_Delay(10);

    tm1628a_set_displaymode(0x03);
    tm1628a_set_brightness(4);
    tm1628a_clear();
}

static void delay_us(uint32_t us)
{
    uint32_t delay = us * 170 / 4;
    do{
        __NOP();
    }while(delay--);
}
static void send_command(uint8_t data)
{
    delay_us(1);
    for(int i = 0; i < 8; i++)
    {
        if(data & 0x01)
            dio_set();
        else
            dio_reset();       
        clk_reset();
        delay_us(1);
        clk_set();
        delay_us(1);
        data >>= 1;
    }
}

void tm1628a_set_displaymode(uint8_t mode)
{
    stb_reset();
    send_command(mode);     /*显示模式设置,7位10段*/
    stb_set();   
}

/*地址自增写*/
void tm1628a_write_continuous(uint8_t* data , uint8_t length)
{
    stb_reset();   
    send_command(0x40);     /*地址自增*/
    stb_set(); 
    delay_us(1); 
    stb_reset();  
    send_command(0xC0);     /*起始地址为第一个数码管*/
    for(int i = 0; i < length; i++)
        send_command(data[i]);
    delay_us(1);    
    stb_set();   
}

void tm1628a_clear(void)
{
    uint8_t data[10] = {0};
    tm1628a_write_continuous(data, sizeof(data));
}

/*地址固定写*/
void tm1628a_write_once(uint8_t addr , uint8_t data)
{
    stb_reset();
    send_command(0x44);     /*地址固定*/
    stb_set();   
    delay_us(1);
    stb_reset();
    send_command(addr); 
    send_command(data);
    delay_us(1); 
    stb_set();   
}
/*亮度等级1~8,设置为0时关闭显示*/
void tm1628a_set_brightness(uint8_t level)
{ 
    if(level > 8)
        level = 8;

    uint8_t value = 0;
    if(level  == 0)
        value = 0x80;
    else
        value = 0x88 | (level - 1);
    stb_reset();
    send_command(value);
    stb_set();
}

void tm1628a_read_key(uint8_t *key)
{ 
    stb_reset();
    send_command(0x42);     /*读按键*/
    delay_us(2);
    dio_set();
    for (int j = 0; j < 5; j++) //连续读取5个字节
    {
        for (int i = 0; i < 8; i++)
        {
            key[j] = key[j] >> 1;
            clk_reset();
            delay_us(1);
            clk_set();
            if (dio_read())
            {
                key[j] = key[j] | 0x80;
            }
        }
        if (key[j] == 0xff) key[j] = 0;
    }
    stb_set(); 
}