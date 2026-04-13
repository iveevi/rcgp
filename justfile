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
experimental: build_rcgp
	cmake --build build -j -t experimental
	./build/experimental

# Compilation tests (static assertions, type checks, expected errors)
compile:
	{{compiler}} {{cxxflags}} -c tests/compile/std430.cpp -o /tmp/std430.o
	{{compiler}} {{cxxflags}} -c tests/compile/scalar.cpp -o /tmp/scalar.o
	{{compiler}} {{cxxflags}} -c tests/compile/resources.cpp -o /tmp/resources.o
	{{compiler}} {{cxxflags}} -c tests/compile/canonical.cpp -o /tmp/canonical.o
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/attribute_stream.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/intrinsics.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/meshlet_payload.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/return_types.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/task_payload.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/traced_params.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/subroutine_call.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/workgroup.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/rasterization_combinator.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/compute_combinator.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/mesh_shading_combinator.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/shader_modules/raytracing_combinator.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/command_modules/rasterization.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/command_modules/compute.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/command_modules/mesh_shading.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/command_modules/raytracing.cpp
	python tests/compile/check_compile_fail.py {{compiler}} {{cxxflags}} tests/compile/command_modules/deferred.cpp

# Tests for JIT tracing of the DSL
dsl *args: build_test
	./build/test {{args}}

# Run all tests
test: compile dsl
