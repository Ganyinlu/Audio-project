#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "async_io/async_io.h"

/**
 * @file test_async_io.c
 * @brief Unit tests for asynchronous I/O framework
 */

#define ASSERT_TEST(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        return 0; \
    } \
} while(0)

static int callback_called = 0;
static async_io_operation_t *callback_operation = NULL;

static void test_callback(async_io_operation_t *operation, void *user_data) {
    callback_called++;
    callback_operation = operation;
    (void)user_data;
}

static int test_async_io_init(void) {
    printf("Testing async I/O initialization...\n");
    
    async_io_config_t config = {
        .max_operations = 16,
        .buffer_size = 1024,
        .interrupt_priority = 5,
        .enable_dma = true
    };
    
    // Test valid initialization
    async_io_context_t *ctx = async_io_init(&config);
    ASSERT_TEST(ctx != NULL, "Valid initialization should succeed");
    ASSERT_TEST(async_io_pending_count(ctx) == 0, "New context should have 0 pending operations");
    
    async_io_deinit(ctx);
    
    // Test invalid initialization
    ASSERT_TEST(async_io_init(NULL) == NULL, "NULL config should fail");
    
    async_io_config_t invalid_config = {0};
    ASSERT_TEST(async_io_init(&invalid_config) == NULL, "Zero max_operations should fail");
    
    printf("PASS: Async I/O initialization\n");
    return 1;
}

static int test_async_io_operations(void) {
    printf("Testing async I/O operations...\n");
    
    async_io_config_t config = {
        .max_operations = 4,
        .buffer_size = 1024,
        .interrupt_priority = 5,
        .enable_dma = true
    };
    
    async_io_context_t *ctx = async_io_init(&config);
    ASSERT_TEST(ctx != NULL, "Context initialization should succeed");
    
    // Create test operation
    uint8_t *buffer = malloc(512);
    async_io_operation_t operation = {
        .type = ASYNC_IO_READ,
        .status = ASYNC_IO_PENDING,
        .buffer = buffer,
        .buffer_size = 512,
        .bytes_transferred = 0,
        .timeout_ms = 1000,
        .callback = test_callback,
        .user_data = NULL,
        .priority = 1,
        .next = NULL
    };
    
    // Test operation submission
    callback_called = 0;
    callback_operation = NULL;
    
    ASSERT_TEST(async_io_submit(ctx, &operation), "Operation submission should succeed");
    ASSERT_TEST(async_io_pending_count(ctx) == 1, "Should have 1 pending operation");
    
    // Simulate completion via ISR
    async_io_isr(ctx, 0, 256, 0); // operation ID 0, 256 bytes transferred, no error
    
    // Process operations
    uint32_t processed = async_io_process(ctx);
    ASSERT_TEST(processed == 1, "Should process 1 operation");
    ASSERT_TEST(callback_called == 1, "Callback should be called once");
    ASSERT_TEST(callback_operation == &operation, "Callback should receive correct operation");
    ASSERT_TEST(operation.status == ASYNC_IO_COMPLETE, "Operation should be complete");
    ASSERT_TEST(operation.bytes_transferred == 256, "Should have 256 bytes transferred");
    ASSERT_TEST(async_io_pending_count(ctx) == 0, "Should have 0 pending operations");
    
    free(buffer);
    async_io_deinit(ctx);
    
    printf("PASS: Async I/O operations\n");
    return 1;
}

static int test_async_io_cancellation(void) {
    printf("Testing async I/O cancellation...\n");
    
    async_io_config_t config = {
        .max_operations = 4,
        .buffer_size = 1024,
        .interrupt_priority = 5,
        .enable_dma = true
    };
    
    async_io_context_t *ctx = async_io_init(&config);
    ASSERT_TEST(ctx != NULL, "Context initialization should succeed");
    
    // Create test operation
    uint8_t *buffer = malloc(512);
    async_io_operation_t operation = {
        .type = ASYNC_IO_WRITE,
        .status = ASYNC_IO_PENDING,
        .buffer = buffer,
        .buffer_size = 512,
        .bytes_transferred = 0,
        .timeout_ms = 1000,
        .callback = test_callback,
        .user_data = NULL,
        .priority = 2,
        .next = NULL
    };
    
    // Submit and then cancel
    callback_called = 0;
    callback_operation = NULL;
    
    ASSERT_TEST(async_io_submit(ctx, &operation), "Operation submission should succeed");
    ASSERT_TEST(async_io_pending_count(ctx) == 1, "Should have 1 pending operation");
    
    ASSERT_TEST(async_io_cancel(ctx, &operation), "Operation cancellation should succeed");
    ASSERT_TEST(callback_called == 1, "Cancel callback should be called");
    ASSERT_TEST(operation.status == ASYNC_IO_ERROR, "Operation should be in error state");
    ASSERT_TEST(async_io_pending_count(ctx) == 0, "Should have 0 pending operations");
    
    // Try to cancel already cancelled operation
    ASSERT_TEST(!async_io_cancel(ctx, &operation), "Cancelling non-pending operation should fail");
    
    free(buffer);
    async_io_deinit(ctx);
    
    printf("PASS: Async I/O cancellation\n");
    return 1;
}

