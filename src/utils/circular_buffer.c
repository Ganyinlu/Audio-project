#include "utils/circular_buffer.h"
#include <string.h>

/**
 * @brief Check if size is power of 2
 */
static bool is_power_of_2(size_t size) {
    return size > 0 && (size & (size - 1)) == 0;
}

bool circular_buffer_init(circular_buffer_t *cb, uint8_t *buffer, size_t size) {
    if (!cb || !buffer || !is_power_of_2(size)) {
        return false;
    }
    
    cb->buffer = buffer;
    cb->head = 0;
    cb->tail = 0;
    cb->size = size;
    cb->mask = size - 1;
    
    return true;
}

size_t circular_buffer_write(circular_buffer_t *cb, const uint8_t *data, size_t length) {
    if (!cb || !data || length == 0) {
        return 0;
    }
    
    size_t space = circular_buffer_space(cb);
    if (length > space) {
        length = space;
    }
    
    size_t head = cb->head;
    size_t first_chunk = cb->size - head;
    
    if (length <= first_chunk) {
        // Single chunk write
        memcpy(&cb->buffer[head], data, length);
    } else {
        // Split write
        memcpy(&cb->buffer[head], data, first_chunk);
        memcpy(&cb->buffer[0], data + first_chunk, length - first_chunk);
    }
    
    cb->head = (head + length) & cb->mask;
    return length;
}

size_t circular_buffer_read(circular_buffer_t *cb, uint8_t *data, size_t length) {
    if (!cb || !data || length == 0) {
        return 0;
    }
    
    size_t available = circular_buffer_available(cb);
    if (length > available) {
        length = available;
    }
    
    size_t tail = cb->tail;
    size_t first_chunk = cb->size - tail;
    
    if (length <= first_chunk) {
        // Single chunk read
        memcpy(data, &cb->buffer[tail], length);
    } else {
        // Split read
        memcpy(data, &cb->buffer[tail], first_chunk);
        memcpy(data + first_chunk, &cb->buffer[0], length - first_chunk);
    }
    
    cb->tail = (tail + length) & cb->mask;
    return length;
}

size_t circular_buffer_peek(circular_buffer_t *cb, uint8_t *data, size_t length) {
    if (!cb || !data || length == 0) {
        return 0;
    }
    
    size_t available = circular_buffer_available(cb);
    if (length > available) {
        length = available;
    }
    
    size_t tail = cb->tail;
    size_t first_chunk = cb->size - tail;
    
    if (length <= first_chunk) {
        // Single chunk peek
        memcpy(data, &cb->buffer[tail], length);
    } else {
        // Split peek
        memcpy(data, &cb->buffer[tail], first_chunk);
        memcpy(data + first_chunk, &cb->buffer[0], length - first_chunk);
    }
    
    return length;
}

size_t circular_buffer_available(const circular_buffer_t *cb) {
    if (!cb) {
        return 0;
    }
    return (cb->head - cb->tail) & cb->mask;
}

size_t circular_buffer_space(const circular_buffer_t *cb) {
    if (!cb) {
        return 0;
    }
    return (cb->tail - cb->head - 1) & cb->mask;
}

bool circular_buffer_empty(const circular_buffer_t *cb) {
    return cb && (cb->head == cb->tail);
}

bool circular_buffer_full(const circular_buffer_t *cb) {
    return cb && (circular_buffer_space(cb) == 0);
}

void circular_buffer_reset(circular_buffer_t *cb) {
    if (cb) {
        cb->head = 0;
        cb->tail = 0;
    }
}

uint8_t *circular_buffer_write_ptr(circular_buffer_t *cb, size_t *available) {
    if (!cb || !available) {
        return NULL;
    }
    
    if (circular_buffer_full(cb)) {
        *available = 0;
        return NULL;
    }
    
    size_t head = cb->head;
    size_t space = circular_buffer_space(cb);
    
    // Calculate contiguous space from head to end of buffer
    size_t contiguous = cb->size - head;
    if (contiguous > space) {
        contiguous = space;
    }
    
    *available = contiguous;
    return &cb->buffer[head];
}

void circular_buffer_write_advance(circular_buffer_t *cb, size_t bytes) {
    if (cb && bytes <= circular_buffer_space(cb)) {
        cb->head = (cb->head + bytes) & cb->mask;
    }
}

const uint8_t *circular_buffer_read_ptr(circular_buffer_t *cb, size_t *available) {
    if (!cb || !available) {
        return NULL;
    }
    
    if (circular_buffer_empty(cb)) {
        *available = 0;
        return NULL;
    }
    
    size_t tail = cb->tail;
    size_t data_available = circular_buffer_available(cb);
    
    // Calculate contiguous data from tail to end of buffer
    size_t contiguous = cb->size - tail;
    if (contiguous > data_available) {
        contiguous = data_available;
    }
    
    *available = contiguous;
    return &cb->buffer[tail];
}

void circular_buffer_read_advance(circular_buffer_t *cb, size_t bytes) {
    if (cb && bytes <= circular_buffer_available(cb)) {
        cb->tail = (cb->tail + bytes) & cb->mask;
    }
}