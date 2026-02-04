#pragma once

#include <functional>
#include <optional>

#include "aliases.hpp"

namespace rcgp {

using _loop_body = std::function <void ()>;
using _loop_cond = std::function <boolean ()>;

struct _while {
	_loop_cond cond;
	_loop_body body;
};

struct _for {
	_loop_cond cond;
	_loop_body step;
	_loop_body body;
};

struct _loop_holder {
	_loop_cond cond;
	std::optional <_loop_body> step;
	_loop_body body;

	_loop_holder() = default;

	_loop_holder(const _loop_holder &) = delete;
	_loop_holder &operator=(const _loop_holder &) = delete;

	_loop_holder(_loop_holder &&other) {
		cond = std::move(other.cond);
		step = std::move(other.step);
		body = std::move(other.body);

		other.step.reset();
	}

	~_loop_holder();
};

inline _loop_holder operator*(_while a, const auto &ftn)
{
	auto result = _loop_holder();
	result.cond = a.cond;
	result.body = _loop_body(ftn);
	return result;
}

inline _loop_holder operator*(_for a, const auto &ftn)
{
	auto result = _loop_holder();
	result.cond = a.cond;
	result.step = a.step;
	result.body = _loop_body(ftn);
	return result;
}

#define $while(cond)				\
	_while([&] { return (cond); }) * [&]

#define $for(init, cond, step)			\
	init;					\
	_for([&] { return (cond); }, [&] { (step); }) * [&]

} // namespace rcgp
