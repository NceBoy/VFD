#ifndef __SERVICE_HMI_H
#define __SERVICE_HMI_H

#ifdef __cplusplus
 extern "C" {
#endif

void service_hmi_start(void);

void ext_notify_stop_code(unsigned char code);
     
 
#ifdef __cplusplus
}
#endif

#endif