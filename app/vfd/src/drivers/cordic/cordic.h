#ifndef __CORDIC_H
#define __CORDIC_H

#ifdef __cplusplus
extern "C" {
#endif

float public_rad_convert(float rad);
    
void cordic_init(void);
    
void cordic_sin(float angle , float* sin);
    
#ifdef __cplusplus
}
#endif

#endif