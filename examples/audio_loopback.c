#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "async_io/async_io.h"
#include "drivers/audio_driver.h"
#include "drivers/i2s_audio_driver.h"
#include "utils/circular_buffer.h"

/**
 * @file audio_loopback.c
 * @brief Audio loopback example demonstrating asynchronous I/O
 * 
 * This example shows how to use the asynchronous I/O framework with
 * audio drivers to create a real-time audio loopback system.
 */

#define SAMPLE_RATE     44100
#define CHANNELS        2
#define FRAME_SIZE      (CHANNELS * sizeof(int16_t))
#define BUFFER_FRAMES   1024

/* Global variables */
static audio_driver_t audio_driver;
static async_io_context_t *async_ctx;
static circular_buffer_t loopback_buffer;
static uint8_t loopback_mem[BUFFER_FRAMES * FRAME_SIZE * 4]; // 4x buffer for safety

/**
 * @brief Audio input callback - receives audio data
 */
static void audio_input_callback(const void *input_buffer, size_t frames, void *user_data) {
    (void)user_data;
    
    printf("[Loopback] Received %zu frames\n", frames);
    
    // Write input data to loopback buffer
    size_t bytes = frames * FRAME_SIZE;
    size_t written = circular_buffer_write(&loopback_buffer, (const uint8_t *)input_buffer, bytes);
    
    if (written != bytes) {
        printf("[Loopback] Warning: Buffer overrun, only wrote %zu/%zu bytes\n", written, bytes);
    }
}

/**
 * @brief Audio output callback - provides audio data
 */
static void audio_output_callback(void *output_buffer, size_t frames, void *user_data) {
    (void)user_data;
    
    printf("[Loopback] Requested %zu frames\n", frames);
    
    // Read data from loopback buffer
    size_t bytes = frames * FRAME_SIZE;
    size_t read = circular_buffer_read(&loopback_buffer, (uint8_t *)output_buffer, bytes);
    
    // Fill remaining with silence if not enough data
    if (read < bytes) {
        memset((uint8_t *)output_buffer + read, 0, bytes - read);
        printf("[Loopback] Warning: Buffer underrun, only read %zu/%zu bytes\n", read, bytes);
    }
}

/**
 * @brief Audio error callback
 */
static void audio_error_callback(audio_driver_status_t status, void *user_data) {
    (void)user_data;
    
    const char *status_str;
    switch (status) {
        case AUDIO_DRIVER_OVERFLOW:
            status_str = "OVERFLOW";
            break;
        case AUDIO_DRIVER_UNDERFLOW:
            status_str = "UNDERFLOW";
            break;
        case AUDIO_DRIVER_ERROR:
            status_str = "ERROR";
            break;
        default:
            status_str = "UNKNOWN";
            break;
    }
    
    printf("[Loopback] Audio error: %s\n", status_str);
}

/**
 * @brief Async I/O completion callback
 */
static void async_io_completion(async_io_operation_t *operation, void *user_data) {
    (void)user_data;
    
    printf("[Async I/O] Operation completed: type=%d, status=%d, bytes=%zu\n",
           operation->type, operation->status, operation->bytes_transferred);
}

