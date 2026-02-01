#ifndef __SERVICE_DATA_H
#define __SERVICE_DATA_H

#ifdef __cplusplus
 extern "C" {
#endif


void service_data_start(void);

void ext_send_buf_to_data(int from_id, unsigned char* buf, int len, int type);

void ext_send_notify_to_data(int from_id);

void ext_send_report_err(int from_id , unsigned short err);

void ext_send_report_status(int from_id , unsigned short status , unsigned char value);
  
#ifdef __cplusplus
}
#endif

#endif