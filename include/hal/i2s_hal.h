#ifndef I2S_HAL_H
#define I2S_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file i2s_hal.h
 * @brief Hardware Abstraction Layer for I2S audio interface
 * 
 * This module provides a hardware abstraction layer for I2S (Inter-IC Sound)
 * interfaces commonly used in embedded audio systems.
 */

/**
 * @brief I2S instance identifier
 */
typedef enum {
    I2S_INSTANCE_0,     /**< I2S instance 0 */
    I2S_INSTANCE_1,     /**< I2S instance 1 */
    I2S_INSTANCE_2,     /**< I2S instance 2 */
    I2S_INSTANCE_MAX
} i2s_instance_t;

/**
 * @brief I2S mode configuration
 */
typedef enum {
    I2S_MODE_MASTER_TX,     /**< Master transmitter */
    I2S_MODE_MASTER_RX,     /**< Master receiver */
    I2S_MODE_MASTER_RXTX,   /**< Master transceiver */
    I2S_MODE_SLAVE_TX,      /**< Slave transmitter */
    I2S_MODE_SLAVE_RX,      /**< Slave receiver */
    I2S_MODE_SLAVE_RXTX     /**< Slave transceiver */
} i2s_mode_t;

/**
 * @brief I2S format configuration
 */
typedef enum {
    I2S_FORMAT_STANDARD,        /**< Standard I2S format */
    I2S_FORMAT_LEFT_JUSTIFIED,  /**< Left-justified format */
    I2S_FORMAT_RIGHT_JUSTIFIED, /**< Right-justified format */
    I2S_FORMAT_DSP_A,          /**< DSP mode A */
    I2S_FORMAT_DSP_B           /**< DSP mode B */
} i2s_format_t;

/**
 * @brief I2S data width
 */
typedef enum {
    I2S_DATA_WIDTH_16BIT,   /**< 16-bit data */
    I2S_DATA_WIDTH_24BIT,   /**< 24-bit data */
    I2S_DATA_WIDTH_32BIT    /**< 32-bit data */
} i2s_data_width_t;

/**
 * @brief I2S channel configuration
 */
typedef enum {
    I2S_CHANNEL_MONO,       /**< Mono (1 channel) */
    I2S_CHANNEL_STEREO      /**< Stereo (2 channels) */
} i2s_channel_t;

/**
 * @brief I2S clock source
 */
typedef enum {
    I2S_CLOCK_INTERNAL,     /**< Internal clock source */
    I2S_CLOCK_EXTERNAL      /**< External clock source */
} i2s_clock_source_t;

/**
 * @brief I2S pin configuration
 */
typedef struct {
    uint8_t bclk_pin;       /**< Bit clock pin */
    uint8_t ws_pin;         /**< Word select pin */
    uint8_t dout_pin;       /**< Data output pin */
    uint8_t din_pin;        /**< Data input pin */
    uint8_t mclk_pin;       /**< Master clock pin (optional) */
} i2s_pin_config_t;

/**
 * @brief I2S DMA configuration
 */
typedef struct {
    bool enable;            /**< Enable DMA */
    uint8_t tx_channel;     /**< TX DMA channel */
    uint8_t rx_channel;     /**< RX DMA channel */
    uint8_t priority;       /**< DMA priority */
    size_t buffer_size;     /**< DMA buffer size */
} i2s_dma_config_t;

/**
 * @brief I2S configuration structure
 */
typedef struct {
    i2s_mode_t mode;                /**< I2S mode */
    i2s_format_t format;            /**< I2S format */
    i2s_data_width_t data_width;    /**< Data width */
    i2s_channel_t channels;         /**< Channel configuration */
    uint32_t sample_rate;           /**< Sample rate in Hz */
    i2s_clock_source_t clock_source; /**< Clock source */
    i2s_pin_config_t pins;          /**< Pin configuration */
    i2s_dma_config_t dma;           /**< DMA configuration */
    uint8_t interrupt_priority;     /**< Interrupt priority */
} i2s_config_t;

/**
 * @brief I2S event types
 */
