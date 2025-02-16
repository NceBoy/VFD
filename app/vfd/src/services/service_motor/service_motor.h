#ifndef __SERVICE_MOTOR_H
#define __SERVICE_MOTOR_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "nx_msg.h"

void service_motor_start(void);

msg_addr service_motor_get_addr(void);
     
 
#ifdef __cplusplus
}
#endif

#endif