int main(void) {
    printf("=== Audio Loopback Example ===\n");
    printf("This example demonstrates asynchronous I/O with audio drivers\n\n");
    
    // Initialize async I/O framework
    async_io_config_t async_config = {
        .max_operations = 16,
        .buffer_size = BUFFER_FRAMES * FRAME_SIZE,
        .interrupt_priority = 5,
        .enable_dma = true
    };
    
    async_ctx = async_io_init(&async_config);
    if (!async_ctx) {
        printf("Error: Failed to initialize async I/O framework\n");
        return 1;
    }
    
    printf("[Init] Async I/O framework initialized\n");
    
    // Initialize loopback buffer
    if (!circular_buffer_init(&loopback_buffer, loopback_mem, sizeof(loopback_mem))) {
        printf("Error: Failed to initialize loopback buffer\n");
        async_io_deinit(async_ctx);
        return 1;
    }
    
    printf("[Init] Loopback buffer initialized (%zu bytes)\n", sizeof(loopback_mem));
    
    // Configure audio driver
    audio_config_t audio_config = {
        .format = AUDIO_FORMAT_I2S_STANDARD,
        .sample_rate = SAMPLE_RATE,
        .channels = CHANNELS,
        .frame_size = FRAME_SIZE,
        .buffer_size = BUFFER_FRAMES,
        .enable_input = true,
        .enable_output = true,
        .use_dma = true,
        .dma_priority = 6
    };
    
    // Initialize audio driver
    if (!audio_driver_init(&audio_driver, &i2s_audio_driver_ops, &audio_config)) {
        printf("Error: Failed to initialize audio driver\n");
        async_io_deinit(async_ctx);
        return 1;
    }
    
    printf("[Init] Audio driver initialized (Sample rate: %lu Hz, Channels: %d)\n",
           audio_config.sample_rate, audio_config.channels);
    
    // Set callbacks
    audio_driver_set_callbacks(&audio_driver,
                              audio_input_callback,
                              audio_output_callback,
                              audio_error_callback,
                              NULL);
    
    // Set async I/O context
    audio_driver_set_async_io(&audio_driver, async_ctx);
    
    printf("[Init] Audio callbacks configured\n");
    
    // Display driver capabilities
    const audio_driver_caps_t *caps = audio_driver.ops->get_capabilities(&audio_driver);
    if (caps) {
        printf("[Info] Driver capabilities:\n");
        printf("  - Input: %s\n", caps->supports_input ? "Yes" : "No");
        printf("  - Output: %s\n", caps->supports_output ? "Yes" : "No");
        printf("  - Duplex: %s\n", caps->supports_duplex ? "Yes" : "No");
        printf("  - DMA: %s\n", caps->supports_dma ? "Yes" : "No");
        printf("  - Sample rate range: %lu - %lu Hz\n", caps->min_sample_rate, caps->max_sample_rate);
        printf("  - Max channels: %d\n", caps->max_channels);
    }
    
    // Start audio processing
    if (!audio_driver_start(&audio_driver)) {
        printf("Error: Failed to start audio driver\n");
        audio_driver_deinit(&audio_driver);
        async_io_deinit(async_ctx);
        return 1;
    }
    
    printf("[Run] Audio driver started - loopback active\n");
    printf("[Run] Press Ctrl+C to stop...\n\n");
    
    // Create some async I/O operations for demonstration
    async_io_operation_t test_operation = {
        .type = ASYNC_IO_DUPLEX,
        .status = ASYNC_IO_PENDING,
        .buffer = malloc(1024),
        .buffer_size = 1024,
        .bytes_transferred = 0,
        .timeout_ms = 5000,
        .callback = async_io_completion,
        .user_data = NULL,
        .priority = 1,
        .next = NULL
    };
    
    if (test_operation.buffer) {
        if (async_io_submit(async_ctx, &test_operation)) {
            printf("[Async I/O] Test operation submitted\n");
        }
    }
    
    // Main processing loop
    int loop_count = 0;
    while (loop_count < 100) { // Run for limited time in example
        // Process async I/O operations
        uint32_t processed = async_io_process(async_ctx);
        if (processed > 0) {
            printf("[Async I/O] Processed %lu operations\n", processed);
        }
        
        // Display statistics every 10 iterations
        if ((loop_count % 10) == 0) {
            const audio_driver_stats_t *stats = audio_driver.ops->get_stats(&audio_driver);
            if (stats) {
                printf("[Stats] Frames: %llu, Overruns: %lu, Underruns: %lu, Interrupts: %lu\n",
                       stats->frames_processed, stats->input_overruns, 
                       stats->output_underruns, stats->interrupt_count);
            }
            
            printf("[Buffer] Available: %zu bytes, Space: %zu bytes\n",
                   circular_buffer_available(&loopback_buffer),
                   circular_buffer_space(&loopback_buffer));
        }
        
        // Simulate processing delay
        usleep(100000); // 100ms
        loop_count++;
    }
    
    // Cleanup
    printf("\n[Cleanup] Stopping audio driver...\n");
    audio_driver_stop(&audio_driver);
    audio_driver_deinit(&audio_driver);
    
    free(test_operation.buffer);
    async_io_deinit(async_ctx);
    
    printf("[Cleanup] Audio loopback example completed\n");
    
    return 0;
}