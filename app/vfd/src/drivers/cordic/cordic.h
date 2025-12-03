#ifndef __CORDIC_H
#define __CORDIC_H

#ifdef __cplusplus
extern "C" {
#endif

float public_rad_convert(float rad);
    
void cordic_init(void);
    
float cordic_sin(float angle);

float cordic_cos(float angle);

void cordic_sqrt_atan2(float x, float y, float *mag, float *phase);

void cordic_sin_cos_rad(float angle_rad, float *sin_out, float *cos_out);
    
#ifdef __cplusplus
}
#endif

#endif