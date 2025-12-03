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

// 输入：x, y ∈ [0.0f, 1.0f]
// 输出：*mag = sqrt(x^2 + y^2), *phase = atan2(y, x) （单位：弧度）
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

    // 使用 32767 作为缩放因子，确保不溢出 Q15
    float scale_factor = 32767.0f / max_val;
    int32_t x_q15 = (int32_t)(x * scale_factor);
    int32_t y_q15 = (int32_t)(y * scale_factor);

    //RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;

    CORDIC->CSR = CORDIC_FUNCTION_ARCTANGENT
                | (4U << CORDIC_CSR_PRECISION_Pos)  // 迭代次数
                | (0U << CORDIC_CSR_SCALE_Pos)
                | (1U << CORDIC_CSR_NRES_Pos)       //2个结果
                | (1U << CORDIC_CSR_NARGS_Pos)      //2个输入
                | (0U << CORDIC_CSR_RESSIZE_Pos)    //32位
                | (0U << CORDIC_CSR_ARGSIZE_Pos);   //32位

    CORDIC->WDATA = x_q15;
    CORDIC->WDATA = y_q15;

    //while ((CORDIC->CSR & CORDIC_CSR_RRDY) == 0);

    int32_t mag_q15   = (int32_t)CORDIC->RDATA;
    int32_t phase_q16 = (int32_t)CORDIC->RDATA;

    *mag = ((float)mag_q15_scaled / 32767.0f) * max_val;
    *phase = (float)phase_q16 / 65536.0f;  // 弧度，范围 [-π, π]
}


// 输入：angle in radians (float)
// 输出：*sin_out, *cos_out ∈ [-1.0, 1.0]
void cordic_sin_cos_rad(float angle_rad, float *sin_out, float *cos_out)
{
    // === 1. 快速角度归一化到 [-π, π) ===
    // 避免大角度导致 Q16 溢出（虽然 CORDIC 能处理，但精度下降）
    const float TWO_PI = 6.283185307179586f;
    const float INV_TWO_PI = 0.15915494309189535f; // 1/(2π)

    // 快速模 2π（避免调用 fmodf）
    int k = (int)(angle_rad * INV_TWO_PI);
    angle_rad -= k * TWO_PI;

    // 二次校正（处理舍入误差）
    if (angle_rad > 3.1415926535897932f)  angle_rad -= TWO_PI;
    if (angle_rad < -3.1415926535897932f) angle_rad += TWO_PI;

    // === 2. 使能 CORDIC 时钟 ===
    //RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;

    // === 3. 配置 CORDIC：SINE/COSINE 模式（旋转模式）===
    // FUNC = 0b001 → Sine & Cosine
    // 输入：角度 θ（Q16），幅度 r=1（隐含）
    // 输出：cos(θ), sin(θ) （均为 Q15）
    CORDIC->CSR = CORDIC_FUNCTION_SINE              // FUNC = 0b001
                | (4U << CORDIC_CSR_PRECISION_Pos)  // 迭代次数
                | (0U << CORDIC_CSR_SCALE_Pos)
                | (1U << CORDIC_CSR_NRES_Pos)       //2个结果
                | (1U << CORDIC_CSR_NARGS_Pos)      //2个输入
                | (0U << CORDIC_CSR_RESSIZE_Pos)    //32位
                | (0U << CORDIC_CSR_ARGSIZE_Pos);   //32位

    // === 4. 将角度转为 Q16 格式（弧度 × 2^16）===
    int32_t angle_q16 = (int32_t)(angle_rad * 65536.0f);  // 2^16 = 65536

    // === 5. 写入输入数据 ===
    CORDIC->WDATA = angle_q16;   // θ in Q16
    CORDIC->WDATA = 0;           // y = 0（极坐标模式下，r=1）

    // === 6. 等待计算完成 ===
    //while ((CORDIC->CSR & CORDIC_CSR_RRDY) == 0);

    // === 7. 读取结果（顺序：先 cos，后 sin）===
    int32_t cos_q15 = (int32_t)CORDIC->RDATA;  // cos(θ) in Q15
    int32_t sin_q15 = (int32_t)CORDIC->RDATA;  // sin(θ) in Q15

    // === 8. 转换为 float [-1.0, 1.0] ===
    // Q15 最大值 = 32767（+1.0），最小值 = -32768（-1.0）
    *cos_out = (float)cos_q15 / 32768.0f;
    *sin_out = (float)sin_q15 / 32768.0f;

    // === 9. 钳位（防止因量化溢出）===
    if (*cos_out > 1.0f) *cos_out = 1.0f;
    if (*cos_out < -1.0f) *cos_out = -1.0f;
    if (*sin_out > 1.0f) *sin_out = 1.0f;
    if (*sin_out < -1.0f) *sin_out = -1.0f;
}