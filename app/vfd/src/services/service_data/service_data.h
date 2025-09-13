#ifndef __SERVICE_DATA_H
#define __SERVICE_DATA_H

#ifdef __cplusplus
 extern "C" {
#endif


void service_data_start(void);

void ext_send_buf_to_data(int from_id, unsigned char* buf, int len);

void ext_send_notify_to_data(int from_id);

void ext_send_report_to_data(int from_id , 
    char d1, char d2, char d3, char d4, char d5);
  
#ifdef __cplusplus
}
#endif

#endif