typedef enum {
    I2S_EVENT_TX_COMPLETE,      /**< Transmit complete */
    I2S_EVENT_RX_COMPLETE,      /**< Receive complete */
    I2S_EVENT_TX_UNDERRUN,      /**< Transmit underrun */
    I2S_EVENT_RX_OVERRUN,       /**< Receive overrun */
    I2S_EVENT_ERROR             /**< General error */
} i2s_event_t;

/**
 * @brief I2S event callback function type
 * @param instance I2S instance
 * @param event Event type
 * @param user_data User data
 */
typedef void (*i2s_event_callback_t)(i2s_instance_t instance, i2s_event_t event, void *user_data);

/**
 * @brief Initialize I2S HAL
 * @param instance I2S instance
 * @param config I2S configuration
 * @return true if successful, false otherwise
 */
bool i2s_hal_init(i2s_instance_t instance, const i2s_config_t *config);

/**
 * @brief Deinitialize I2S HAL
 * @param instance I2S instance
 */
void i2s_hal_deinit(i2s_instance_t instance);

/**
 * @brief Start I2S transmission/reception
 * @param instance I2S instance
 * @return true if successful, false otherwise
 */
bool i2s_hal_start(i2s_instance_t instance);

/**
 * @brief Stop I2S transmission/reception
 * @param instance I2S instance
 */
void i2s_hal_stop(i2s_instance_t instance);

/**
 * @brief Set I2S event callback
 * @param instance I2S instance
 * @param callback Callback function
 * @param user_data User data for callback
 */
void i2s_hal_set_callback(i2s_instance_t instance, i2s_event_callback_t callback, void *user_data);

/**
 * @brief Write data to I2S (blocking)
 * @param instance I2S instance
 * @param data Data buffer
 * @param size Data size in bytes
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes written
 */
size_t i2s_hal_write(i2s_instance_t instance, const void *data, size_t size, uint32_t timeout_ms);

/**
 * @brief Read data from I2S (blocking)
 * @param instance I2S instance
 * @param data Data buffer
 * @param size Buffer size in bytes
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes read
 */
size_t i2s_hal_read(i2s_instance_t instance, void *data, size_t size, uint32_t timeout_ms);

/**
 * @brief Write data to I2S using DMA (non-blocking)
 * @param instance I2S instance
 * @param data Data buffer
 * @param size Data size in bytes
 * @return true if DMA transfer started, false otherwise
 */
bool i2s_hal_write_dma(i2s_instance_t instance, const void *data, size_t size);

/**
 * @brief Read data from I2S using DMA (non-blocking)
 * @param instance I2S instance
 * @param data Data buffer
 * @param size Buffer size in bytes
 * @return true if DMA transfer started, false otherwise
 */
bool i2s_hal_read_dma(i2s_instance_t instance, void *data, size_t size);

/**
 * @brief Check if I2S is busy
 * @param instance I2S instance
 * @return true if busy, false otherwise
 */
bool i2s_hal_is_busy(i2s_instance_t instance);

/**
 * @brief Get I2S status flags
 * @param instance I2S instance
 * @return Status flags bitmap
 */
uint32_t i2s_hal_get_status(i2s_instance_t instance);

/**
 * @brief Clear I2S status flags
 * @param instance I2S instance
 * @param flags Flags to clear
 */
void i2s_hal_clear_status(i2s_instance_t instance, uint32_t flags);

/**
 * @brief Configure I2S clock divider
 * @param instance I2S instance
 * @param divider Clock divider value
 * @return true if successful, false otherwise
 */
bool i2s_hal_set_clock_divider(i2s_instance_t instance, uint32_t divider);

/**
 * @brief Get available space in TX FIFO
 * @param instance I2S instance
 * @return Available space in words
 */
size_t i2s_hal_get_tx_fifo_space(i2s_instance_t instance);

/**
 * @brief Get available data in RX FIFO
 * @param instance I2S instance
 * @return Available data in words
 */
size_t i2s_hal_get_rx_fifo_count(i2s_instance_t instance);

#endif /* I2S_HAL_H */