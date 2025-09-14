# API Reference

## Asynchronous I/O Framework

### Data Structures

#### `async_io_config_t`
Configuration structure for initializing the async I/O framework.

```c
typedef struct {
    uint32_t max_operations;        // Maximum concurrent operations
    uint32_t buffer_size;           // Default buffer size
    uint32_t interrupt_priority;    // Interrupt priority level
    bool enable_dma;                // Enable DMA transfers
} async_io_config_t;
```

#### `async_io_operation_t`
Represents an asynchronous I/O operation.

```c
struct async_io_operation {
    async_io_type_t type;           // Operation type (READ/WRITE/DUPLEX)
    async_io_status_t status;       // Current status
    uint8_t *buffer;                // Data buffer
    size_t buffer_size;             // Buffer size in bytes
    size_t bytes_transferred;       // Bytes actually transferred
    uint32_t timeout_ms;            // Timeout in milliseconds
    async_io_callback_t callback;   // Completion callback
    void *user_data;                // User data for callback
    uint32_t priority;              // Operation priority (0 = highest)
    struct async_io_operation *next; // Next operation in queue
};
```

### Functions

#### `async_io_init()`
Initialize the asynchronous I/O framework.

```c
async_io_context_t *async_io_init(const async_io_config_t *config);
```

**Parameters:**
- `config`: Configuration parameters

**Returns:**
- Pointer to initialized context, `NULL` on failure

**Example:**
```c
async_io_config_t config = {
    .max_operations = 16,
    .buffer_size = 1024,
    .interrupt_priority = 5,
    .enable_dma = true
};

async_io_context_t *ctx = async_io_init(&config);
if (!ctx) {
    // Handle initialization failure
}
```

#### `async_io_submit()`
Submit an asynchronous I/O operation.

```c
bool async_io_submit(async_io_context_t *context, async_io_operation_t *operation);
```

**Parameters:**
- `context`: I/O context
- `operation`: Operation to submit

**Returns:**
- `true` if operation was queued successfully, `false` otherwise

**Example:**
```c
async_io_operation_t operation = {
    .type = ASYNC_IO_READ,
    .buffer = my_buffer,
    .buffer_size = 1024,
    .timeout_ms = 5000,
    .callback = my_completion_callback,
    .priority = 1
};

if (!async_io_submit(ctx, &operation)) {
    // Handle submission failure
}
```

## Circular Buffer

### Data Structures

#### `circular_buffer_t`
Circular buffer structure for lock-free data storage.

```c
typedef struct {
    uint8_t *buffer;        // Data buffer
    volatile size_t head;   // Write position
    volatile size_t tail;   // Read position
    size_t size;            // Buffer size (must be power of 2)
    size_t mask;            // Size mask for efficient modulo operation
} circular_buffer_t;
```

### Functions

#### `circular_buffer_init()`
Initialize a circular buffer.

```c
bool circular_buffer_init(circular_buffer_t *cb, uint8_t *buffer, size_t size);
```

**Parameters:**
- `cb`: Pointer to circular buffer structure
- `buffer`: Data buffer (must be aligned for DMA if used)
- `size`: Buffer size in bytes (must be power of 2)

**Returns:**
- `true` if initialization successful, `false` otherwise

**Example:**
```c
uint8_t buffer_memory[1024];  // Must be power of 2
circular_buffer_t cb;

if (!circular_buffer_init(&cb, buffer_memory, sizeof(buffer_memory))) {
    // Handle initialization failure
}
```

#### `circular_buffer_write()`
Write data to circular buffer.

```c
size_t circular_buffer_write(circular_buffer_t *cb, const uint8_t *data, size_t length);
```

**Parameters:**
- `cb`: Pointer to circular buffer
- `data`: Data to write
- `length`: Number of bytes to write

**Returns:**
- Number of bytes actually written

#### `circular_buffer_read()`
Read data from circular buffer.

```c
size_t circular_buffer_read(circular_buffer_t *cb, uint8_t *data, size_t length);
```

**Parameters:**
- `cb`: Pointer to circular buffer
- `data`: Buffer to read data into
- `length`: Number of bytes to read

