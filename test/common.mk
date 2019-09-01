PROJECT_ROOT = $(realpath ../..)
TEST_DIR = $(realpath ..)
NAME = $(notdir $(realpath .))

BIN_DIR = $(TEST_DIR)/bin
BUILD_DIR = $(TEST_DIR)/build/$(NAME)
SRC_DIR = src

TS_BIN = ../test-summary/bin/test-summary

export PATH := $(PATH):$(TEST_DIR)/script:$(TEST_DIR)/test-summary/bin

export TERMINFO_DIRS = $(PROJECT_ROOT)/build/terminfo/

BIN = $(BIN_DIR)/$(NAME)

LIBTTYMULTIPLEX_BASE_A = build/libttymultiplex.a
ABS_LIBTTYMULTIPLEX_BASE_A = $(PROJECT_ROOT)/$(LIBTTYMULTIPLEX_BASE_A)

TERMINFO_BASE = bin/terminfo/
ABS_TERMINFO_BASE = $(PROJECT_ROOT)/$(TERMINFO_BASE)

HEADERS += $(wildcard include/*.h) $(wildcard include/**/*.h)
HEADERS += $(wildcard $(PROJECT_ROOT)/*.h) $(wildcard $(PROJECT_ROOT)/include/**/*.h)

ifdef DEBUG
CC_OPTS += -Og -g
endif

ifndef LENIENT
CC_OPTS = -Werror
endif

CC_OPTS += -std=c99 -Wall -Wextra -pedantic
CC_OPTS += -D_DEFAULT_SOURCE
CC_OPTS += -Iinclude
CC_OPTS += -I$(PROJECT_ROOT)/include

CC_OPTS += -DTYM_LOG_PROJECT='"test-$(NAME)"'
CC_OPTS += -DTYM_I_BACKEND_NAME='"$(NAME)"'
CC_OPTS += -DTYM_I_BACKEND_ID="B_$$(printf "$(BACKEND)" | sed 's/[^a-zA-Z0-9]/_/g')"
CC_OPTS += -DTYM_I_BACKEND_PRIORITY=100

LD_OPTS += -ldl -lutil -pthread

OBJS += $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.c.o,$(SOURCES))

always:

test: test-base $(TS_BASE)
	test-summary "$(NAME)" $(MAKE) do-test

$(TS_BIN): always
	$(MAKE) -C .. test-summary/bin/test-summary

bin-base: $(BIN)

$(BIN): $(ABS_LIBTTYMULTIPLEX_BASE_A) $(OBJS)
	mkdir -p "$(dir $@)"
	$(CC) -o "$@" -Wl,--whole-archive $^ -Wl,--no-whole-archive  $(LD_OPTS) $(LDFLAGS)

$(BUILD_DIR)/%.c.o: $(SRC_DIR)/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CFLAGS) "$<"

$(ABS_LIBTTYMULTIPLEX_BASE_A):
	$(MAKE) -C "$(PROJECT_ROOT)" "$(LIBTTYMULTIPLEX_BASE_A)"

$(ABS_TERMINFO_BASE): always
	$(MAKE) -C "$(PROJECT_ROOT)" "$(TERMINFO_BASE)"

clean-base:
	rm -rf "$(BUILD_DIR)" "$(BIN_DIR)"

test-base: bin $(TS_BIN) $(ABS_TERMINFO_BASE)
