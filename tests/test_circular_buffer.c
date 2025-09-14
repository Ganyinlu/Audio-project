#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "utils/circular_buffer.h"

/**
 * @file test_circular_buffer.c
 * @brief Unit tests for circular buffer implementation
 */

#define TEST_BUFFER_SIZE 256
#define ASSERT_TEST(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        return 0; \
    } \
} while(0)

static int test_circular_buffer_init(void) {
    printf("Testing circular buffer initialization...\n");
    
    circular_buffer_t cb;
    uint8_t buffer[TEST_BUFFER_SIZE];
    
    // Test valid initialization
    ASSERT_TEST(circular_buffer_init(&cb, buffer, TEST_BUFFER_SIZE), 
                "Valid initialization should succeed");
    ASSERT_TEST(circular_buffer_empty(&cb), "New buffer should be empty");
    ASSERT_TEST(!circular_buffer_full(&cb), "New buffer should not be full");
    ASSERT_TEST(circular_buffer_available(&cb) == 0, "New buffer should have 0 available");
    ASSERT_TEST(circular_buffer_space(&cb) == TEST_BUFFER_SIZE - 1, 
                "New buffer should have size-1 space");
    
    // Test invalid initialization
    ASSERT_TEST(!circular_buffer_init(NULL, buffer, TEST_BUFFER_SIZE), 
                "NULL cb should fail");
    ASSERT_TEST(!circular_buffer_init(&cb, NULL, TEST_BUFFER_SIZE), 
                "NULL buffer should fail");
    ASSERT_TEST(!circular_buffer_init(&cb, buffer, 0), 
                "Zero size should fail");
    ASSERT_TEST(!circular_buffer_init(&cb, buffer, 100), 
                "Non-power-of-2 size should fail");
    
    printf("PASS: Circular buffer initialization\n");
    return 1;
}

static int test_circular_buffer_write_read(void) {
    printf("Testing circular buffer write/read...\n");
    
    circular_buffer_t cb;
    uint8_t buffer[TEST_BUFFER_SIZE];
    uint8_t write_data[64];
    uint8_t read_data[64];
    
    circular_buffer_init(&cb, buffer, TEST_BUFFER_SIZE);
    
    // Prepare test data
    for (int i = 0; i < 64; i++) {
        write_data[i] = (uint8_t)(i + 1);
    }
    
    // Test basic write/read
    size_t written = circular_buffer_write(&cb, write_data, 32);
    ASSERT_TEST(written == 32, "Should write 32 bytes");
    ASSERT_TEST(circular_buffer_available(&cb) == 32, "Should have 32 bytes available");
    ASSERT_TEST(!circular_buffer_empty(&cb), "Buffer should not be empty");
    
    size_t read = circular_buffer_read(&cb, read_data, 16);
    ASSERT_TEST(read == 16, "Should read 16 bytes");
    ASSERT_TEST(circular_buffer_available(&cb) == 16, "Should have 16 bytes available");
    ASSERT_TEST(memcmp(read_data, write_data, 16) == 0, "Read data should match written data");
    
    // Test reading remaining data
    read = circular_buffer_read(&cb, read_data + 16, 16);
    ASSERT_TEST(read == 16, "Should read remaining 16 bytes");
    ASSERT_TEST(circular_buffer_empty(&cb), "Buffer should be empty");
    ASSERT_TEST(memcmp(read_data, write_data, 32) == 0, "All read data should match");
    
    printf("PASS: Circular buffer write/read\n");
    return 1;
}

static int test_circular_buffer_wrap_around(void) {
    printf("Testing circular buffer wrap around...\n");
    
    circular_buffer_t cb;
    uint8_t buffer[TEST_BUFFER_SIZE];
    uint8_t write_data[TEST_BUFFER_SIZE];
    uint8_t read_data[TEST_BUFFER_SIZE];
    
    circular_buffer_init(&cb, buffer, TEST_BUFFER_SIZE);
    
    // Prepare test data
    for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
        write_data[i] = (uint8_t)(i & 0xFF);
    }
    
    // Fill buffer almost to capacity
    size_t written = circular_buffer_write(&cb, write_data, TEST_BUFFER_SIZE - 1);
    ASSERT_TEST(written == TEST_BUFFER_SIZE - 1, "Should write max capacity");
    ASSERT_TEST(circular_buffer_full(&cb), "Buffer should be full");
    
    // Try to write more (should fail)
    written = circular_buffer_write(&cb, write_data, 1);
    ASSERT_TEST(written == 0, "Should not write when full");
    
    // Read some data to make space
    size_t read = circular_buffer_read(&cb, read_data, 64);
    ASSERT_TEST(read == 64, "Should read 64 bytes");
    
    // Write more data (should wrap around)
    written = circular_buffer_write(&cb, write_data, 64);
    ASSERT_TEST(written == 64, "Should write 64 bytes after making space");
    
    printf("PASS: Circular buffer wrap around\n");
    return 1;
}

