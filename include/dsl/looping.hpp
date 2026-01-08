#pragma once

#include <functional>
#include <optional>

#include "scalar.hpp"
#include "instructions.hpp"

using _loop_body = std::function <void ()>;
using _loop_cond = std::function <boolean ()>;

struct _while {
	_loop_cond cond;
	_loop_body body;
};

struct _for {
	_loop_body init;
	_loop_cond cond;
	_loop_body step;
	_loop_body body;
};

struct _for_noinit {
	_loop_cond cond;
	_loop_body step;
	_loop_body body;
};

struct _loop_holder {
	LoopKind kind = LoopKind::eWhile;
	std::optional <_loop_body> init;
	_loop_cond cond;
	std::optional <_loop_body> step;
	_loop_body body;

	_loop_holder() = default;

	_loop_holder(const _loop_holder &) = delete;
	_loop_holder &operator=(const _loop_holder &) = delete;

	_loop_holder(_loop_holder &&other) {
		kind = other.kind;
		init = std::move(other.init);
		cond = std::move(other.cond);
		step = std::move(other.step);
		body = std::move(other.body);

		other.init.reset();
		other.step.reset();
	}

	~_loop_holder();
};

inline _loop_holder operator*(_while a, const auto &ftn)
{
	auto result = _loop_holder();
	result.kind = LoopKind::eWhile;
	result.cond = a.cond;
	result.body = _loop_body(ftn);
	return result;
}

inline _loop_holder operator*(_for a, const auto &ftn)
{
	auto result = _loop_holder();
	result.kind = LoopKind::eFor;
	result.init = a.init;
	result.cond = a.cond;
	result.step = a.step;
	result.body = _loop_body(ftn);
	return result;
}

inline _loop_holder operator*(_for_noinit a, const auto &ftn)
{
	auto result = _loop_holder();
	result.kind = LoopKind::eFor;
	result.cond = a.cond;
	result.step = a.step;
	result.body = _loop_body(ftn);
	return result;
}

#define $while(cond)	_while([&] { return (cond); }) * [&]
#define $for(init, cond, step) if (bool __for_once = true) \
	for (init; __for_once; __for_once = false) \
		_for_noinit([&] { return (cond); }, [&] { step; }) * [&]
