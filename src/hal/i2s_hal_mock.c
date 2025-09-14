#include "hal/i2s_hal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file i2s_hal_mock.c
 * @brief Mock implementation of I2S HAL for demonstration and testing
 * 
 * This is a software simulation of I2S hardware for development and testing
 * purposes. In a real embedded system, this would interface with actual
 * I2S hardware registers and DMA controllers.
 */

/**
 * @brief I2S instance data
 */
typedef struct {
    bool initialized;
    bool running;
    i2s_config_t config;
    i2s_event_callback_t callback;
    void *user_data;
    uint32_t status_flags;
    uint8_t *tx_buffer;
    uint8_t *rx_buffer;
    size_t tx_buffer_size;
    size_t rx_buffer_size;
    size_t tx_pos;
    size_t rx_pos;
} i2s_instance_data_t;

/* Global I2S instance data */
static i2s_instance_data_t i2s_instances[I2S_INSTANCE_MAX];

/**
 * @brief Simulate I2S interrupt (would be called by actual hardware interrupt)
 */
static void simulate_i2s_interrupt(i2s_instance_t instance) {
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized || !inst->running || !inst->callback) {
        return;
    }
    
    // Simulate periodic interrupts for audio processing
    static uint32_t interrupt_counter = 0;
    interrupt_counter++;
    
    // Simulate TX complete every 100 calls (simulating buffer completion)
    if ((interrupt_counter % 100) == 0) {
        if (inst->config.mode == I2S_MODE_MASTER_TX || 
            inst->config.mode == I2S_MODE_MASTER_RXTX) {
            inst->callback(instance, I2S_EVENT_TX_COMPLETE, inst->user_data);
        }
    }
    
    // Simulate RX complete every 100 calls
    if ((interrupt_counter % 100) == 50) {
        if (inst->config.mode == I2S_MODE_MASTER_RX || 
            inst->config.mode == I2S_MODE_MASTER_RXTX) {
            inst->callback(instance, I2S_EVENT_RX_COMPLETE, inst->user_data);
        }
    }
}

