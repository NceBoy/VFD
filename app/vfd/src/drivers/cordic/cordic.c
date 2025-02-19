#include "main.h"


static CORDIC_HandleTypeDef hcordic;

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
    hcordic.Instance = CORDIC;
    HAL_CORDIC_Init(&hcordic);

#if 0
    CORDIC_ConfigTypeDef sCordicConfig;

    sCordicConfig.Function         = CORDIC_FUNCTION_SINE;     //选择计算正弦
    sCordicConfig.Precision        = CORDIC_PRECISION_6CYCLES; //选择计算精度等级
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
    float angle_pi = public_rad_convert(angle);
	/* Q31,two write, one read, sine calculate, 6 precision */
	hcordic.Instance->CSR = 0x00100061;
	/* Write data into WDATA */
	hcordic.Instance->WDATA = (int32_t )(angle_pi * RADIAN_Q31_f);
	/* Modulus is m=1 */
	hcordic.Instance->WDATA = 0x7FFFFFFF;
	/* Get sin value in float */
	return ((int32_t)hcordic.Instance->RDATA)*1.0f/Q31; 
}