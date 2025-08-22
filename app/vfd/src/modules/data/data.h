#ifndef __DATA_H
#define __DATA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "protocol.h"

int data_process(Packet* in , Packet* out);

int data_poll(void);


#ifdef __cplusplus
}
#endif
#endif