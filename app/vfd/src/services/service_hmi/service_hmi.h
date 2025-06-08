#ifndef __SERVICE_HMI_H
#define __SERVICE_HMI_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "nx_msg.h"

void service_hmi_start(void);

msg_addr service_hmi_get_addr(void);
     
 
#ifdef __cplusplus
}
#endif

#endif