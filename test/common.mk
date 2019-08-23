PROJECT_ROOT = ../..
TEST_DIR = ..
NAME = $(notdir $(realpath .))

BIN_DIR = $(TEST_DIR)/bin
BUILD_DIR = $(TEST_DIR)/build/$(NAME)
SRC_DIR = src

BIN = $(BIN_DIR)/$(NAME)

LIBTTYMULTIPLEX_BASE_A = build/libttymultiplex.a
ABS_LIBTTYMULTIPLEX_BASE_A = $(PROJECT_ROOT)/$(LIBTTYMULTIPLEX_BASE_A)

CC = gcc

HEADERS += $(wildcard include/*.h) $(wildcard include/**/*.h)
HEADERS += $(wildcard $(PROJECT_ROOT)/*.h) $(wildcard $(PROJECT_ROOT)/include/**/*.h)

OPTS += -pthread -lutil

CC_OPTS += -std=c99 -Wall -Wextra -pedantic -Werror
CC_OPTS += -Og -g
CC_OPTS += -D_DEFAULT_SOURCE
CC_OPTS += -Iinclude
CC_OPTS += -I$(PROJECT_ROOT)/include

CC_OPTS += -DTYM_LOG_PROJECT='"test-$(NAME)"'
CC_OPTS += -DTYM_I_BACKEND_NAME='"$(NAME)"'
CC_OPTS += -DTYM_I_BACKEND_ID="B_$$(printf "$(BACKEND)" | sed 's/[^a-zA-Z0-9]/_/g')"
CC_OPTS += -DTYM_I_BACKEND_PRIORITY=100

LD_OPTS += -ldl
CC_OPTS += $(OPTS)
LD_OPTS += $(OPTS)

OBJS += $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

bin-base: $(BIN)

$(BIN): $(ABS_LIBTTYMULTIPLEX_BASE_A) $(OBJS)
	mkdir -p "$(dir $@)"
	$(CC) $(LD_OPTS) -Wl,--whole-archive $^ -Wl,--no-whole-archive -o "$@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) $(CC_OPTS) "$<" -c -o "$@"

$(ABS_LIBTTYMULTIPLEX_BASE_A):
	$(MAKE) -C "$(PROJECT_ROOT)" "$(LIBTTYMULTIPLEX_BASE_A)"

clean-base:
	rm -rf "$(BUILD_DIR)" "$(BIN_DIR)"

test-base: bin
