
BACKEND_SOURCES += src/main.c

INCLUDES = $(shell ncursesw5-config --cflags)

include src/common.mk

all: build/backend/curses.a

build/backend/curses.a: $(OBJS) | build/.dir
	$(AR) scrT $@ $^