bool i2s_hal_init(i2s_instance_t instance, const i2s_config_t *config) {
    if (instance >= I2S_INSTANCE_MAX || !config) {
        return false;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    // Initialize instance data
    memset(inst, 0, sizeof(i2s_instance_data_t));
    memcpy(&inst->config, config, sizeof(i2s_config_t));
    inst->initialized = true;
    inst->running = false;
    
    // Allocate mock buffers for DMA simulation
    if (config->dma.enable) {
        inst->tx_buffer_size = config->dma.buffer_size;
        inst->rx_buffer_size = config->dma.buffer_size;
        
        inst->tx_buffer = malloc(inst->tx_buffer_size);
        inst->rx_buffer = malloc(inst->rx_buffer_size);
        
        if (!inst->tx_buffer || !inst->rx_buffer) {
            free(inst->tx_buffer);
            free(inst->rx_buffer);
            inst->initialized = false;
            return false;
        }
        
        memset(inst->tx_buffer, 0, inst->tx_buffer_size);
        memset(inst->rx_buffer, 0, inst->rx_buffer_size);
    }
    
    printf("[I2S HAL] Instance %d initialized (Sample rate: %lu Hz, Channels: %s, Format: %d)\n",
           instance, 
           config->sample_rate,
           config->channels == I2S_CHANNEL_MONO ? "Mono" : "Stereo",
           config->format);
    
    return true;
}

void i2s_hal_deinit(i2s_instance_t instance) {
    if (instance >= I2S_INSTANCE_MAX) {
        return;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized) {
        return;
    }
    
    // Stop if running
    i2s_hal_stop(instance);
    
    // Free allocated buffers
    free(inst->tx_buffer);
    free(inst->rx_buffer);
    
    // Clear instance data
    memset(inst, 0, sizeof(i2s_instance_data_t));
    
    printf("[I2S HAL] Instance %d deinitialized\n", instance);
}

bool i2s_hal_start(i2s_instance_t instance) {
    if (instance >= I2S_INSTANCE_MAX) {
        return false;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized) {
        return false;
    }
    
    inst->running = true;
    inst->tx_pos = 0;
    inst->rx_pos = 0;
    
    printf("[I2S HAL] Instance %d started\n", instance);
    
    // Simulate initial interrupt to start audio processing
    simulate_i2s_interrupt(instance);
    
    return true;
}

void i2s_hal_stop(i2s_instance_t instance) {
    if (instance >= I2S_INSTANCE_MAX) {
        return;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized) {
        return;
    }
    
    inst->running = false;
    
    printf("[I2S HAL] Instance %d stopped\n", instance);
}

void i2s_hal_set_callback(i2s_instance_t instance, i2s_event_callback_t callback, void *user_data) {
    if (instance >= I2S_INSTANCE_MAX) {
        return;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized) {
        return;
    }
    
    inst->callback = callback;
    inst->user_data = user_data;
}

size_t i2s_hal_write(i2s_instance_t instance, const void *data, size_t size, uint32_t timeout_ms) {
    if (instance >= I2S_INSTANCE_MAX || !data || size == 0) {
        return 0;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized || !inst->running) {
        return 0;
    }
    
    // Simulate writing to hardware FIFO
    printf("[I2S HAL] Write %zu bytes to instance %d (timeout: %lu ms)\n", 
           size, instance, timeout_ms);
    
    // Simulate interrupt after write
    simulate_i2s_interrupt(instance);
    
    return size;
}

size_t i2s_hal_read(i2s_instance_t instance, void *data, size_t size, uint32_t timeout_ms) {
    if (instance >= I2S_INSTANCE_MAX || !data || size == 0) {
        return 0;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized || !inst->running) {
        return 0;
    }
    
    // Simulate reading from hardware FIFO
    memset(data, 0, size); // Simulate silence
    
    printf("[I2S HAL] Read %zu bytes from instance %d (timeout: %lu ms)\n", 
           size, instance, timeout_ms);
    
    // Simulate interrupt after read
    simulate_i2s_interrupt(instance);
    
    return size;
}

bool i2s_hal_write_dma(i2s_instance_t instance, const void *data, size_t size) {
    if (instance >= I2S_INSTANCE_MAX || !data || size == 0) {
        return false;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized || !inst->running || !inst->config.dma.enable) {
        return false;
    }
    
    // Simulate DMA transfer
    if (size > inst->tx_buffer_size) {
        size = inst->tx_buffer_size;
    }
    
    memcpy(inst->tx_buffer, data, size);
    inst->tx_pos = size;
    
    printf("[I2S HAL] DMA write %zu bytes to instance %d\n", size, instance);
    
    // Simulate DMA completion interrupt
    simulate_i2s_interrupt(instance);
    
    return true;
}

bool i2s_hal_read_dma(i2s_instance_t instance, void *data, size_t size) {
    if (instance >= I2S_INSTANCE_MAX || !data || size == 0) {
        return false;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized || !inst->running || !inst->config.dma.enable) {
        return false;
    }
    
    // Simulate DMA transfer
    if (size > inst->rx_buffer_size) {
        size = inst->rx_buffer_size;
    }
    
    // Simulate received data (silence for now)
    memset(inst->rx_buffer, 0, size);
    memcpy(data, inst->rx_buffer, size);
    inst->rx_pos = size;
    
    printf("[I2S HAL] DMA read %zu bytes from instance %d\n", size, instance);
    
    // Simulate DMA completion interrupt
    simulate_i2s_interrupt(instance);
    
    return true;
}

bool i2s_hal_is_busy(i2s_instance_t instance) {
    if (instance >= I2S_INSTANCE_MAX) {
        return false;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    return inst->initialized && inst->running;
}

uint32_t i2s_hal_get_status(i2s_instance_t instance) {
    if (instance >= I2S_INSTANCE_MAX) {
        return 0;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    return inst->status_flags;
}

void i2s_hal_clear_status(i2s_instance_t instance, uint32_t flags) {
    if (instance >= I2S_INSTANCE_MAX) {
        return;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    inst->status_flags &= ~flags;
}

bool i2s_hal_set_clock_divider(i2s_instance_t instance, uint32_t divider) {
    if (instance >= I2S_INSTANCE_MAX) {
        return false;
    }
    
    i2s_instance_data_t *inst = &i2s_instances[instance];
    
    if (!inst->initialized) {
        return false;
    }
    
    printf("[I2S HAL] Set clock divider %lu for instance %d\n", divider, instance);
    
    return true;
}

size_t i2s_hal_get_tx_fifo_space(i2s_instance_t instance) {
    if (instance >= I2S_INSTANCE_MAX) {
        return 0;
    }
    
    // Simulate FIFO with 64 word capacity
    return 64;
}

size_t i2s_hal_get_rx_fifo_count(i2s_instance_t instance) {
    if (instance >= I2S_INSTANCE_MAX) {
        return 0;
    }
    
    // Simulate FIFO with some data available
    return 32;
}