# Copyright (c) 2018 Daniel Abrecht
# SPDX-License-Identifier: AGPL-3.0-or-later

SOURCES = $(wildcard src/sequencehandler/*.c)
SOURCES += src/libttymultiplex.c
SOURCES += src/main.c
SOURCES += src/pane.c
SOURCES += src/pane_flag.c
SOURCES += src/calc.c
SOURCES += src/list.c
SOURCES += src/pseudoterminal.c
SOURCES += src/utils.c
SOURCES += src/parser.c

HEADERS = $(wildcard include/*.h) $(wildcard include/**/*.h)

OBJS = $(patsubst src/%.c,build/%.o,$(SOURCES))

CC = gcc
AR = ar

OPTIONS  = -fPIC -pthread -ffunction-sections -fdata-sections -g -Og
CC_OPTS  = -fvisibility=hidden -DTYM_BUILD -I include
CC_OPTS += -std=c99 -Wall -Wextra -pedantic -Wno-unused-function -Werror -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE
LD_OPTS  = --shared -Wl,-gc-sections -Wl,-no-undefined

LD_OPTS += -lutil -lncursesw
#-ltermcap -ltinfo

CC_OPTS += $(OPTIONS)
LD_OPTS += $(OPTIONS)

all: bin/libttymultiplex.so bin/libttymultiplex.a

%/.dir:
	mkdir -p "$(dir $@)"
	touch "$@"

build/%.o: src/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) $(CC_OPTS) -c -o $@ $<

bin/libttymultiplex.a: $(OBJS) | bin/.dir
	$(AR) scr $@ $^

bin/libttymultiplex.so: bin/libttymultiplex.a | bin/.dir
	$(CC) $(LD_OPTS) -Wl,--whole-archive $^ -Wl,--no-whole-archive -o $@

clean:
	rm -rf bin/ build/
