#pragma once

#include <cstdlib>
#include <string_view>

template <size_t N>
struct static_string {
	static constexpr size_t length = N;

	char elements[N];

	constexpr static_string() = default;

	constexpr static_string(const char (&str)[N + 1]) {
		for (size_t i = 0; i < N; i++)
			elements[i] = str[i];
	}

	constexpr static_string(char c) {
		for (size_t i = 0; i < N; i++)
			elements[i] = c;
	}

	constexpr std::string_view view() const {
		return { elements, N };
	}
};

#define $ss(str) static_string <sizeof(str) - 1> (str)

template <size_t N, size_t M>
constexpr static_string <N + M> operator+(static_string <N> A, static_string <M> B)
{
	static_string <N + M> result;
	for (size_t i = 0; i < N; i++)
		result.elements[i] = A.elements[i];
	for (size_t i = 0; i < M; i++)
		result.elements[i + N] = B.elements[i];
	return result;
}

template <typename T>
consteval std::string_view scribbled_name_view()
{
#if defined(__clang__)
	std::string_view f = __PRETTY_FUNCTION__;
	auto key = std::string_view { "[T = " };
	auto start = f.find(key);
	start = (start == std::string_view::npos) ? 0 : start + key.size();
	auto end = f.rfind(']');
	return f.substr(start, end - start);
#elif defined(__GNUC__)
	std::string_view f = __PRETTY_FUNCTION__;
	auto key = std::string_view { "with T = " };
	auto start = f.find(key);
	start = (start == std::string_view::npos) ? 0 : start + key.size();
	auto end = f.find(';', start);
	return f.substr(start, end - start);
#elif defined(_MSC_VER)
	std::string_view f = __FUNCSIG__;
	auto key = std::string_view { "pretty_name_view<" };
	auto start = f.find(key);
	start = (start == std::string_view::npos) ? 0 : start + key.size();
	auto end = f.find(">(void)", start);
	return f.substr(start, end - start);
#else
#error Unsupported compiler
#endif
}

template <typename T>
struct type_string {
	template <size_t I>
	static consteval auto eval() {
		constexpr auto v = scribbled_name_view <T> ();
		auto result = static_string <v.size()> ();
		for (size_t i = 0; i < v.size(); i++)
			result.elements[i] = v[i];

		return result;
	}
};

#define $ss_type(T)		type_string <T> ::template eval <0> ()
#define $ss_type_indented(T, N)	type_string <T> ::template eval <N> ()

consteval size_t ndigits_decimal(size_t x)
{
	size_t count = 1;
	while (x >= 10) {
		x /= 10;
		count++;
	}

	return count;
}

consteval size_t ndigits_hex(size_t x)
{
	if (x == 0) return 1;
	size_t count = 0;
	while (x > 0) {
		x >>= 4;
		count++;
	}
	return count;
}

template <size_t N>
consteval auto ss_from_ulong_decimal()
{
	constexpr size_t L = ndigits_decimal(N);

	static_string <L> result;

	size_t i = L;
	size_t x = N;

	do {
		result.elements[--i] = char('0' + (x % 10));
		x /= 10;
	} while (x);

	return result;
}

template <size_t N>
consteval auto ss_from_ulong_hex()
{
	constexpr size_t L = ndigits_hex(N) + 2;

	static_string <L> result;

	result.elements[0] = '0';
	result.elements[1] = 'x';

	size_t i = L;
	size_t x = N;

	do {
		size_t digit = x & 0xF;
		result.elements[--i] = (digit < 10) ? char('0' + digit) : char('a' + digit - 10);
		x >>= 4;
	} while (x);

	return result;
}

#define $ss_ulong_decimal(N)	ss_from_ulong_decimal <N> ()
#define $ss_ulong_hex(N)	ss_from_ulong_hex <N> ()

template <static_string str>
consteval bool is_insertion_point(size_t i)
{
	if (str.elements[i] != '{')
		return false;

	if (i + 1 >= str.length || str.elements[i + 1] != '}')
		return false;

	if (i >= 1 && str.elements[i - 1] == '{')
		return false;

	return true;
}

template <static_string str>
consteval size_t insertions()
{
	size_t count = 0;
	for (size_t i = 0; i < str.length; i++) {
		if (is_insertion_point <str> (i))
			count++;
	}

	return count;
}

template <static_string format, static_string... args>
consteval auto static_string_format()
{
	constexpr std::size_t ins = insertions <format> ();

	static_assert(ins == sizeof...(args), "arity mismatch");

	constexpr std::size_t args_sum = (args.length + ... + 0);
	constexpr std::size_t total = (format.length - 2 * ins) + args_sum;

	static_string <total> out;

	constexpr auto av = std::array <
		std::string_view,
		sizeof...(args)
	> { args.view()... };

	std::size_t oi = 0;
	std::size_t argi = 0;

	for (std::size_t i = 0; i < format.length;) {
		if (is_insertion_point <format> (i)) {
			const auto sv = av[argi++];
			for (char c : sv)
				out.elements[oi++] = c;
			i += 2;
		} else {
			out.elements[oi++] = format.elements[i++];
		}
	}
	return out;
}

#define $ss_format(fmt, ...)	static_string_format <fmt __VA_OPT__(,) __VA_ARGS__> ()
