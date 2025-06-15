#ifndef __NX_MSG_H
#define __NX_MSG_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "tx_api.h"
#include "tx_queue.h"

typedef enum
{
    MSG_ID_TEST,
    MSG_ID_MOTOR_START,
    MSG_ID_MOTOR_VF,
    MSG_ID_MOTOR_REVERSE,
    MSG_ID_MOTOR_BREAK,

    MSG_ID_UART_DATA,

    MSG_ID_KEY_SCAN,
    MSG_ID_STOP_CODE,
}MSG_ID;

typedef TX_QUEUE  *msg_addr;

typedef struct
{
	MSG_ID 	        mtype;
	msg_addr		from;
	msg_addr        to;
	unsigned int 	len;
	void*			buf;
}MSG_MGR_T;



/*the interface can be initialised once, usually at the main entry.*/
void nx_msg_init(void); 

/*the interface is created when each thread needs to communicate.*/
void nx_msg_queue_create(TX_QUEUE *queue_ptr, CHAR *name_ptr,VOID *queue_start, ULONG queue_size);

int  nx_msg_send(msg_addr from, msg_addr to, MSG_ID msgid, void *buf, int len);

int  nx_msg_recv(msg_addr from, MSG_MGR_T **msg , int timeout);

void nx_msg_free(MSG_MGR_T *msg);

void nx_msg_clr(msg_addr addr);

#ifdef __cplusplus
}
#endif

#endif