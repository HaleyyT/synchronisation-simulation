# Compiler and flags
CC       = gcc
CFLAGS   = -Wall -Wextra -pedantic -std=c11 -pthread -g
LDFLAGS  =
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -g -O0


# Directories
SRC_DIR  = src
INC_DIR  = include
OBJ_DIR  = obj
BIN      = sim

# Sources/objects
SRC      = $(wildcard $(SRC_DIR)/*.c)
OBJ      = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))


# Link (single rule; includes $(LDFLAGS))
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Ensure obj dir exists
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Always rebuild from scratch for a normal build
all: clean $(BIN)

# ASan build also forces a clean, then rebuilds with extra flags
asan: clean
	$(MAKE) CFLAGS="$(CFLAGS) $(ASAN_FLAGS)" LDFLAGS="$(LDFLAGS) -fsanitize=address" $(BIN)
	@echo "Built with AddressSanitizer."

run-asan: asan
	ASAN_OPTIONS="strict_string_checks=1:abort_on_error=1:fast_unwind_on_malloc=0" ./$(BIN)

# Clean up build artifacts
clean:
	$(RM) -rf $(OBJ_DIR) $(BIN)

# Phony targets
.PHONY: all clean asan run-asan
