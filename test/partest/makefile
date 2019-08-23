SOURCES += src/main.c

TESTS = $(patsubst check/%.sh,%,$(wildcard check/*.sh))

include ../common.mk

all: bin
bin: bin-base
test: $(addprefix test-,$(TESTS))
clean: $(addprefix clean-,$(TESTS)) clean-base

clean-%:
	rm -rf "$(patsubst clean-%,check/%.result/,$@)"

test-%: check/%.sh test-base
	@ \
	name="$(patsubst test-%,%,$@)"; \
	base="check/$$name"; \
	rm -rf "$$base.result/"; \
	mkdir -p "$$base.result/"; \
	"$<" | "$(BIN)" "$$base.result/"; \
	diff "$$base.result" "$$base.expected" >/dev/null 2>/dev/null; \
	result=$$?; \
	printf '%s: ' "$@"; \
	if [ "$$result" = "0" ]; \
	  then echo OK; exit 0; \
	  else echo Failed; exit 1; \
	fi
