#include "main.h"


/*cordic的输入范围为[-pi~pi],根据sin的周期性
  将弧度转换至该范围内 */
float public_rad_convert(float rad)
{
    float tempf = rad;
    if(tempf > 0){
        while(tempf > M_PI)
            tempf -= PI_2;
    }else{
        while(tempf < -M_PI)
            tempf += PI_2;        
    }
    return tempf;
}

void cordic_init(void)
{ 
    __HAL_RCC_CORDIC_CLK_ENABLE(); 

#if 0
    CORDIC_ConfigTypeDef sCordicConfig;

    sCordicConfig.Function         = CORDIC_FUNCTION_SINE;     //选择计算正弦
    sCordicConfig.Precision        = CORDIC_PRECISION_4CYCLES; //选择计算精度等级
    sCordicConfig.Scale            = CORDIC_SCALE_0;           //选择计算系数
    sCordicConfig.NbWrite          = CORDIC_NBWRITE_2;         //选择计算结果个数
    sCordicConfig.NbRead           = CORDIC_NBREAD_1;          //选择输出正弦
    sCordicConfig.InSize           = CORDIC_INSIZE_32BITS;
      //选择输入数据格式Q1.31，在Q1.31格式的数字范围：-1 (0x80000000) to 1 至 2^(-31) (0x7FFFFFFF).
    sCordicConfig.OutSize          = CORDIC_OUTSIZE_32BITS;    //选择数据输出格式Q1.31
    HAL_CORDIC_Configure(&hcordic, &sCordicConfig);            //初始化
#endif
}

float cordic_sin(float angle)
{
    float angle_pi = public_rad_convert(angle);  /* 弧度转换到-pi~pi */
	/* Q31,two write, one read, sine calculate, 4 precision */
	CORDIC->CSR = 0x00100041;
	/* Write data into WDATA */
	CORDIC->WDATA = (int32_t )(angle_pi * RADIAN_Q31_f);
	/* Modulus is m=1 */
	CORDIC->WDATA = 0x7FFFFFFF;
	/* Get sin value in float */
	return ((int32_t)CORDIC->RDATA)*1.0f/Q31; 
}

float cordic_cos(float angle)
{
    float angle_pi = public_rad_convert(angle);  /* 弧度转换到-pi~pi */
	/* Q31,two write, one read, cosine calculate, 4 precision */
	CORDIC->CSR = 0x00100040;
	/* Write data into WDATA */
	CORDIC->WDATA = (int32_t )(angle_pi * RADIAN_Q31_f);
	/* Modulus is m=1 */
	CORDIC->WDATA = 0x7FFFFFFF;
	/* Get sin value in float */
	return ((int32_t)CORDIC->RDATA)*1.0f/Q31; 
}

void cordic_sin_cos(float angle, float *sin , float *cos)
{
    float angle_pi = public_rad_convert(angle);  /* 弧度转换到-pi~pi */
	/* Q31,two write, one read, sine calculate, 4 precision */
	CORDIC->CSR = 0x00180041;
	/* Write data into WDATA */
	CORDIC->WDATA = (int32_t )(angle_pi * RADIAN_Q31_f);
	/* Modulus is m=1 */
	CORDIC->WDATA = 0x7FFFFFFF;
	/* Get sin value in float */
    if(sin)
        *sin = ((int32_t)CORDIC->RDATA)*1.0f/Q31;
    /* Get cos value in float */
    if(cos)
        *cos = ((int32_t)CORDIC->RDATA)*1.0f/Q31;
	
}

void cordic_sqrt_atan2(float x, float y, float *mag, float *phase)
{
    // 估算最大值，避免饱和
    float abs_x = (x >= 0) ? x : -x;
    float abs_y = (y >= 0) ? y : -y;
    float max_val = (abs_x > abs_y) ? abs_x : abs_y;

    if (max_val == 0.0f) {
        *mag = 0.0f;
        *phase = 0.0f;
        return;
    }

    int32_t x_q15 = (int32_t)(x * Q31);
    int32_t y_q15 = (int32_t)(y * Q31);

    //RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;

    CORDIC->CSR = CORDIC_FUNCTION_MODULUS
                | CORDIC_PRECISION_4CYCLES  // 迭代次数
                | CORDIC_SCALE_0
                | CORDIC_NBREAD_2       //2个结果
                | CORDIC_NBWRITE_2      //2个输入
                | CORDIC_OUTSIZE_32BITS    //32位
                | CORDIC_INSIZE_32BITS;   //32位

    CORDIC->WDATA = x_q15;
    CORDIC->WDATA = y_q15;

    //while ((CORDIC->CSR & CORDIC_CSR_RRDY) == 0);

    int32_t mag_q15   = (int32_t)CORDIC->RDATA;
    int32_t phase_q16 = (int32_t)CORDIC->RDATA;
    
    if(mag)
        *mag   = (float)mag_q15 * 1.0f / Q31;      // ∈ [0, √2]
    if(phase)
        *phase = (float)phase_q16 * 1.0f / Q31 * M_PI;    // ∈ [-π, π]
}