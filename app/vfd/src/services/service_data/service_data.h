#ifndef __SERVICE_DATA_H
#define __SERVICE_DATA_H

#ifdef __cplusplus
 extern "C" {
#endif


void service_data_start(void);

void ext_send_buf_to_data(int from_id, unsigned char* buf, int len);

void ext_send_notify_to_data(int from_id);

void ext_send_report_to_data(int from_id , unsigned char d1, unsigned char d2, 
    unsigned char d3, unsigned char d4, unsigned char d5, unsigned char d6);
  
#ifdef __cplusplus
}
#endif

#endif