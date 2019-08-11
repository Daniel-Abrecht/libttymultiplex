# Copyright (c) 2018 Daniel Abrecht
# SPDX-License-Identifier: AGPL-3.0-or-later

SOURCES += $(wildcard src/sequencehandler/*.c)
SOURCES += src/libttymultiplex.c
SOURCES += src/main.c
SOURCES += src/pane.c
SOURCES += src/pane_flag.c
SOURCES += src/calc.c
SOURCES += src/list.c
SOURCES += src/pseudoterminal.c
SOURCES += src/utils.c
SOURCES += src/parser.c
SOURCES += src/charset.c
SOURCES += src/utf8.c
SOURCES += src/backend.c
SOURCES += src/backend_default_procs.c
SOURCES += src/terminfo_helper.c

EXTERNAL_BACKENDS += curses
BUILTIN_BACKENDS +=

LIBS = -lutil -ldl

FILES_WITH_LIB_VERSION += debian/control
FILES_WITH_LIB_VERSION += $(wildcard debian/libttymultiplex*.*)


include src/common.mk

all: bin/libttymultiplex.so $(patsubst %,bin/backend/%.so,$(EXTERNAL_BACKENDS))

backend-%: bin/backend/%.so

clean-backend:
	rm -rf "build/backend/"

clean-backend-%:
	rm -rf "$(patsubst clean-backend-%,build/backend/%/,$@)"
	rm -f "$(patsubst clean-backend-%,bin/backend/%.so,$@)"

always:

docs:
	rm -rf bin/doc
	export PROJECT_NUMBER="v$$version $$(git rev-parse HEAD ; git diff-index --quiet HEAD || echo '(with uncommitted changes)')"; \
	doxygen doxygen/Doxyfile

release-major: release@$(shell expr $(MAJOR) + 1).0.0

release-minor: release@$(MAJOR).$(shell expr $(MINOR) + 1).0

release-patch: release@$(MAJOR).$(MINOR).$(shell expr $(PATCH) + 1)

release@%:
	set -x; \
	git clean -fdX debian/; \
	version="$(patsubst release@%,%,$@)"; \
	major="$(word 1,$(subst ., ,$(patsubst release@%,%,$@)))"; \
	minor="$(word 2,$(subst ., ,$(patsubst release@%,%,$@)))"; \
	patch="$(word 3,$(subst ., ,$(patsubst release@%,%,$@)))"; \
	dch -v "$$version"; \
	for file in $(FILES_WITH_LIB_VERSION); \
	do \
	  for prefix in libttymultiplex libttymultiplex.so.; \
	    do sed -i "s|\($$prefix\)\([0-9]\+\.[0-9]\+.[0-9]\+\)|\1$$major.$$minor.$$patch|g;s|\($$prefix\)\([0-9]\+\.[0-9]\+\)|\1$$major.$$minor|g;s|\($$prefix\)\([0-9]\+\)|\1$$major|g" "$$file"; \
	  done; \
	  for prefix in libttymultiplex libttymultiplex.so.; \
	  do \
	    if printf '%s\n' "$$file" | grep -q "$$prefix[0-9]\+"; \
	      then mv "$$file" "$$(printf '%s\n' "$$file" | sed "s|\($$prefix\)\([0-9]\+\.[0-9]\+.[0-9]\+\)|\1$$major.$$minor.$$patch|g;s|\($$prefix\)\([0-9]\+\.[0-9]\+\)|\1$$major.$$minor|g;s|\($$prefix\)\([0-9]\+\)|\1$$major|g")"; \
	    fi; \
	  done; \
	done; \
	echo "$$version" > version; \
	git add debian/; \
	git commit -m "New release v$$version"; \
	git tag "v$$version"

build/backend/%.a: backend/%/backend.mk always build/backend/.dir
	$(MAKE) -f "$<" BACKEND="$(patsubst build/backend/%.a,%,$@)" "$@"

bin/backend/%.so: build/backend/%.a | bin/backend/.dir
	options_file="$(patsubst bin/backend/%.so,backend/%/options,$@)"; \
	if [ -f "$$options_file" ]; \
	  then . "$$options_file"; \
	fi; \
	libs="-Lbin/ -lttymultiplex $$libs"; \
	ld_opts="$(LD_OPTS) $$ld_opts"; \
	$(CC) $$ld_opts -Wl,--whole-archive $^ -Wl,--no-whole-archive $$libs -o $@;

build/libttymultiplex.a: $(OBJS) $(patsubst %,build/backend/%.a,$(BUILTIN_BACKENDS)) | build/.dir
	$(AR) scrT $@ $^

bin/libttymultiplex.so: build/libttymultiplex.a | bin/.dir
	libs="$(LIBS) \
	$$(for backend in $(BUILTIN_BACKENDS); \
	do (\
	  options_file="backend/$$backend/options"; \
	  if ! [ -f "$$options_file" ]; \
	    then continue; \
	  fi; \
	  . "$$options_file"; \
	  printf "%s\n" $$libs; \
	) done)"; \
	$(CC) $(LD_OPTS) -Wl,--whole-archive $^ -Wl,--no-whole-archive $$libs -o $@.$(MAJOR).$(MINOR).$(PATCH) -Wl,-soname,libttymultiplex.so.$(MAJOR)
	cd bin; \
	ln -sf "libttymultiplex.so.$(MAJOR).$(MINOR).$(PATCH)" "libttymultiplex.so.$(MAJOR).$(MINOR)"; \
	ln -sf "libttymultiplex.so.$(MAJOR).$(MINOR).$(PATCH)" "libttymultiplex.so.$(MAJOR)"; \
	ln -sf "libttymultiplex.so.$(MAJOR).$(MINOR).$(PATCH)" "libttymultiplex.so";

cppcheck:
	cppcheck $(CPPCHECK_OPTIONS) $(SOURCES)

install-backend-%: bin/backend/%.so
	mkdir -p "$(DESTDIR)/$(BACKEND_DIR)"
	backend="$(patsubst install-backend-%,%,$@)"; \
	priority=50; \
	if [ -f "backend/$$backend/priority" ] && grep -q '^[0-9][0-9]$$' "backend/$$backend/priority" && [ $$(wc -l "backend/$$backend/priority" | grep -o '^[0-9]*') = 1 ];  \
	  then priority=$$(cat "backend/$$backend/priority"); \
	fi; \
	cp $^ "$(DESTDIR)/$(BACKEND_DIR)/$$priority-$$backend.so"

install-backends: $(patsubst %,install-backend-%,$(EXTERNAL_BACKENDS))

install: install-lib install-header install-backends

install-lib: bin/libttymultiplex.so
	mkdir -p "$(DESTDIR)$(PREFIX)/lib/"
	cp "bin/libttymultiplex.so.$(MAJOR).$(MINOR).$(PATCH)" "$(DESTDIR)$(PREFIX)/lib/libttymultiplex.so.$(MAJOR).$(MINOR).$(PATCH)"
	cd "$(DESTDIR)$(PREFIX)/lib/"; \
	ln -sf "libttymultiplex.so.$(MAJOR).$(MINOR).$(PATCH)" "libttymultiplex.so.$(MAJOR).$(MINOR)"; \
	ln -sf "libttymultiplex.so.$(MAJOR).$(MINOR).$(PATCH)" "libttymultiplex.so.$(MAJOR)"; \
	ln -sf "libttymultiplex.so.$(MAJOR).$(MINOR).$(PATCH)" "libttymultiplex.so";

install-header: include/libttymultiplex.h
	mkdir -p "$(DESTDIR)$(PREFIX)/include/"
	cp include/libttymultiplex.h "$(DESTDIR)$(PREFIX)/include/"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/lib/libttymultiplex.so*"
	rm -f "$(DESTDIR)$(PREFIX)/include/libttymultiplex.h"

clean:
	rm -rf bin/ build/
	git clean -fdX debian/ || true

.PHONY: all always clean install install-lib install-header uninstall install-backend-% cppcheck
