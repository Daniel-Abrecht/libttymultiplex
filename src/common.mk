
HEADERS += $(wildcard include/*.h) $(wildcard include/**/*.h)

PREFIX = /usr

CC = gcc
AR = ar

INCLUDES += -Iinclude
DEFAULT_INCLUDES += $(shell $(CC) -Wp,-v -x c++ -fsyntax-only /dev/null 2>&1 | sed -n '/search starts here/,/End of search list/p' | grep '^ ')

CPPCHECK_OPTIONS += --enable=all
CPPCHECK_OPTIONS += $(INCLUDES) $(addprefix -I,$(DEFAULT_INCLUDES)) --std=c99 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE -DTYM_BUILD
CPPCHECK_OPTIONS += --std=c99 -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE -DTYM_BUILD
CPPCHECK_OPTIONS += $(CPPCHECK_OPTS)

OPTIONS += -fPIC -pthread -ffunction-sections -fdata-sections -fstack-protector-all -g -Og
CC_OPTS += -DTYM_BUILD -finput-charset=UTF-8
CC_OPTS += $(INCLUDES)
CC_OPTS += -std=c99 -Wall -Wextra -pedantic -Werror -Wno-implicit-fallthrough
CC_OPTS += -D_POSIX_C_SOURCE -D_DEFAULT_SOURCE
CC_OPTS += -DPREFIX='"$(PREFIX)"'
LD_OPTS += --shared -Wl,-gc-sections -Wl,-no-undefined

CC_OPTS += $(OPTIONS)
LD_OPTS += $(OPTIONS)

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
OBJS += $(patsubst backend/$(BACKEND)/src/%.c,build/backend/$(BACKEND)/%.o,$(SOURCES))

build/backend/$(BACKEND)/%.o: backend/$(BACKEND)/src/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) $(CC_OPTS) -c -o $@ $<

else

CC_OPTS += -DTYM_LOG_PROJECT='"libttymultiplex"'
OBJS += $(patsubst src/%.c,build/%.o,$(SOURCES))

build/%.o: src/%.c $(HEADERS)
	mkdir -p "$(dir $@)"
	$(CC) $(CC_OPTS) -c -o $@ $<

endif


%/.dir:
	mkdir -p "$(dir $@)"
	touch "$@"

.PRECIOUS: %/.dir build/backend/%.a
