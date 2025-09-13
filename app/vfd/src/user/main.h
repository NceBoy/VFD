#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif


/* Define PI */
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
/* Define Q31 */
#ifndef Q31
#define Q31 0x80000000
#endif
/* Define Q31 to float unit in RADIAN = Q31/PI */
#ifndef  RADIAN_Q31_f
#define RADIAN_Q31_f  (Q31/M_PI)
#endif

#ifndef PI_2
#define PI_2	6.283185307f
#endif

#ifndef PHASE_SHIFT_120
#define PHASE_SHIFT_120 (PI_2 / 3)
#endif

#define MAIN_CTL_PERIOD   10




#include "stm32g4xx_hal.h"


void Error_Handler(void);



#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