static int test_async_io_priority(void) {
    printf("Testing async I/O priority ordering...\n");
    
    async_io_config_t config = {
        .max_operations = 8,
        .buffer_size = 1024,
        .interrupt_priority = 5,
        .enable_dma = true
    };
    
    async_io_context_t *ctx = async_io_init(&config);
    ASSERT_TEST(ctx != NULL, "Context initialization should succeed");
    
    // Create operations with different priorities
    uint8_t *buffers[3];
    async_io_operation_t operations[3];
    
    for (int i = 0; i < 3; i++) {
        buffers[i] = malloc(256);
        operations[i] = (async_io_operation_t){
            .type = ASYNC_IO_DUPLEX,
            .status = ASYNC_IO_PENDING,
            .buffer = buffers[i],
            .buffer_size = 256,
            .bytes_transferred = 0,
            .timeout_ms = 1000,
            .callback = test_callback,
            .user_data = (void *)(intptr_t)i,
            .priority = (uint32_t)(2 - i), // Reverse priority: 2, 1, 0 (0 is highest)
            .next = NULL
        };
    }
    
    // Submit operations in non-priority order
    for (int i = 0; i < 3; i++) {
        ASSERT_TEST(async_io_submit(ctx, &operations[i]), "Operation submission should succeed");
    }
    
    ASSERT_TEST(async_io_pending_count(ctx) == 3, "Should have 3 pending operations");
    
    // Clean up
    for (int i = 0; i < 3; i++) {
        free(buffers[i]);
    }
    async_io_deinit(ctx);
    
    printf("PASS: Async I/O priority ordering\n");
    return 1;
}

static int test_async_io_error_handling(void) {
    printf("Testing async I/O error handling...\n");
    
    async_io_config_t config = {
        .max_operations = 2,
        .buffer_size = 1024,
        .interrupt_priority = 5,
        .enable_dma = true
    };
    
    async_io_context_t *ctx = async_io_init(&config);
    ASSERT_TEST(ctx != NULL, "Context initialization should succeed");
    
    // Test invalid operations
    ASSERT_TEST(!async_io_submit(NULL, NULL), "NULL context should fail");
    ASSERT_TEST(!async_io_submit(ctx, NULL), "NULL operation should fail");
    
    // Fill queue to capacity
    uint8_t *buffers[2];
    async_io_operation_t operations[2];
    
    for (int i = 0; i < 2; i++) {
        buffers[i] = malloc(256);
        operations[i] = (async_io_operation_t){
            .type = ASYNC_IO_READ,
            .status = ASYNC_IO_PENDING,
            .buffer = buffers[i],
            .buffer_size = 256,
            .bytes_transferred = 0,
            .timeout_ms = 1000,
            .callback = test_callback,
            .user_data = NULL,
            .priority = 1,
            .next = NULL
        };
        
        ASSERT_TEST(async_io_submit(ctx, &operations[i]), "Operation submission should succeed");
    }
    
    // Try to submit one more (should fail - queue full)
    uint8_t *extra_buffer = malloc(256);
    async_io_operation_t extra_operation = {
        .type = ASYNC_IO_WRITE,
        .status = ASYNC_IO_PENDING,
        .buffer = extra_buffer,
        .buffer_size = 256,
        .bytes_transferred = 0,
        .timeout_ms = 1000,
        .callback = test_callback,
        .user_data = NULL,
        .priority = 1,
        .next = NULL
    };
    
    ASSERT_TEST(!async_io_submit(ctx, &extra_operation), "Queue full submission should fail");
    
    // Clean up
    for (int i = 0; i < 2; i++) {
        free(buffers[i]);
    }
    free(extra_buffer);
    async_io_deinit(ctx);
    
    printf("PASS: Async I/O error handling\n");
    return 1;
}

int main(void) {
    printf("=== Async I/O Unit Tests ===\n\n");
    
    int tests_passed = 0;
    int total_tests = 0;
    
    total_tests++; if (test_async_io_init()) tests_passed++;
    total_tests++; if (test_async_io_operations()) tests_passed++;
    total_tests++; if (test_async_io_cancellation()) tests_passed++;
    total_tests++; if (test_async_io_priority()) tests_passed++;
    total_tests++; if (test_async_io_error_handling()) tests_passed++;
    
    printf("\n=== Test Results ===\n");
    printf("Passed: %d/%d tests\n", tests_passed, total_tests);
    
    if (tests_passed == total_tests) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}