**Returns:**
- Number of bytes actually read

## Audio Driver Framework

### Data Structures

#### `audio_config_t`
Audio configuration parameters.

```c
typedef struct {
    audio_format_t format;      // Audio format
    uint32_t sample_rate;       // Sample rate in Hz
    uint8_t channels;           // Number of channels
    uint16_t frame_size;        // Size of one audio frame in bytes
    uint32_t buffer_size;       // Buffer size in frames
    bool enable_input;          // Enable audio input
    bool enable_output;         // Enable audio output
    bool use_dma;               // Use DMA for transfers
    uint8_t dma_priority;       // DMA priority level
} audio_config_t;
```

#### `audio_driver_t`
Audio driver instance structure.

```c
struct audio_driver {
    const audio_driver_ops_t *ops;      // Driver operations
    void *private_data;                 // Driver private data
    audio_config_t config;              // Current configuration
    audio_driver_status_t status;       // Current status
    audio_driver_stats_t stats;         // Driver statistics
    
    // Callbacks
    audio_input_callback_t input_callback;
    audio_output_callback_t output_callback;
    audio_error_callback_t error_callback;
    void *callback_user_data;
    
    // Async I/O integration
    async_io_context_t *async_io_ctx;
};
```

### Functions

#### `audio_driver_init()`
Initialize an audio driver.

```c
bool audio_driver_init(audio_driver_t *driver, const audio_driver_ops_t *ops, 
                      const audio_config_t *config);
```

**Parameters:**
- `driver`: Driver instance to initialize
- `ops`: Driver operations structure
- `config`: Audio configuration

**Returns:**
- `true` if successful, `false` otherwise

**Example:**
```c
audio_driver_t driver;
audio_config_t config = {
    .format = AUDIO_FORMAT_I2S_STANDARD,
    .sample_rate = 44100,
    .channels = 2,
    .frame_size = 4,  // 2 channels * 16-bit
    .buffer_size = 1024,
    .enable_input = true,
    .enable_output = true,
    .use_dma = true,
    .dma_priority = 6
};

if (!audio_driver_init(&driver, &i2s_audio_driver_ops, &config)) {
    // Handle initialization failure
}
```

#### `audio_driver_set_callbacks()`
Set audio callbacks.

```c
void audio_driver_set_callbacks(audio_driver_t *driver,
                               audio_input_callback_t input_cb,
                               audio_output_callback_t output_cb,
                               audio_error_callback_t error_cb,
                               void *user_data);
```

**Parameters:**
- `driver`: Driver instance
- `input_cb`: Input callback (can be NULL)
- `output_cb`: Output callback (can be NULL)
- `error_cb`: Error callback (can be NULL)
- `user_data`: User data for callbacks

**Example:**
```c
void input_callback(const void *input_buffer, size_t frames, void *user_data) {
    // Process input audio data
}

void output_callback(void *output_buffer, size_t frames, void *user_data) {
    // Provide output audio data
}

void error_callback(audio_driver_status_t status, void *user_data) {
    // Handle audio errors
}

audio_driver_set_callbacks(&driver, input_callback, output_callback, 
                          error_callback, NULL);
```

## I2S Hardware Abstraction Layer

### Data Structures

#### `i2s_config_t`
I2S configuration structure.

```c
typedef struct {
    i2s_mode_t mode;                // I2S mode
    i2s_format_t format;            // I2S format
    i2s_data_width_t data_width;    // Data width
    i2s_channel_t channels;         // Channel configuration
    uint32_t sample_rate;           // Sample rate in Hz
    i2s_clock_source_t clock_source; // Clock source
    i2s_pin_config_t pins;          // Pin configuration
    i2s_dma_config_t dma;           // DMA configuration
    uint8_t interrupt_priority;     // Interrupt priority
} i2s_config_t;
```

### Functions

#### `i2s_hal_init()`
Initialize I2S HAL.

```c
bool i2s_hal_init(i2s_instance_t instance, const i2s_config_t *config);
```

**Parameters:**
- `instance`: I2S instance
- `config`: I2S configuration

