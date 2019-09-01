SOURCES += src/main.c

all: bin

include ../common.mk

bin: bin-base
clean: clean-base
test: test-base

do-test: bin

test-terminfo-%: $(TS_BIN)
	term="$(patsubst test-terminfo-%,%,$@)"; \
	test-summary "terminfo-$$term" $(MAKE) "do-test-terminfo-$$term"

do-test-terminfo-%: bin
	echo 'checking if some sequences are detected...'
	export TERM="$(patsubst do-test-terminfo-%,%,$@)"; \
	infocmp -s d -x -1 | sed -n 's/^\s\+\([^=,]\+\)=\?\(.*\),.*/\1 \2/p' | ( \
          res=0; \
	  while read cap seq; \
	  do \
	    case "$$cap" in \
	      k*) continue ;; \
	      '') continue ;; \
	    esac; \
	    [ -n "$$seq" ] || continue; \
	    [ -n "$$seq" ] || continue; \
	    printf '  %s: ' "$$cap"; \
	    if test-exec "$$cap" "$(BIN)" "$$(tput "$$cap" 1 2 3 4 5 6 7 8 9)" >/dev/null; \
	      then printf 'OK\n' "$$cap"; \
	      else printf 'Failed\n' "$$cap"; res=1; \
	    fi; \
	  done; \
	  exit "$$res"; \
        )
