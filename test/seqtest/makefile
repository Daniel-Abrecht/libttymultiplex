SOURCES += src/main.c

include ../common.mk

all: bin

bin: bin-base

clean: clean-base

test: test-base
	echo

check-sequence: bin
	echo 'checking if some sequences are detected...'
	infocmp -s d -x -1 | sed -n 's/^\s\+\([^=,]\+\)=\?\(.*\),.*/\1 \2/p' | ( \
          res=0; \
	  while read cap seq; \
	  do \
	    [ -n "$$seq" ] || continue; \
	    printf '  %s: ' "$$cap"; \
	    if $(BIN) "$$(tput "$$cap" 1 2 3 4 5 6 7 8 9)" >/dev/null; \
	      then printf 'OK\n' "$$cap"; \
	      else printf 'NO\n' "$$cap"; res=1; \
	    fi; \
	  done; \
	  exit "$$res"; \
        )
