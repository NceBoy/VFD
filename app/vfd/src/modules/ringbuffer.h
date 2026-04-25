#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 环形缓冲区结构体
 */
typedef struct {
    uint8_t *buffer;     ///< 数据缓冲区指针
    size_t size;         ///< 缓冲区总大小
    size_t head;         ///< 头指针，指向下一个写入位置
    size_t tail;         ///< 尾指针，指向下一个读取位置
    size_t count;        ///< 当前缓冲区中数据的字节数
} ringbuffer_t;

/**
 * @brief 初始化环形缓冲区
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @param buffer 用于存储数据的外部缓冲区
 * @param size 缓冲区大小
 * @return 0 成功，-1 失败（参数错误）
 */
int ringbuffer_init(ringbuffer_t *rb, uint8_t *buffer, size_t size);

/**
 * @brief 检查环形缓冲区是否为空
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @return 1 为空，0 非空
 */
int ringbuffer_is_empty(ringbuffer_t *rb);

/**
 * @brief 检查环形缓冲区是否已满
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @return 1 已满，0 未满
 */
int ringbuffer_is_full(ringbuffer_t *rb);

/**
 * @brief 获取环形缓冲区中当前数据的数量
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @return 当前数据字节数
 */
size_t ringbuffer_count(ringbuffer_t *rb);

/**
 * @brief 向环形缓冲区写入一个字节
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @param data 要写入的数据
 * @return 0 成功，-1 失败（缓冲区已满）
 */
int ringbuffer_put(ringbuffer_t *rb, uint8_t data);

/**
 * @brief 从环形缓冲区读取一个字节
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @param data 指向存储读取数据的指针
 * @return 0 成功，-1 失败（缓冲区为空或参数错误）
 */
int ringbuffer_get(ringbuffer_t *rb, uint8_t *data);

/**
 * @brief 向环形缓冲区批量写入数据
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @param data 要写入的数据指针
 * @param len 要写入的数据长度
 * @return 0 成功，-1 失败（参数错误或空间不足）
 */
int ringbuffer_write(ringbuffer_t *rb, const uint8_t *data, size_t len);

/**
 * @brief 从环形缓冲区批量读取数据
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @param data 指向存储读取数据的指针
 * @param len 要读取的数据长度
 * @return 0 成功，-1 失败（参数错误或数据不足）
 */
int ringbuffer_read(ringbuffer_t *rb, uint8_t *data, size_t len);

/**
 * @brief 清空环形缓冲区
 * 
 * @param rb 指向环形缓冲区结构体的指针
 */
void ringbuffer_clear(ringbuffer_t *rb);

/**
 * @brief 查看环形缓冲区中指定位置的数据（不移除）
 * 
 * @param rb 指向环形缓冲区结构体的指针
 * @param data 指向存储查看数据的指针
 * @param index 要查看的数据索引（相对于tail的位置）
 * @return 0 成功，-1 失败（参数错误或索引超出范围）
 */
int ringbuffer_peek(ringbuffer_t *rb, uint8_t *data, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* RINGBUFFER_H */