#ifndef ASYNC_IO_H
#define ASYNC_IO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file async_io.h
 * @brief Asynchronous I/O framework for embedded audio processing
 * 
 * This module provides a non-blocking, interrupt-driven I/O framework
 * specifically designed for real-time audio processing in embedded systems.
 */

/* Forward declarations */
typedef struct async_io_context async_io_context_t;
typedef struct async_io_operation async_io_operation_t;

/**
 * @brief I/O operation types
 */
typedef enum {
    ASYNC_IO_READ,      /**< Read operation */
    ASYNC_IO_WRITE,     /**< Write operation */
    ASYNC_IO_DUPLEX     /**< Full-duplex operation */
} async_io_type_t;

/**
 * @brief I/O operation status
 */
typedef enum {
    ASYNC_IO_PENDING,   /**< Operation is pending */
    ASYNC_IO_COMPLETE,  /**< Operation completed successfully */
    ASYNC_IO_ERROR,     /**< Operation failed */
    ASYNC_IO_TIMEOUT    /**< Operation timed out */
} async_io_status_t;

/**
 * @brief Callback function type for I/O completion
 * @param operation Pointer to the completed operation
 * @param user_data User-provided data
 */
typedef void (*async_io_callback_t)(async_io_operation_t *operation, void *user_data);

/**
 * @brief Asynchronous I/O operation structure
 */
struct async_io_operation {
    async_io_type_t type;           /**< Operation type */
    async_io_status_t status;       /**< Current status */
    uint8_t *buffer;                /**< Data buffer */
    size_t buffer_size;             /**< Buffer size in bytes */
    size_t bytes_transferred;       /**< Bytes actually transferred */
    uint32_t timeout_ms;            /**< Timeout in milliseconds */
    async_io_callback_t callback;   /**< Completion callback */
    void *user_data;                /**< User data for callback */
    uint32_t priority;              /**< Operation priority (0 = highest) */
    struct async_io_operation *next; /**< Next operation in queue */
};

/**
 * @brief Asynchronous I/O context configuration
 */
typedef struct {
    uint32_t max_operations;        /**< Maximum concurrent operations */
    uint32_t buffer_size;           /**< Default buffer size */
    uint32_t interrupt_priority;    /**< Interrupt priority level */
    bool enable_dma;                /**< Enable DMA transfers */
} async_io_config_t;

/**
 * @brief Initialize the asynchronous I/O framework
 * @param config Configuration parameters
 * @return Pointer to initialized context, NULL on failure
 */
async_io_context_t *async_io_init(const async_io_config_t *config);

/**
 * @brief Deinitialize the asynchronous I/O framework
 * @param context I/O context to deinitialize
 */
void async_io_deinit(async_io_context_t *context);

/**
 * @brief Submit an asynchronous I/O operation
 * @param context I/O context
 * @param operation Operation to submit
 * @return true if operation was queued successfully, false otherwise
 */
bool async_io_submit(async_io_context_t *context, async_io_operation_t *operation);

/**
 * @brief Cancel a pending I/O operation
 * @param context I/O context
 * @param operation Operation to cancel
 * @return true if operation was cancelled, false if not found or already completed
 */
bool async_io_cancel(async_io_context_t *context, async_io_operation_t *operation);

/**
 * @brief Process pending I/O operations (call from main loop)
 * @param context I/O context
 * @return Number of operations processed
 */
uint32_t async_io_process(async_io_context_t *context);

/**
 * @brief Get the number of pending operations
 * @param context I/O context
 * @return Number of pending operations
 */
uint32_t async_io_pending_count(async_io_context_t *context);

/**
 * @brief Interrupt service routine for I/O completion
 * @param context I/O context
 * @param operation_id ID of completed operation
 * @param bytes_transferred Number of bytes transferred
 * @param error_code Error code (0 for success)
 */
void async_io_isr(async_io_context_t *context, uint32_t operation_id, 
                  size_t bytes_transferred, uint32_t error_code);

#endif /* ASYNC_IO_H */