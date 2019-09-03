# Copyright (c) 2018 Daniel Abrecht
# SPDX-License-Identifier: AGPL-3.0-or-later

TESTS = $(dir $(wildcard */.test))

TS_BIN = test-summary/bin/test-summary

all: test

clean: $(addprefix clean-,$(TESTS)) clean-test-summary
build: $(addprefix bin/,$(TESTS))


test: $(TS_BIN)
	$(TS_BIN) partest $(MAKE) do-test

do-test:
	res=0; \
	for test in $(TESTS); \
	  do $(MAKE) "test-$$test" || res=1; \
	done; \
	exit "$$res"

$(TS_BIN): always
	$(MAKE) -C test-summary

bin/%: always
	$(MAKE) -C "$(patsubst bin/%,%,$@)"

build-%: bin/%

clean-%:
	$(MAKE) -C "$(patsubst clean-%,%,$@)" clean

test-%:
	$(MAKE) -C "$(patsubst test-%,%,$@)" test

always: ;
