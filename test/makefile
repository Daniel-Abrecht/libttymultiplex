TESTS = $(dir $(wildcard */makefile))

all: test

clean: $(addprefix clean-,$(TESTS))
build: $(addprefix bin/,$(TESTS))
test: $(addprefix test-,$(TESTS))

bin/%: always
	$(MAKE) -C "$(patsubst bin/%,%,$@)"

build-%: bin/%

clean-%:
	$(MAKE) -C "$(patsubst clean-%,%,$@)" clean

test-%:
	$(MAKE) -C "$(patsubst test-%,%,$@)" test

always: ;
