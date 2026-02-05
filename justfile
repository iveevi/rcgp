compiler := "clang++"
cxxflags := "-std=c++26 -I include"

# Build RCGP code library
build_rcgp:
	cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug .
	cmake --build build -j

# Build runtime tests
build_test: build_rcgp
	cmake --build build -j -t test

# Experimental program
exp: build_rcgp
	cmake --build build -j -t experimental

# Tests for scaffold generation
scaffold:
	{{compiler}} {{cxxflags}} -c tests/scaffold/std430.cpp -o /tmp/std430.o
	{{compiler}} {{cxxflags}} -c tests/scaffold/scalar.cpp -o /tmp/scalar.o
	{{compiler}} {{cxxflags}} -c tests/scaffold/resources.cpp -o /tmp/resources.o

# Tests for JIT tracing of the DSL
dsl *args: build_test
	./build/test {{args}}

# Run all tests
test: scaffold dsl
