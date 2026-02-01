compiler := "clang++"
cxxflags := "-std=c++26 -I include"

# Build RCGP code library
rcgp:
	cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug .
	cmake --build build -j

# Build runtime tests
test:
	cmake --build build -j -t test

# Tests for scaffold generation
scaffold:
	{{compiler}} {{cxxflags}} -c tests/scaffold/std430.cpp -o /tmp/std430.o
	{{compiler}} {{cxxflags}} -c tests/scaffold/scalar.cpp -o /tmp/scalar.o

# Tests for JIT tracing of the DSL
dsl *args: test
	./build/test {{args}}
