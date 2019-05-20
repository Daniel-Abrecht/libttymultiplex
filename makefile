# Copyright (c) 2018 Daniel Abrecht
# SPDX-License-Identifier: AGPL-3.0-or-later

SOURCES += $(wildcard src/sequencehandler/*.c)
SOURCES += $(wildcard src/backend/*.c)
SOURCES += src/libttymultiplex.c
SOURCES += src/main.c
SOURCES += src/pane.c
SOURCES += src/pane_flag.c
SOURCES += src/calc.c
SOURCES += src/list.c
SOURCES += src/pseudoterminal.c
SOURCES += src/utils.c
SOURCES += src/parser.c
SOURCES += src/charset.c
SOURCES += src/utf8.c
SOURCES += src/backend.c
SOURCES += src/backend_default_procs.c

HEADERS = $(wildcard include/*.h) $(wildcard include/**/*.h)

OBJS = $(patsubst src/%.c,build/%.o,$(SOURCES))

PREFIX = /usr

CC = gcc
AR = ar

INCLUDES = $(shell ncursesw5-config --cflags) -Iinclude
DEFAULT_INCLUDES = $(shell $(CC) -Wp,-v -x c++ -fsyntax-only /dev/null 2>&1 | sed -n '/search starts here/,/End of search list/p' | grep '^ ')

CPPCHECK_OPTIONS += --enable=all
CPPCHECK_OPTIONS += $(INCLUDES) $(addprefix -I,$(DEFAULT_INCLUDES)) --std=c99 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE -DTYM_BUILD
CPPCHECK_OPTIONS += --std=c99 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE -DTYM_BUILD
CPPCHECK_OPTIONS += $(CPPCHECK_OPTS)

OPTIONS += -fPIC -pthread -ffunction-sections -fdata-sections -fstack-protector-all -g -Og
CC_OPTS += -fvisibility=hidden -DTYM_BUILD -finput-charset=UTF-8
CC_OPTS += $(INCLUDES)
CC_OPTS += -std=c99 -Wall -Wextra -pedantic -Werror -Wno-implicit-fallthrough -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE
LD_OPTS += --shared -Wl,-gc-sections -Wl,-no-undefined

LIBS += -lutil $(shell ncursesw5-config --libs)

CC_OPTS += $(OPTIONS)
LD_OPTS += $(OPTIONS)

all: bin/libttymultiplex.so bin/libttymultiplex.a

%/.dir:
	mkdir -p "$(dir $@)"
	touch "$@"

docs:
	rm -rf doc
	export PROJECT_NUMBER="$$(git rev-parse HEAD ; git diff-index --quiet HEAD || echo '(with uncommitted changes)')"; \
	doxygen

build/%.o: src/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) $(CC_OPTS) -c -o $@ $<

bin/libttymultiplex.a: $(OBJS) | bin/.dir
	$(AR) scr $@ $^

bin/libttymultiplex.so: bin/libttymultiplex.a | bin/.dir
	$(CC) $(LD_OPTS) -Wl,--whole-archive $^ -Wl,--no-whole-archive $(LIBS) -o $@

cppcheck:
	cppcheck $(CPPCHECK_OPTIONS) $(SOURCES)

install:
	mkdir -p "$(DESTDIR)$(PREFIX)/lib"
	mkdir -p "$(DESTDIR)$(PREFIX)/include"
	cp bin/libttymultiplex.a "$(DESTDIR)$(PREFIX)/lib/libttymultiplex.a"
	cp bin/libttymultiplex.so "$(DESTDIR)$(PREFIX)/lib/libttymultiplex.so"
	cp include/libttymultiplex.h "$(DESTDIR)$(PREFIX)/include/"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/lib/libttymultiplex.a"
	rm -f "$(DESTDIR)$(PREFIX)/lib/libttymultiplex.so"
	rm -f "$(DESTDIR)$(PREFIX)/include/libttymultiplex.h"

clean:
	rm -rf bin/ build/
