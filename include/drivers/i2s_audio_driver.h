#ifndef I2S_AUDIO_DRIVER_H
#define I2S_AUDIO_DRIVER_H

#include "drivers/audio_driver.h"

/**
 * @file i2s_audio_driver.h
 * @brief I2S audio driver implementation
 * 
 * This module provides a concrete implementation of the audio driver interface
 * for I2S (Inter-IC Sound) peripherals.
 */

/**
 * @brief I2S audio driver operations
 */
extern const audio_driver_ops_t i2s_audio_driver_ops;

#endif /* I2S_AUDIO_DRIVER_H */