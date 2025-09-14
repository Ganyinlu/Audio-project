#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "async_io/async_io.h"

/**
 * @file audio_driver.h
 * @brief Audio driver interface for embedded systems
 * 
 * This module provides a unified interface for various audio peripherals
 * including I2S, SPI, and other audio interfaces commonly found in embedded systems.
 */

/* Forward declarations */
typedef struct audio_driver audio_driver_t;

/**
 * @brief Audio format configuration
 */
typedef enum {
    AUDIO_FORMAT_PCM_8BIT,      /**< 8-bit PCM */
    AUDIO_FORMAT_PCM_16BIT,     /**< 16-bit PCM */
    AUDIO_FORMAT_PCM_24BIT,     /**< 24-bit PCM */
    AUDIO_FORMAT_PCM_32BIT,     /**< 32-bit PCM */
    AUDIO_FORMAT_I2S_STANDARD,  /**< I2S standard format */
    AUDIO_FORMAT_I2S_LEFT_JUSTIFIED, /**< Left-justified I2S */
    AUDIO_FORMAT_I2S_RIGHT_JUSTIFIED /**< Right-justified I2S */
} audio_format_t;

/**
 * @brief Audio driver capabilities
 */
typedef struct {
    bool supports_input;        /**< Supports audio input */
    bool supports_output;       /**< Supports audio output */
    bool supports_duplex;       /**< Supports full-duplex */
    bool supports_dma;          /**< Supports DMA transfers */
    uint32_t max_sample_rate;   /**< Maximum sample rate in Hz */
    uint32_t min_sample_rate;   /**< Minimum sample rate in Hz */
    uint8_t max_channels;       /**< Maximum number of channels */
    uint32_t buffer_alignment;  /**< Required buffer alignment */
} audio_driver_caps_t;

/**
 * @brief Audio configuration parameters
 */
typedef struct {
    audio_format_t format;      /**< Audio format */
    uint32_t sample_rate;       /**< Sample rate in Hz */
    uint8_t channels;           /**< Number of channels */
    uint16_t frame_size;        /**< Size of one audio frame in bytes */
    uint32_t buffer_size;       /**< Buffer size in frames */
    bool enable_input;          /**< Enable audio input */
    bool enable_output;         /**< Enable audio output */
    bool use_dma;               /**< Use DMA for transfers */
    uint8_t dma_priority;       /**< DMA priority level */
} audio_config_t;

/**
 * @brief Audio driver status
 */
typedef enum {
    AUDIO_DRIVER_STOPPED,       /**< Driver is stopped */
    AUDIO_DRIVER_RUNNING,       /**< Driver is running */
    AUDIO_DRIVER_ERROR,         /**< Driver encountered an error */
    AUDIO_DRIVER_OVERFLOW,      /**< Input buffer overflow */
    AUDIO_DRIVER_UNDERFLOW      /**< Output buffer underflow */
} audio_driver_status_t;

/**
 * @brief Audio driver statistics
 */
typedef struct {
    uint64_t frames_processed;  /**< Total frames processed */
    uint32_t input_overruns;    /**< Number of input overruns */
    uint32_t output_underruns;  /**< Number of output underruns */
    uint32_t dma_errors;        /**< Number of DMA errors */
    uint32_t interrupt_count;   /**< Total interrupt count */
    uint32_t avg_latency_us;    /**< Average latency in microseconds */
} audio_driver_stats_t;

/**
 * @brief Audio driver callback function types
 */
typedef void (*audio_input_callback_t)(const void *input_buffer, size_t frames, void *user_data);
typedef void (*audio_output_callback_t)(void *output_buffer, size_t frames, void *user_data);
typedef void (*audio_error_callback_t)(audio_driver_status_t status, void *user_data);

/**
 * @brief Audio driver operations structure
 */
