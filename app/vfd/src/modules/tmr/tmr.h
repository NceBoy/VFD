#ifndef __TMR_H
#define __TMR_H


#ifdef __cplusplus
extern "C" {
#endif

#include "tx_api.h"
#include "tx_timer.h"

typedef VOID (*tmr_callback)(ULONG);

typedef struct 
{
    TX_TIMER tx_tmr;
    char name[16];
    tmr_callback cb_tmr;
    unsigned int num_ticks;
}tmr_t;

int tmr_init(tmr_t* tmr ,char* name , tmr_callback cb_tmr,unsigned int ticks);

int tmr_create(tmr_t* tmr);

int tmr_start(tmr_t* tmr);

int tmr_stop(tmr_t* tmr);

int tmr_delete(tmr_t* tmr);


#ifdef __cplusplus
}
#endif

#endif