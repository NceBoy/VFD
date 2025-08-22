// ringbuffer.c
#include "ringbuffer.h"
#include <stdlib.h>
#include <string.h>

/**
 * 初始化环形缓冲区
 */
int ringbuffer_init(ringbuffer_t *rb, uint8_t *buffer, size_t size) {
    if (!rb || !buffer || size == 0) {
        return -1;
    }
    
    rb->buffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    
    return 0;
}

/**
 * 检查缓冲区是否为空
 */
int ringbuffer_is_empty(ringbuffer_t *rb) {
    return (rb->count == 0);
}

/**
 * 检查缓冲区是否已满
 */
int ringbuffer_is_full(ringbuffer_t *rb) {
    return (rb->count == rb->size);
}

/**
 * 获取缓冲区中数据的数量
 */
size_t ringbuffer_count(ringbuffer_t *rb) {
    return rb->count;
}

/**
 * 向缓冲区写入单个字节
 */
int ringbuffer_put(ringbuffer_t *rb, uint8_t data) {
    if (ringbuffer_is_full(rb)) {
        return -1; // 缓冲区已满
    }
    
    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) % rb->size;
    rb->count++;
    
    return 0;
}

/**
 * 从缓冲区读取单个字节
 */
int ringbuffer_get(ringbuffer_t *rb, uint8_t *data) {
    if (ringbuffer_is_empty(rb) || !data) {
        return -1; // 缓冲区为空或参数错误
    }
    
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    
    return 0;
}

/**
 * 批量写入数据到缓冲区
 */
int ringbuffer_write(ringbuffer_t *rb, const uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) {
        return -1;
    }
    
    // 检查是否有足够空间
    if (len > (rb->size - rb->count)) {
        return -1; // 空间不足
    }
    
    for (size_t i = 0; i < len; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % rb->size;
    }
    rb->count += len;
    
    return 0;
}

/**
 * 批量读取缓冲区数据
 */
int ringbuffer_read(ringbuffer_t *rb, uint8_t *data, size_t len) {
    if (!rb || !data || len == 0) {
        return -1;
    }
    
    // 检查是否有足够数据
    if (len > rb->count) {
        return -1; // 数据不足
    }
    
    for (size_t i = 0; i < len; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % rb->size;
    }
    rb->count -= len;
    
    return 0;
}

/**
 * 清空缓冲区
 */
void ringbuffer_clear(ringbuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

/**
 * 查看缓冲区数据但不移除（peek操作）
 */
int ringbuffer_peek(ringbuffer_t *rb, uint8_t *data, size_t index) {
    if (!rb || !data || index >= rb->count) {
        return -1;
    }
    
    size_t actual_index = (rb->tail + index) % rb->size;
    *data = rb->buffer[actual_index];
    
    return 0;
}