static int test_circular_buffer_peek(void) {
    printf("Testing circular buffer peek...\n");
    
    circular_buffer_t cb;
    uint8_t buffer[TEST_BUFFER_SIZE];
    uint8_t write_data[32];
    uint8_t peek_data[32];
    uint8_t read_data[32];
    
    circular_buffer_init(&cb, buffer, TEST_BUFFER_SIZE);
    
    // Prepare test data
    for (int i = 0; i < 32; i++) {
        write_data[i] = (uint8_t)(i + 10);
    }
    
    // Write data
    circular_buffer_write(&cb, write_data, 32);
    
    // Peek at data
    size_t peeked = circular_buffer_peek(&cb, peek_data, 16);
    ASSERT_TEST(peeked == 16, "Should peek 16 bytes");
    ASSERT_TEST(circular_buffer_available(&cb) == 32, "Available should not change after peek");
    ASSERT_TEST(memcmp(peek_data, write_data, 16) == 0, "Peeked data should match written data");
    
    // Read data and verify it matches peeked data
    size_t read = circular_buffer_read(&cb, read_data, 16);
    ASSERT_TEST(read == 16, "Should read 16 bytes");
    ASSERT_TEST(memcmp(read_data, peek_data, 16) == 0, "Read data should match peeked data");
    
    printf("PASS: Circular buffer peek\n");
    return 1;
}

static int test_circular_buffer_zero_copy(void) {
    printf("Testing circular buffer zero-copy operations...\n");
    
    circular_buffer_t cb;
    uint8_t buffer[TEST_BUFFER_SIZE];
    
    circular_buffer_init(&cb, buffer, TEST_BUFFER_SIZE);
    
    // Test write pointer
    size_t available;
    uint8_t *write_ptr = circular_buffer_write_ptr(&cb, &available);
    ASSERT_TEST(write_ptr != NULL, "Write pointer should not be NULL");
    ASSERT_TEST(available == TEST_BUFFER_SIZE - 1, "Should have max space available");
    
    // Write data directly
    for (size_t i = 0; i < 64; i++) {
        write_ptr[i] = (uint8_t)(i + 20);
    }
    circular_buffer_write_advance(&cb, 64);
    ASSERT_TEST(circular_buffer_available(&cb) == 64, "Should have 64 bytes available");
    
    // Test read pointer
    const uint8_t *read_ptr = circular_buffer_read_ptr(&cb, &available);
    ASSERT_TEST(read_ptr != NULL, "Read pointer should not be NULL");
    ASSERT_TEST(available == 64, "Should have 64 bytes available");
    
    // Verify data
    for (size_t i = 0; i < 64; i++) {
        ASSERT_TEST(read_ptr[i] == (uint8_t)(i + 20), "Data should match");
    }
    
    circular_buffer_read_advance(&cb, 64);
    ASSERT_TEST(circular_buffer_empty(&cb), "Buffer should be empty");
    
    printf("PASS: Circular buffer zero-copy operations\n");
    return 1;
}

int main(void) {
    printf("=== Circular Buffer Unit Tests ===\n\n");
    
    int tests_passed = 0;
    int total_tests = 0;
    
    total_tests++; if (test_circular_buffer_init()) tests_passed++;
    total_tests++; if (test_circular_buffer_write_read()) tests_passed++;
    total_tests++; if (test_circular_buffer_wrap_around()) tests_passed++;
    total_tests++; if (test_circular_buffer_peek()) tests_passed++;
    total_tests++; if (test_circular_buffer_zero_copy()) tests_passed++;
    
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