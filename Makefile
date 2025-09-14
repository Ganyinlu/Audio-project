# Audio Project Makefile
# Embedded audio processing system with asynchronous I/O and driver support

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
INCLUDES = -Iinclude
LDFLAGS = -lm -lpthread

# Directories
SRC_DIR = src
INCLUDE_DIR = include
EXAMPLES_DIR = examples
TESTS_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
ASYNC_IO_SRCS = $(wildcard $(SRC_DIR)/async_io/*.c)
DRIVERS_SRCS = $(wildcard $(SRC_DIR)/drivers/*.c)
HAL_SRCS = $(wildcard $(SRC_DIR)/hal/*.c)
UTILS_SRCS = $(wildcard $(SRC_DIR)/utils/*.c)

ALL_SRCS = $(ASYNC_IO_SRCS) $(DRIVERS_SRCS) $(HAL_SRCS) $(UTILS_SRCS)

# Object files
ASYNC_IO_OBJS = $(ASYNC_IO_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DRIVERS_OBJS = $(DRIVERS_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
HAL_OBJS = $(HAL_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
UTILS_OBJS = $(UTILS_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

ALL_OBJS = $(ASYNC_IO_OBJS) $(DRIVERS_OBJS) $(HAL_OBJS) $(UTILS_OBJS)

# Example programs
EXAMPLES = $(EXAMPLES_DIR)/audio_loopback.c
EXAMPLE_BINS = $(EXAMPLES:$(EXAMPLES_DIR)/%.c=$(BIN_DIR)/%)

# Test programs
TEST_SRCS = $(wildcard $(TESTS_DIR)/*.c)
TEST_BINS = $(TEST_SRCS:$(TESTS_DIR)/%.c=$(BIN_DIR)/test_%)

# Library
LIB_NAME = libaudio
STATIC_LIB = $(BUILD_DIR)/$(LIB_NAME).a
SHARED_LIB = $(BUILD_DIR)/$(LIB_NAME).so

# Default target
all: $(STATIC_LIB) $(SHARED_LIB) examples

# Create directories
$(OBJ_DIR) $(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

$(OBJ_DIR)/async_io $(OBJ_DIR)/drivers $(OBJ_DIR)/hal $(OBJ_DIR)/utils: | $(OBJ_DIR)
	mkdir -p $@

# Compile source files
$(OBJ_DIR)/async_io/%.o: $(SRC_DIR)/async_io/%.c | $(OBJ_DIR)/async_io
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/drivers/%.o: $(SRC_DIR)/drivers/%.c | $(OBJ_DIR)/drivers
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/hal/%.o: $(SRC_DIR)/hal/%.c | $(OBJ_DIR)/hal
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/utils/%.o: $(SRC_DIR)/utils/%.c | $(OBJ_DIR)/utils
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create static library
$(STATIC_LIB): $(ALL_OBJS) | $(BUILD_DIR)
	ar rcs $@ $^
	@echo "Static library created: $@"

# Create shared library
$(SHARED_LIB): $(ALL_OBJS) | $(BUILD_DIR)
	$(CC) -shared -o $@ $^ $(LDFLAGS)
	@echo "Shared library created: $@"

# Build examples
examples: $(EXAMPLE_BINS)

$(BIN_DIR)/audio_loopback: $(EXAMPLES_DIR)/audio_loopback.c $(STATIC_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(STATIC_LIB) $(LDFLAGS) -o $@
	@echo "Example built: $@"

# Build tests
tests: $(TEST_BINS)

$(BIN_DIR)/test_%: $(TESTS_DIR)/%.c $(STATIC_LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(STATIC_LIB) $(LDFLAGS) -o $@
	@echo "Test built: $@"

# Run examples
run-examples: examples
	@echo "=== Running Audio Loopback Example ==="
	$(BIN_DIR)/audio_loopback

# Run tests
run-tests: tests
	@echo "=== Running Tests ==="
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	@echo "Build artifacts cleaned"

# Install (for system-wide installation)
install: $(STATIC_LIB) $(SHARED_LIB)
	@echo "Installing libraries and headers..."
	sudo mkdir -p /usr/local/lib /usr/local/include/audio
	sudo cp $(STATIC_LIB) $(SHARED_LIB) /usr/local/lib/
	sudo cp -r $(INCLUDE_DIR)/* /usr/local/include/audio/
	sudo ldconfig
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling..."
	sudo rm -f /usr/local/lib/$(LIB_NAME).*
	sudo rm -rf /usr/local/include/audio
	sudo ldconfig
	@echo "Uninstallation complete"

# Format code
format:
	@echo "Formatting code..."
	find $(SRC_DIR) $(INCLUDE_DIR) $(EXAMPLES_DIR) $(TESTS_DIR) -name "*.c" -o -name "*.h" | \
		xargs clang-format -i --style="{BasedOnStyle: LLVM, IndentWidth: 4, TabWidth: 4}"

# Check code style
check-style:
	@echo "Checking code style..."
	find $(SRC_DIR) $(INCLUDE_DIR) $(EXAMPLES_DIR) $(TESTS_DIR) -name "*.c" -o -name "*.h" | \
		xargs clang-format --dry-run --Werror --style="{BasedOnStyle: LLVM, IndentWidth: 4, TabWidth: 4}"

# Static analysis
analyze:
	@echo "Running static analysis..."
	cppcheck --enable=all --inconclusive --std=c99 $(INCLUDES) $(SRC_DIR) $(EXAMPLES_DIR)

# Generate documentation
docs:
	@echo "Generating documentation..."
	doxygen Doxyfile

# Show help
help:
	@echo "Audio Project Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build libraries and examples (default)"
	@echo "  examples     - Build example programs"
	@echo "  tests        - Build test programs"
	@echo "  run-examples - Run example programs"
	@echo "  run-tests    - Run test programs"
	@echo "  clean        - Clean build artifacts"
	@echo "  install      - Install system-wide"
	@echo "  uninstall    - Uninstall from system"
	@echo "  format       - Format source code"
	@echo "  check-style  - Check code formatting"
	@echo "  analyze      - Run static analysis"
	@echo "  docs         - Generate documentation"
	@echo "  help         - Show this help"

# Debug information
debug-info:
	@echo "=== Debug Information ==="
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "INCLUDES: $(INCLUDES)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "ALL_SRCS: $(ALL_SRCS)"
	@echo "ALL_OBJS: $(ALL_OBJS)"
	@echo "EXAMPLE_BINS: $(EXAMPLE_BINS)"
	@echo "TEST_BINS: $(TEST_BINS)"

# Phony targets
.PHONY: all examples tests run-examples run-tests clean install uninstall format check-style analyze docs help debug-info

# Dependencies (automatically generated)
-include $(ALL_OBJS:.o=.d)

# Generate dependency files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@