**Returns:**
- `true` if successful, `false` otherwise

**Example:**
```c
i2s_config_t i2s_config = {
    .mode = I2S_MODE_MASTER_RXTX,
    .format = I2S_FORMAT_STANDARD,
    .data_width = I2S_DATA_WIDTH_16BIT,
    .channels = I2S_CHANNEL_STEREO,
    .sample_rate = 44100,
    .clock_source = I2S_CLOCK_INTERNAL,
    .pins = {
        .bclk_pin = 26,
        .ws_pin = 25,
        .dout_pin = 22,
        .din_pin = 23
    },
    .dma = {
        .enable = true,
        .tx_channel = 0,
        .rx_channel = 1,
        .priority = 6,
        .buffer_size = 4096
    },
    .interrupt_priority = 5
};

if (!i2s_hal_init(I2S_INSTANCE_0, &i2s_config)) {
    // Handle initialization failure
}
```

## Error Codes and Status Values

### Async I/O Status

```c
typedef enum {
    ASYNC_IO_PENDING,   // Operation is pending
    ASYNC_IO_COMPLETE,  // Operation completed successfully
    ASYNC_IO_ERROR,     // Operation failed
    ASYNC_IO_TIMEOUT    // Operation timed out
} async_io_status_t;
```

### Audio Driver Status

```c
typedef enum {
    AUDIO_DRIVER_STOPPED,       // Driver is stopped
    AUDIO_DRIVER_RUNNING,       // Driver is running
    AUDIO_DRIVER_ERROR,         // Driver encountered an error
    AUDIO_DRIVER_OVERFLOW,      // Input buffer overflow
    AUDIO_DRIVER_UNDERFLOW      // Output buffer underflow
} audio_driver_status_t;
```

## Usage Patterns

### Basic Audio Loopback

```c
// Initialize async I/O
async_io_config_t async_config = {
    .max_operations = 16,
    .buffer_size = 1024,
    .interrupt_priority = 5,
    .enable_dma = true
};
async_io_context_t *async_ctx = async_io_init(&async_config);

// Initialize audio driver
audio_config_t audio_config = {
    .format = AUDIO_FORMAT_I2S_STANDARD,
    .sample_rate = 44100,
    .channels = 2,
    .frame_size = 4,
    .buffer_size = 1024,
    .enable_input = true,
    .enable_output = true,
    .use_dma = true
};

audio_driver_t driver;
audio_driver_init(&driver, &i2s_audio_driver_ops, &audio_config);
audio_driver_set_callbacks(&driver, input_callback, output_callback, 
                          error_callback, NULL);
audio_driver_set_async_io(&driver, async_ctx);

// Start processing
audio_driver_start(&driver);

// Main loop
while (running) {
    async_io_process(async_ctx);
    // Other processing...
}

// Cleanup
audio_driver_stop(&driver);
audio_driver_deinit(&driver);
async_io_deinit(async_ctx);
```

### Zero-Copy Buffer Operations

```c
// Get direct write pointer
size_t space_available;
uint8_t *write_ptr = circular_buffer_write_ptr(&cb, &space_available);
if (write_ptr && space_available >= required_size) {
    // Write data directly to buffer
    generate_audio_data(write_ptr, required_size);
    
    // Advance write pointer
    circular_buffer_write_advance(&cb, required_size);
}

// Get direct read pointer
size_t data_available;
const uint8_t *read_ptr = circular_buffer_read_ptr(&cb, &data_available);
if (read_ptr && data_available >= required_size) {
    // Process data directly from buffer
    process_audio_data(read_ptr, required_size);
    
    // Advance read pointer
    circular_buffer_read_advance(&cb, required_size);
}
```

## Best Practices

1. **Buffer Sizes**: Always use power-of-2 buffer sizes for optimal performance
2. **Error Handling**: Always check return values and handle errors appropriately
3. **Memory Alignment**: Ensure DMA buffers are properly aligned
4. **Interrupt Context**: Keep interrupt service routines short and simple
5. **Priority Management**: Set appropriate priorities for real-time operations
6. **Resource Cleanup**: Always clean up resources in reverse initialization order