#ifndef __NX_MALLOC_H__
#define __NX_MALLOC_H__

#include <stdint.h>

void nx_malloc_init();

void* nx_malloc(uint32_t len);

void nx_free(void* ptr);

#endif