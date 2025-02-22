#include <stdint.h>
#include <assert.h>
#include "nx_msg.h"
#include "tx_block_pool.h"
#include "tx_byte_pool.h"

#define MSG_MGR_MAX_NUM             16
#define MSG_MEM_POOL_BYTES          2 * 1024

static UINT msg_mgr[MSG_MGR_MAX_NUM * 6] = {0};
static UINT msg_mem[MSG_MEM_POOL_BYTES / sizeof(UINT)] = {0};

static TX_BLOCK_POOL g_msg_mgr_blk = {0};
static TX_BYTE_POOL  g_msg_mem_pool = {0};

void nx_msg_init(void)
{
    UINT ret = 0;
    ret = tx_block_pool_create(&g_msg_mgr_blk, "msg_mgr", sizeof(MSG_MGR_T),(VOID *)msg_mgr, sizeof(msg_mgr));
    assert(ret == TX_SUCCESS);

    ret = tx_byte_pool_create(&g_msg_mem_pool, "msg_mem", (VOID *)msg_mem, MSG_MEM_POOL_BYTES);
    assert(ret == TX_SUCCESS);
}

int  nx_msg_send(msg_addr from, msg_addr to, MSG_ID msgid, void *buf, int len)
{
    UINT ret = 0;
    MSG_MGR_T *block = NULL;
    UCHAR *ptr = NULL;
    ret = tx_block_allocate(&g_msg_mgr_blk, (VOID **)&block, TX_NO_WAIT);
    if(ret != TX_SUCCESS)
        return -1;
    block->mtype = msgid;
    block->from = from;
    block->to = to;
    if((buf != NULL) && (len != 0))
    {
        ret = tx_byte_allocate(&g_msg_mem_pool, (VOID **)&ptr, len,  TX_NO_WAIT);
        if(ret != TX_SUCCESS)
        {
            tx_block_release((VOID *)block);
            return -2;
        }
        block->buf = (void*)ptr;
        block->len = len;
        memcpy(block->buf , buf , len);
    }
    ret = tx_queue_send(to, (VOID *)&block, TX_NO_WAIT);
    if(ret != TX_SUCCESS)
    {
        tx_byte_release((VOID *)ptr);
        tx_block_release((VOID *)block);
        return -3;
    }
    return 0;
}

int  nx_msg_recv(msg_addr from, MSG_MGR_T **msg , int timeout)
{
    UINT ret = 0;
    MSG_MGR_T *buf = NULL;
    ret = tx_queue_receive(from, (VOID *)&buf, timeout);
    if(ret != TX_SUCCESS)
        return -1;
    *msg = buf;  
    return 0;
}

void nx_msg_free(MSG_MGR_T *msg)
{
    assert (msg != NULL);
    if(msg->buf != NULL)
    {
        tx_byte_release((VOID *)msg->buf);
        msg->buf = NULL;        
    }
    tx_block_release((VOID *)msg);
    msg = NULL;
}

void nx_msg_clr(msg_addr addr)
{
    UINT ret = 0;
    MSG_MGR_T *buf = {0};
    while(1)
    {
        ret = tx_queue_receive(addr, (VOID *)&buf, TX_NO_WAIT);
        if(ret != TX_SUCCESS)
            break ;
        nx_msg_free(buf);
    }
    tx_queue_flush(addr);
}

void nx_msg_queue_create(TX_QUEUE *queue_ptr, CHAR *name_ptr,VOID *queue_start, ULONG queue_size)
{
    UINT ret = 0;
    ret = tx_queue_create(queue_ptr, name_ptr, TX_1_ULONG ,queue_start, queue_size);
    assert(ret == TX_SUCCESS);
}
