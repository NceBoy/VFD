#include <stdint.h>
#include <assert.h>
#include "tx_api.h"
#include "tx_byte_pool.h"

#define MALLOC_MEM_POOL_BYTES          1 * 1024

static uint32_t malloc_mem[MALLOC_MEM_POOL_BYTES / sizeof(uint32_t)] = {0};
static TX_BYTE_POOL g_malloc_mem_pool = {0};

void nx_malloc_init() 
{
    uint32_t ret = 0;
    ret = tx_byte_pool_create(&g_malloc_mem_pool, "malloc_mem", (VOID*)malloc_mem, MALLOC_MEM_POOL_BYTES);
    assert(ret == TX_SUCCESS);
}

void* nx_malloc(uint32_t len)
{
    uint8_t ret = 0;
    uint8_t* ptr;
    ret = tx_byte_allocate(&g_malloc_mem_pool, (VOID **)&ptr, len, TX_NO_WAIT);
    if (ret != TX_SUCCESS)
        return NULL;
    return ptr;
}

void nx_free(void* ptr)
{
    uint8_t ret;
    if(!ptr)
        return;
    ret = tx_byte_release(ptr);
    if (ret != TX_SUCCESS) 
    {
        // 内存释放失败，可能需要进一步处理
        // printf("Memory release failed with status: %d\n", ret);
    }
}