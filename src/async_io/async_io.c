#include "async_io/async_io.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Asynchronous I/O context structure
 */
struct async_io_context {
    async_io_config_t config;           /**< Configuration */
    async_io_operation_t *pending_head; /**< Head of pending operations queue */
    async_io_operation_t *pending_tail; /**< Tail of pending operations queue */
    async_io_operation_t *operations;   /**< Pool of operation structures */
    uint32_t pending_count;             /**< Number of pending operations */
    uint32_t next_operation_id;         /**< Next operation ID */
    bool initialized;                   /**< Initialization flag */
};

/**
 * @brief Find operation by ID in pending queue
 */
static async_io_operation_t *find_operation_by_id(async_io_context_t *context, uint32_t id) {
    async_io_operation_t *op = context->pending_head;
    uint32_t current_id = 0;
    while (op) {
        if (current_id == id) {
            return op;
        }
        op = op->next;
        current_id++;
    }
    return NULL;
}

/**
 * @brief Remove operation from pending queue
 */
static void remove_from_pending_queue(async_io_context_t *context, async_io_operation_t *operation) {
    if (!operation || context->pending_count == 0) {
        return;
    }
    
    // Find previous operation
    async_io_operation_t *prev = NULL;
    async_io_operation_t *current = context->pending_head;
    
    while (current && current != operation) {
        prev = current;
        current = current->next;
    }
    
    if (!current) {
        return; // Operation not found
    }
    
    // Remove from queue
    if (prev) {
        prev->next = operation->next;
    } else {
        context->pending_head = operation->next;
    }
    
    if (operation == context->pending_tail) {
        context->pending_tail = prev;
    }
    
    operation->next = NULL;
    context->pending_count--;
}

/**
 * @brief Add operation to pending queue (sorted by priority)
 */
static void add_to_pending_queue(async_io_context_t *context, async_io_operation_t *operation) {
    if (!operation) {
        return;
    }
    
    // Find insertion point (sorted by priority, 0 = highest)
    async_io_operation_t *prev = NULL;
    async_io_operation_t *current = context->pending_head;
    
    while (current && current->priority <= operation->priority) {
        prev = current;
        current = current->next;
    }
    
    // Insert operation
    operation->next = current;
    
    if (prev) {
        prev->next = operation;
    } else {
        context->pending_head = operation;
    }
    
    if (!current) {
        context->pending_tail = operation;
    }
    
    context->pending_count++;
}

async_io_context_t *async_io_init(const async_io_config_t *config) {
    if (!config || config->max_operations == 0) {
        return NULL;
    }
    
    async_io_context_t *context = malloc(sizeof(async_io_context_t));
    if (!context) {
        return NULL;
    }
    
    // Allocate operation pool
    context->operations = malloc(sizeof(async_io_operation_t) * config->max_operations);
    if (!context->operations) {
        free(context);
        return NULL;
    }
    
    // Initialize context
    memcpy(&context->config, config, sizeof(async_io_config_t));
    context->pending_head = NULL;
    context->pending_tail = NULL;
    context->pending_count = 0;
    context->next_operation_id = 0;
    context->initialized = true;
    
    // Initialize operation pool
    memset(context->operations, 0, sizeof(async_io_operation_t) * config->max_operations);
    
    return context;
}

void async_io_deinit(async_io_context_t *context) {
    if (!context || !context->initialized) {
        return;
    }
    
    // Cancel all pending operations
    while (context->pending_head) {
        async_io_operation_t *op = context->pending_head;
        op->status = ASYNC_IO_ERROR;
        if (op->callback) {
            op->callback(op, op->user_data);
        }
        remove_from_pending_queue(context, op);
    }
    
    free(context->operations);
    context->initialized = false;
    free(context);
}

bool async_io_submit(async_io_context_t *context, async_io_operation_t *operation) {
    if (!context || !context->initialized || !operation) {
        return false;
    }
    
    if (context->pending_count >= context->config.max_operations) {
        return false; // Queue full
    }
    
    // Initialize operation
    operation->status = ASYNC_IO_PENDING;
    operation->bytes_transferred = 0;
    operation->next = NULL;
    
    // Add to pending queue
    add_to_pending_queue(context, operation);
    
    return true;
}

bool async_io_cancel(async_io_context_t *context, async_io_operation_t *operation) {
    if (!context || !context->initialized || !operation) {
        return false;
    }
    
    // Check if operation is in pending queue
    async_io_operation_t *current = context->pending_head;
    while (current) {
        if (current == operation) {
            operation->status = ASYNC_IO_ERROR;
            remove_from_pending_queue(context, operation);
            
            // Call callback if provided
            if (operation->callback) {
                operation->callback(operation, operation->user_data);
            }
            
            return true;
        }
        current = current->next;
    }
    
    return false; // Operation not found or already completed
}

uint32_t async_io_process(async_io_context_t *context) {
    if (!context || !context->initialized) {
        return 0;
    }
    
    uint32_t processed = 0;
    async_io_operation_t *current = context->pending_head;
    
    while (current) {
        async_io_operation_t *next = current->next;
        
        // Check for timeout
        if (current->timeout_ms > 0) {
            // TODO: Implement timeout checking with system timer
            // For now, assume no timeouts in simulation
        }
        
        // Process completed operations
        if (current->status == ASYNC_IO_COMPLETE || 
            current->status == ASYNC_IO_ERROR ||
            current->status == ASYNC_IO_TIMEOUT) {
            
            remove_from_pending_queue(context, current);
            
            // Call completion callback
            if (current->callback) {
                current->callback(current, current->user_data);
            }
            
            processed++;
        }
        
        current = next;
    }
    
    return processed;
}

uint32_t async_io_pending_count(async_io_context_t *context) {
    if (!context || !context->initialized) {
        return 0;
    }
    
    return context->pending_count;
}

void async_io_isr(async_io_context_t *context, uint32_t operation_id, 
                  size_t bytes_transferred, uint32_t error_code) {
    if (!context || !context->initialized) {
        return;
    }
    
    // Find operation by ID
    async_io_operation_t *operation = find_operation_by_id(context, operation_id);
    if (!operation) {
        return;
    }
    
    // Update operation status
    operation->bytes_transferred = bytes_transferred;
    
    if (error_code == 0) {
        operation->status = ASYNC_IO_COMPLETE;
    } else {
        operation->status = ASYNC_IO_ERROR;
    }
    
    // Note: Actual callback will be called in async_io_process() to avoid
    // calling user code from interrupt context
}