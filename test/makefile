TESTS = $(dir $(wildcard */makefile))

all: test

clean: $(addprefix clean-,$(TESTS)) 
build: $(addprefix build-,$(TESTS)) 
test: $(addprefix test-,$(TESTS)) 

build-%:
	$(MAKE) -C "$(patsubst build-%,%,$@)"

clean-%:
	$(MAKE) -C "$(patsubst clean-%,%,$@)" clean

test-%:
	$(MAKE) -C "$(patsubst test-%,%,$@)" test
