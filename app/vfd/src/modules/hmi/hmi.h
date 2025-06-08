#ifndef __HMI_H
#define __HMI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void hmi_init(void);
void hmi_scan_key(void);
void hmi_clear_menu(void);

#ifdef __cplusplus
}
#endif

#endif

