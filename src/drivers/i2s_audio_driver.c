#include "drivers/audio_driver.h"
#include "hal/i2s_hal.h"
#include "utils/circular_buffer.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief I2S audio driver private data
 */
typedef struct {
    i2s_instance_t instance;        /**< I2S instance */
    i2s_config_t i2s_config;       /**< I2S configuration */
    circular_buffer_t input_buffer; /**< Input circular buffer */
    circular_buffer_t output_buffer; /**< Output circular buffer */
    uint8_t *input_buf_mem;         /**< Input buffer memory */
    uint8_t *output_buf_mem;        /**< Output buffer memory */
    bool dma_enabled;               /**< DMA enabled flag */
    uint32_t frame_size;            /**< Size of one audio frame */
} i2s_audio_driver_data_t;

/* Driver capabilities */
static const audio_driver_caps_t i2s_capabilities = {
    .supports_input = true,
    .supports_output = true,
    .supports_duplex = true,
    .supports_dma = true,
    .max_sample_rate = 192000,
    .min_sample_rate = 8000,
    .max_channels = 2,
    .buffer_alignment = 4
};

/**
 * @brief I2S event callback from HAL
 */
static void i2s_event_callback(i2s_instance_t instance, i2s_event_t event, void *user_data) {
    audio_driver_t *driver = (audio_driver_t *)user_data;
    i2s_audio_driver_data_t *priv = (i2s_audio_driver_data_t *)driver->private_data;
    
    if (!driver || !priv) {
        return;
    }
    
    switch (event) {
        case I2S_EVENT_RX_COMPLETE:
            driver->stats.frames_processed++;
            if (driver->input_callback) {
                // Get data from circular buffer
                size_t available;
                const uint8_t *data = circular_buffer_read_ptr(&priv->input_buffer, &available);
                if (data && available >= priv->frame_size) {
                    size_t frames = available / priv->frame_size;
                    driver->input_callback(data, frames, driver->callback_user_data);
                    circular_buffer_read_advance(&priv->input_buffer, frames * priv->frame_size);
                }
            }
            break;
            
        case I2S_EVENT_TX_COMPLETE:
            driver->stats.frames_processed++;
            if (driver->output_callback) {
                // Get space in circular buffer
                size_t available;
                uint8_t *data = circular_buffer_write_ptr(&priv->output_buffer, &available);
                if (data && available >= priv->frame_size) {
                    size_t frames = available / priv->frame_size;
                    driver->output_callback(data, frames, driver->callback_user_data);
                    circular_buffer_write_advance(&priv->output_buffer, frames * priv->frame_size);
                }
            }
            break;
            
        case I2S_EVENT_RX_OVERRUN:
            driver->stats.input_overruns++;
            driver->status = AUDIO_DRIVER_OVERFLOW;
            if (driver->error_callback) {
                driver->error_callback(AUDIO_DRIVER_OVERFLOW, driver->callback_user_data);
            }
            break;
            
        case I2S_EVENT_TX_UNDERRUN:
            driver->stats.output_underruns++;
            driver->status = AUDIO_DRIVER_UNDERFLOW;
            if (driver->error_callback) {
                driver->error_callback(AUDIO_DRIVER_UNDERFLOW, driver->callback_user_data);
            }
            break;
            
        case I2S_EVENT_ERROR:
            driver->stats.dma_errors++;
            driver->status = AUDIO_DRIVER_ERROR;
            if (driver->error_callback) {
                driver->error_callback(AUDIO_DRIVER_ERROR, driver->callback_user_data);
            }
            break;
    }
}

/**
 * @brief Convert audio format to I2S format
 */
static i2s_format_t audio_format_to_i2s(audio_format_t format) {
    switch (format) {
        case AUDIO_FORMAT_I2S_STANDARD:
            return I2S_FORMAT_STANDARD;
        case AUDIO_FORMAT_I2S_LEFT_JUSTIFIED:
            return I2S_FORMAT_LEFT_JUSTIFIED;
        case AUDIO_FORMAT_I2S_RIGHT_JUSTIFIED:
            return I2S_FORMAT_RIGHT_JUSTIFIED;
        default:
            return I2S_FORMAT_STANDARD;
    }
}

/**
 * @brief Convert audio format to I2S data width
 */
static i2s_data_width_t audio_format_to_data_width(audio_format_t format) {
    switch (format) {
        case AUDIO_FORMAT_PCM_16BIT:
        case AUDIO_FORMAT_I2S_STANDARD:
        case AUDIO_FORMAT_I2S_LEFT_JUSTIFIED:
        case AUDIO_FORMAT_I2S_RIGHT_JUSTIFIED:
            return I2S_DATA_WIDTH_16BIT;
        case AUDIO_FORMAT_PCM_24BIT:
            return I2S_DATA_WIDTH_24BIT;
        case AUDIO_FORMAT_PCM_32BIT:
            return I2S_DATA_WIDTH_32BIT;
        default:
            return I2S_DATA_WIDTH_16BIT;
    }
}

/**
 * @brief Initialize I2S audio driver
 */
