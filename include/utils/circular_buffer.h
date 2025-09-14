#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file circular_buffer.h
 * @brief Lock-free circular buffer implementation for audio data
 * 
 * This module provides a thread-safe, lock-free circular buffer optimized
 * for single-producer, single-consumer scenarios common in audio processing.
 */

/**
 * @brief Circular buffer structure
 */
typedef struct {
    uint8_t *buffer;        /**< Data buffer */
    volatile size_t head;   /**< Write position */
    volatile size_t tail;   /**< Read position */
    size_t size;            /**< Buffer size (must be power of 2) */
    size_t mask;            /**< Size mask for efficient modulo operation */
} circular_buffer_t;

/**
 * @brief Initialize a circular buffer
 * @param cb Pointer to circular buffer structure
 * @param buffer Data buffer (must be aligned for DMA if used)
 * @param size Buffer size in bytes (must be power of 2)
 * @return true if initialization successful, false otherwise
 */
bool circular_buffer_init(circular_buffer_t *cb, uint8_t *buffer, size_t size);

/**
 * @brief Write data to circular buffer
 * @param cb Pointer to circular buffer
 * @param data Data to write
 * @param length Number of bytes to write
 * @return Number of bytes actually written
 */
size_t circular_buffer_write(circular_buffer_t *cb, const uint8_t *data, size_t length);

/**
 * @brief Read data from circular buffer
 * @param cb Pointer to circular buffer
 * @param data Buffer to read data into
 * @param length Number of bytes to read
 * @return Number of bytes actually read
 */
size_t circular_buffer_read(circular_buffer_t *cb, uint8_t *data, size_t length);

/**
 * @brief Peek at data in circular buffer without consuming it
 * @param cb Pointer to circular buffer
 * @param data Buffer to peek data into
 * @param length Number of bytes to peek
 * @return Number of bytes actually peeked
 */
size_t circular_buffer_peek(circular_buffer_t *cb, uint8_t *data, size_t length);

/**
 * @brief Get number of bytes available for reading
 * @param cb Pointer to circular buffer
 * @return Number of bytes available
 */
size_t circular_buffer_available(const circular_buffer_t *cb);

/**
 * @brief Get number of bytes available for writing
 * @param cb Pointer to circular buffer
 * @return Number of bytes available
 */
size_t circular_buffer_space(const circular_buffer_t *cb);

/**
 * @brief Check if buffer is empty
 * @param cb Pointer to circular buffer
 * @return true if empty, false otherwise
 */
bool circular_buffer_empty(const circular_buffer_t *cb);

/**
 * @brief Check if buffer is full
 * @param cb Pointer to circular buffer
 * @return true if full, false otherwise
 */
bool circular_buffer_full(const circular_buffer_t *cb);

/**
 * @brief Reset circular buffer to empty state
 * @param cb Pointer to circular buffer
 */
void circular_buffer_reset(circular_buffer_t *cb);

/**
 * @brief Get direct write pointer for zero-copy operations
 * @param cb Pointer to circular buffer
 * @param available Pointer to store available space
 * @return Pointer to write location, NULL if buffer is full
 */
uint8_t *circular_buffer_write_ptr(circular_buffer_t *cb, size_t *available);

/**
 * @brief Advance write pointer after direct write
 * @param cb Pointer to circular buffer
 * @param bytes Number of bytes written
 */
void circular_buffer_write_advance(circular_buffer_t *cb, size_t bytes);

/**
 * @brief Get direct read pointer for zero-copy operations
 * @param cb Pointer to circular buffer
 * @param available Pointer to store available data
 * @return Pointer to read location, NULL if buffer is empty
 */
const uint8_t *circular_buffer_read_ptr(circular_buffer_t *cb, size_t *available);

/**
 * @brief Advance read pointer after direct read
 * @param cb Pointer to circular buffer
 * @param bytes Number of bytes read
 */
void circular_buffer_read_advance(circular_buffer_t *cb, size_t bytes);

#endif /* CIRCULAR_BUFFER_H */