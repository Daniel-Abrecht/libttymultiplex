
BACKEND_SOURCES += src/main.c

INCLUDES = $(shell pkg-config --cflags ncursesw)

include src/common.mk

all: build/backend/curses.a

build/backend/curses.a: $(OBJS) | build/.dir
	$(AR) scrT $@ $^
