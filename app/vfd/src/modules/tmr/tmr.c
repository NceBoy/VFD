#include <string.h>
#include "tmr.h"

int tmr_init(tmr_t* tmr ,char* name , tmr_callback cb_tmr, unsigned int para, unsigned int ticks)
{
    tmr->num_ticks = ticks;
    tmr->cb_tmr = cb_tmr;
    tmr->para = para;
    strcpy(tmr->name , name );
    return 0;
}

int tmr_create(tmr_t* tmr)
{
    return tx_timer_create(&tmr->tx_tmr,tmr->name,tmr->cb_tmr,tmr->para,tmr->num_ticks,tmr->num_ticks,TX_NO_ACTIVATE); 
}


int tmr_start(tmr_t* tmr)
{
    return tx_timer_activate(&tmr->tx_tmr);
}


int tmr_stop(tmr_t* tmr)
{
    return tx_timer_deactivate(&tmr->tx_tmr);
}

int tmr_delete(tmr_t* tmr)
{
    return tx_timer_delete(&tmr->tx_tmr);
}