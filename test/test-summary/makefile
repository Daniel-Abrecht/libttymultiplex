
all: bin/test-summary

bin/test-summary: build/main.o build/summary.o
	mkdir -p "$(dir $@)"
	$(CXX) -o "$@" $^

build/%.o: src/%.c
	mkdir -p "$(dir $@)"
	$(CC)  -c -o "$@" -std=c99 -Wall -Wextra -pedantic -Werror -Iinclude -D_DEFAULT_SOURCE "$<"

build/%.o: src/%.cpp
	mkdir -p "$(dir $@)"
	$(CXX)  -c -o "$@" -std=c++98 -Wall -Wextra -pedantic -Werror -Iinclude -D_DEFAULT_SOURCE "$<"

clean:
	rm -rf build
