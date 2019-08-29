SOURCES += src/main.c

TESTS = $(patsubst check/%.sh,%,$(wildcard check/*.sh))

all: bin

include ../common.mk

bin: bin-base
clean: $(addprefix clean-,$(TESTS)) clean-base

do-test:
	res=0; \
	for test in $(TESTS); \
	  do $(MAKE) "test-$$test" || res=1; \
	done; \
	exit "$$res"


clean-%:
	rm -rf "$(patsubst clean-%,check/%.result/,$@)"

test-%: check/%.sh test-base
	name="$(patsubst test-%,%,$@)"; \
	base="check/$$name"; \
	rm -rf "$$base.result/"; \
	mkdir -p "$$base.result/"; \
	"$<" | "$(BIN)" "$$base.result/"; \
	test-exec "$$name" diff "$$base.result" "$$base.expected" >/dev/null 2>/dev/null; \
	result=$$?; \
	printf '%s: ' "$@"; \
	if [ "$$result" = "0" ]; \
	  then echo OK; exit 0; \
	  else echo Failed; exit 1; \
	fi
