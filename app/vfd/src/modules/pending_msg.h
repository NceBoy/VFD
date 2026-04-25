#ifndef __PENDING_MSG_H__
#define __PENDING_MSG_H__


void pending_msg_add(unsigned int msg_id, unsigned char* buf, int len);

void pending_msg_remove(int index);

int pending_msg_find_and_remove(unsigned int msg_id);

void pending_msg_check(void);

#endif