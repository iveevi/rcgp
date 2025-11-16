#pragma once

#include <fmt/printf.h>

#include "../dsl/instructions.hpp"
#include "stage_intrinsics.hpp"
#include "static_string.hpp"

template <typename R>
struct _return_operator {};

template <typename T>
struct return_handler_t {
	static void main(const T &, size_t &argi) {
		fmt::println("generic return handler for {}, argi={}", 
	       		$ss_type(T).view(), argi);
	}
};

template <typename T, ThreadOutput::Properties P>
struct return_handler_t <Interpolant <T, P>> {
	static void main(const Interpolant <T, P> &interpolant, size_t &argi) {
		auto type = reconstruct_type <T> ();
		auto tout = ThreadOutput(type, argi, P);
		$tsb.context.add_thread_output(tout);

		// Fix the argument index of the original value
		auto &instr = *interpolant.ref;
		instr.template as <ThreadOutput> ()
			.argi = argi;

		argi++;
	}
};

template <typename ... Args>
struct return_handler_t <std::tuple <Args...>> {
	static void main(const std::tuple <Args...> &args, size_t &argi) {
		std::apply([&](auto ... xs) {
			std::make_tuple(return_handler(xs, argi)...);
		}, args);
	}
};

template <typename T>
bool return_handler(const T &v, size_t &argi)
{
	fmt::println("ftn for {}", $ss_type(T).view());
	return_handler_t <T> ::main(v, argi);
	return true;
}

template <typename R, typename U>
requires std::is_convertible_v <U, R>
void operator<<(const _return_operator <R>, U value)
{
	// Force conversion to get expected behavior; only
	// if necessary to avoid unexpected duplicate behaviour
	auto cvted = [&]() -> R {
		if constexpr (std::is_same_v <R, std::decay_t <U>>)
			return value;
		else
			return R(value);
	} ();

	size_t argi = 0;
	return_handler(cvted, argi);
}