static bool i2s_audio_init(audio_driver_t *driver, const audio_config_t *config) {
    if (!driver || !config) {
        return false;
    }
    
    // Allocate private data
    i2s_audio_driver_data_t *priv = malloc(sizeof(i2s_audio_driver_data_t));
    if (!priv) {
        return false;
    }
    
    memset(priv, 0, sizeof(i2s_audio_driver_data_t));
    driver->private_data = priv;
    
    // Calculate frame size
    priv->frame_size = config->frame_size;
    
    // Allocate circular buffers
    size_t buffer_size = config->buffer_size * priv->frame_size;
    // Ensure buffer size is power of 2
    size_t aligned_size = 1;
    while (aligned_size < buffer_size) {
        aligned_size <<= 1;
    }
    
    if (config->enable_input) {
        priv->input_buf_mem = malloc(aligned_size);
        if (!priv->input_buf_mem) {
            free(priv);
            return false;
        }
        circular_buffer_init(&priv->input_buffer, priv->input_buf_mem, aligned_size);
    }
    
    if (config->enable_output) {
        priv->output_buf_mem = malloc(aligned_size);
        if (!priv->output_buf_mem) {
            free(priv->input_buf_mem);
            free(priv);
            return false;
        }
        circular_buffer_init(&priv->output_buffer, priv->output_buf_mem, aligned_size);
    }
    
    // Configure I2S
    priv->instance = I2S_INSTANCE_0; // Default to instance 0
    priv->dma_enabled = config->use_dma;
    
    priv->i2s_config.mode = config->enable_input && config->enable_output ? 
                           I2S_MODE_MASTER_RXTX : 
                           (config->enable_input ? I2S_MODE_MASTER_RX : I2S_MODE_MASTER_TX);
    priv->i2s_config.format = audio_format_to_i2s(config->format);
    priv->i2s_config.data_width = audio_format_to_data_width(config->format);
    priv->i2s_config.channels = config->channels == 1 ? I2S_CHANNEL_MONO : I2S_CHANNEL_STEREO;
    priv->i2s_config.sample_rate = config->sample_rate;
    priv->i2s_config.clock_source = I2S_CLOCK_INTERNAL;
    
    // Configure pins (platform-specific)
    priv->i2s_config.pins.bclk_pin = 26;
    priv->i2s_config.pins.ws_pin = 25;
    priv->i2s_config.pins.dout_pin = 22;
    priv->i2s_config.pins.din_pin = 23;
    priv->i2s_config.pins.mclk_pin = 0;
    
    // Configure DMA
    priv->i2s_config.dma.enable = config->use_dma;
    priv->i2s_config.dma.tx_channel = 0;
    priv->i2s_config.dma.rx_channel = 1;
    priv->i2s_config.dma.priority = config->dma_priority;
    priv->i2s_config.dma.buffer_size = aligned_size;
    
    priv->i2s_config.interrupt_priority = 5;
    
    // Initialize I2S HAL
    if (!i2s_hal_init(priv->instance, &priv->i2s_config)) {
        free(priv->input_buf_mem);
        free(priv->output_buf_mem);
        free(priv);
        return false;
    }
    
    // Set I2S callback
    i2s_hal_set_callback(priv->instance, i2s_event_callback, driver);
    
    return true;
}

/**
 * @brief Start I2S audio driver
 */
static bool i2s_audio_start(audio_driver_t *driver) {
    if (!driver || !driver->private_data) {
        return false;
    }
    
    i2s_audio_driver_data_t *priv = (i2s_audio_driver_data_t *)driver->private_data;
    
    return i2s_hal_start(priv->instance);
}

/**
 * @brief Stop I2S audio driver
 */
static void i2s_audio_stop(audio_driver_t *driver) {
    if (!driver || !driver->private_data) {
        return;
    }
    
    i2s_audio_driver_data_t *priv = (i2s_audio_driver_data_t *)driver->private_data;
    
    i2s_hal_stop(priv->instance);
}

/**
 * @brief Deinitialize I2S audio driver
 */
static void i2s_audio_deinit(audio_driver_t *driver) {
    if (!driver || !driver->private_data) {
        return;
    }
    
    i2s_audio_driver_data_t *priv = (i2s_audio_driver_data_t *)driver->private_data;
    
    i2s_hal_deinit(priv->instance);
    
    free(priv->input_buf_mem);
    free(priv->output_buf_mem);
    free(priv);
    
    driver->private_data = NULL;
}

/**
 * @brief Get I2S driver capabilities
 */
static const audio_driver_caps_t *i2s_audio_get_capabilities(audio_driver_t *driver) {
    (void)driver; // Unused parameter
    return &i2s_capabilities;
}

/**
 * @brief Get I2S driver status
 */
static audio_driver_status_t i2s_audio_get_status(audio_driver_t *driver) {
    if (!driver) {
        return AUDIO_DRIVER_ERROR;
    }
    
    return driver->status;
}

/**
 * @brief Get I2S driver statistics
 */
static const audio_driver_stats_t *i2s_audio_get_stats(audio_driver_t *driver) {
    if (!driver) {
        return NULL;
    }
    
    return &driver->stats;
}

/**
 * @brief Set volume (not implemented for basic I2S)
 */
static bool i2s_audio_set_volume(audio_driver_t *driver, uint8_t volume) {
    (void)driver;
    (void)volume;
    return false; // Not supported by basic I2S interface
}

/**
 * @brief Set mute (not implemented for basic I2S)
 */
static bool i2s_audio_set_mute(audio_driver_t *driver, bool mute) {
    (void)driver;
    (void)mute;
    return false; // Not supported by basic I2S interface
}

/**
 * @brief I2S audio driver operations
 */
const audio_driver_ops_t i2s_audio_driver_ops = {
    .init = i2s_audio_init,
    .start = i2s_audio_start,
    .stop = i2s_audio_stop,
    .deinit = i2s_audio_deinit,
    .get_capabilities = i2s_audio_get_capabilities,
    .get_status = i2s_audio_get_status,
    .get_stats = i2s_audio_get_stats,
    .set_volume = i2s_audio_set_volume,
    .set_mute = i2s_audio_set_mute
};