# Copyright (c) 2018 Daniel Abrecht
# SPDX-License-Identifier: AGPL-3.0-or-later

VERSION := $(shell cat version)
MAJOR   := $(word 1,$(subst ., ,$(VERSION)))
MINOR   := $(word 2,$(subst ., ,$(VERSION)))
PATCH   := $(word 3,$(subst ., ,$(VERSION)))

HEADERS += $(wildcard include/*.h) $(wildcard include/**/*.h)

PREFIX = /usr
BACKEND_DIR = $(PREFIX)/lib/libttymultiplex/backend-$(VERSION)

INCLUDES += -Iinclude
DEFAULT_INCLUDES += $(shell $(CC) -Wp,-v -x c++ -fsyntax-only /dev/null 2>&1 | sed -n '/search starts here/,/End of search list/p' | grep '^ ')

CPPCHECK_OPTIONS += --enable=all
CPPCHECK_OPTIONS += $(INCLUDES) $(addprefix -I,$(DEFAULT_INCLUDES)) --std=c99 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE -DTYM_BUILD
CPPCHECK_OPTIONS += --std=c99 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE -DTYM_BUILD
CPPCHECK_OPTIONS += $(CPPCHECK_OPTS)

ifdef DEBUG
CC_OPTS += -Og -g
endif

ifndef LENIENT
CC_OPTS += -Werror
endif

CC_OPTS += -fPIC -pthread -ffunction-sections -fdata-sections -fstack-protector-all
CC_OPTS += -DTYM_BUILD -finput-charset=UTF-8
CC_OPTS += $(INCLUDES)
CC_OPTS += -std=c99 -Wall -Wextra -pedantic -Wno-implicit-fallthrough
CC_OPTS += -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE
CC_OPTS += -DTYM_PREFIX='"$(PREFIX)"'
CC_OPTS += -DTYM_VERSION='"$(VERSION)"' -DTYM_MAJOR='$(MAJOR)' -DTYM_MINOR='$(MINOR)' -DTYM_PATCH='$(PATCH)'
CC_OPTS += -DTYM_BACKEND_DIR='"$(BACKEND_DIR)"'

LD_OPTS += --shared -Wl,-gc-sections -Wl,-no-undefined -pthread

ifdef BACKEND

HEADERS += $(wildcard backend/$(BACKEND)/include/*.h) $(wildcard backend/$(BACKEND)/include/**/*.h)
INCLUDES += -Ibackend/$(BACKEND)/include
CC_OPTS += -fvisibility=hidden
CC_OPTS += -DTYM_LOG_PROJECT='"libttymultiplex-backend-$(BACKEND)"'
CC_OPTS += -DTYM_I_BACKEND_NAME='"$(BACKEND)"'
CC_OPTS += -DTYM_I_BACKEND_ID="B_$$(printf "$(BACKEND)" | sed 's/[^a-zA-Z0-9]/_/g')"
BACKEND_PRIORITY = $(shell \
  priority=50; \
  if [ -f "backend/$(BACKEND)/priority" ] && grep -q '^[0-9][0-9]$$' "backend/$(BACKEND)/priority" && [ $$(wc -l "backend/$(BACKEND)/priority" | grep -o '^[0-9]*') = 1 ];  \
    then priority=$$(cat "backend/$(BACKEND)/priority"); \
  fi; \
  echo "$$priority" \
)
CC_OPTS += -DTYM_I_BACKEND_PRIORITY=$(BACKEND_PRIORITY)

SOURCES += $(addprefix backend/$(BACKEND)/,$(BACKEND_SOURCES))
OBJS += $(patsubst backend/$(BACKEND)/src/%.c,build/backend/$(BACKEND)/%.c.o,$(SOURCES))

build/backend/$(BACKEND)/%.c.o: backend/$(BACKEND)/src/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CFLAGS) $(CPPFLAGS) "$<"

else

CC_OPTS += -DTYM_LOG_PROJECT='"libttymultiplex"'
OBJS += $(patsubst src/%.c,build/%.c.o,$(SOURCES))

build/%.c.o: src/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) -c -o "$@" $(CC_OPTS) $(CFLAGS) $(CPPFLAGS) "$<"

endif


%/.dir:
	mkdir -p "$(dir $@)"
	touch "$@"

.PRECIOUS: %/.dir build/backend/%.a
