#include "drivers/audio_driver.h"
#include <string.h>

bool audio_driver_init(audio_driver_t *driver, const audio_driver_ops_t *ops, 
                      const audio_config_t *config) {
    if (!driver || !ops || !config) {
        return false;
    }
    
    // Initialize driver structure
    memset(driver, 0, sizeof(audio_driver_t));
    driver->ops = ops;
    memcpy(&driver->config, config, sizeof(audio_config_t));
    driver->status = AUDIO_DRIVER_STOPPED;
    
    // Initialize driver using ops
    if (ops->init) {
        return ops->init(driver, config);
    }
    
    return true;
}

void audio_driver_set_callbacks(audio_driver_t *driver,
                               audio_input_callback_t input_cb,
                               audio_output_callback_t output_cb,
                               audio_error_callback_t error_cb,
                               void *user_data) {
    if (!driver) {
        return;
    }
    
    driver->input_callback = input_cb;
    driver->output_callback = output_cb;
    driver->error_callback = error_cb;
    driver->callback_user_data = user_data;
}

void audio_driver_set_async_io(audio_driver_t *driver, async_io_context_t *async_io_ctx) {
    if (driver) {
        driver->async_io_ctx = async_io_ctx;
    }
}

bool audio_driver_start(audio_driver_t *driver) {
    if (!driver || !driver->ops) {
        return false;
    }
    
    if (driver->ops->start) {
        bool result = driver->ops->start(driver);
        if (result) {
            driver->status = AUDIO_DRIVER_RUNNING;
        }
        return result;
    }
    
    return false;
}

void audio_driver_stop(audio_driver_t *driver) {
    if (!driver || !driver->ops) {
        return;
    }
    
    if (driver->ops->stop) {
        driver->ops->stop(driver);
        driver->status = AUDIO_DRIVER_STOPPED;
    }
}

void audio_driver_deinit(audio_driver_t *driver) {
    if (!driver || !driver->ops) {
        return;
    }
    
    // Stop driver first
    audio_driver_stop(driver);
    
    // Deinitialize using ops
    if (driver->ops->deinit) {
        driver->ops->deinit(driver);
    }
    
    // Clear structure
    memset(driver, 0, sizeof(audio_driver_t));
}