typedef struct {
    /**
     * @brief Initialize the audio driver
     * @param driver Driver instance
     * @param config Audio configuration
     * @return true if successful, false otherwise
     */
    bool (*init)(audio_driver_t *driver, const audio_config_t *config);
    
    /**
     * @brief Start audio processing
     * @param driver Driver instance
     * @return true if successful, false otherwise
     */
    bool (*start)(audio_driver_t *driver);
    
    /**
     * @brief Stop audio processing
     * @param driver Driver instance
     */
    void (*stop)(audio_driver_t *driver);
    
    /**
     * @brief Deinitialize the driver
     * @param driver Driver instance
     */
    void (*deinit)(audio_driver_t *driver);
    
    /**
     * @brief Get driver capabilities
     * @param driver Driver instance
     * @return Pointer to capabilities structure
     */
    const audio_driver_caps_t *(*get_capabilities)(audio_driver_t *driver);
    
    /**
     * @brief Get current driver status
     * @param driver Driver instance
     * @return Current status
     */
    audio_driver_status_t (*get_status)(audio_driver_t *driver);
    
    /**
     * @brief Get driver statistics
     * @param driver Driver instance
     * @return Pointer to statistics structure
     */
    const audio_driver_stats_t *(*get_stats)(audio_driver_t *driver);
    
    /**
     * @brief Set volume level
     * @param driver Driver instance
     * @param volume Volume level (0-100)
     * @return true if successful, false otherwise
     */
    bool (*set_volume)(audio_driver_t *driver, uint8_t volume);
    
    /**
     * @brief Mute/unmute audio
     * @param driver Driver instance
     * @param mute true to mute, false to unmute
     * @return true if successful, false otherwise
     */
    bool (*set_mute)(audio_driver_t *driver, bool mute);
} audio_driver_ops_t;

/**
 * @brief Audio driver instance structure
 */
struct audio_driver {
    const audio_driver_ops_t *ops;      /**< Driver operations */
    void *private_data;                 /**< Driver private data */
    audio_config_t config;              /**< Current configuration */
    audio_driver_status_t status;       /**< Current status */
    audio_driver_stats_t stats;         /**< Driver statistics */
    
    /* Callbacks */
    audio_input_callback_t input_callback;
    audio_output_callback_t output_callback;
    audio_error_callback_t error_callback;
    void *callback_user_data;
    
    /* Async I/O integration */
    async_io_context_t *async_io_ctx;
};

/**
 * @brief Initialize an audio driver
 * @param driver Driver instance to initialize
 * @param ops Driver operations structure
 * @param config Audio configuration
 * @return true if successful, false otherwise
 */
bool audio_driver_init(audio_driver_t *driver, const audio_driver_ops_t *ops, 
                      const audio_config_t *config);

/**
 * @brief Set audio callbacks
 * @param driver Driver instance
 * @param input_cb Input callback (can be NULL)
 * @param output_cb Output callback (can be NULL)
 * @param error_cb Error callback (can be NULL)
 * @param user_data User data for callbacks
 */
void audio_driver_set_callbacks(audio_driver_t *driver,
                               audio_input_callback_t input_cb,
                               audio_output_callback_t output_cb,
                               audio_error_callback_t error_cb,
                               void *user_data);

/**
 * @brief Set asynchronous I/O context
 * @param driver Driver instance
 * @param async_io_ctx Async I/O context
 */
void audio_driver_set_async_io(audio_driver_t *driver, async_io_context_t *async_io_ctx);

/**
 * @brief Start audio processing
 * @param driver Driver instance
 * @return true if successful, false otherwise
 */
bool audio_driver_start(audio_driver_t *driver);

/**
 * @brief Stop audio processing
 * @param driver Driver instance
 */
void audio_driver_stop(audio_driver_t *driver);

/**
 * @brief Deinitialize audio driver
 * @param driver Driver instance
 */
void audio_driver_deinit(audio_driver_t *driver);

#endif /* AUDIO_DRIVER_H */