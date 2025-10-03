#pragma once

#include <functional>

#include "ugp/static_string.hpp"
#include "ugp/stage_intrinsics.hpp"
#include "ugp/reflection.hpp"
#include "ugp/type_string_extensions.hpp"
#include "ugp/this_injection.hpp"
#include "ugp/type_hash.hpp"

// TODO: UGP namespace

// TODO: ray payloads as resources, not attributes... (ref checks should be easier)

template <typename T>
struct reflection_fetcher : std::false_type {
	using type = T;
};

template <reflection_holder T>
struct reflection_fetcher <T> : std::true_type {
	using type = typename T::reflection;
};

template <typename T>
constexpr bool has_reflection = reflection_fetcher <T> ::value;

// TODO: layout for parameter blocks is a wrapper layout on the inner type...
// TODO: layout engine type operator...
// TODO: native types have a singleton layout...

template <auto &resource>
struct resource_instance_t {
	using value_type = std::remove_reference_t <decltype(resource)>;

	value_type *operator->() {
		// TODO: need to return a proxy handle?
		// every jit variable/taggable is either actual code
		// or a reference to a slice of staging memory?
		// actual GPU memory isnt known until we interface with the pipeline
		return new value_type();
	}
};

// TODO: need to inject into the layout...
// deal with this at the same level as before (during jit scoping)

#define $import(name)	resource_reference_t <name> name

// TODO: aggregate_reflection_t flattening...

// TODO: filtration for function reflection:
// 1. linearize non-parameter block arguments (e.g. vertex attribute packets) in an order preserving way
// 2. hoist out parameter block resources

template <typename R, typename ... Args>
auto function_reflection_generator() -> function_reflection_t <
	typename reflection_fetcher <R> ::type,
	typename reflection_fetcher <std::decay_t <Args>> ::type ...
>;

// TODO: needs a layout...
template <typename T>
struct ParameterBlock : public T {
	// TODO: lock the members until its used in resource_reference_t
	using reflection = parameter_block_reflection_t <
		typename reflection_fetcher <T> ::type
	>;
};

template <auto &resource>
using resource_reference_base_t = std::remove_reference_t <decltype(resource)>;

template <auto &resource>
struct resource_reference_t : resource_reference_base_t <resource> {
	using value_type = resource_reference_base_t <resource>;

	static_assert(has_reflection <value_type>,
	       // TODO: string only format style...
	       ($ss_type(value_type) + $ss(
	       " has no valid reflection, perhaps you forgot to"
	       " add it to the registry via $reflection(...)?")).view());

	using reflection = referencing_reflection_t <
		resource,
		typename value_type::reflection
	>;
};
// TODO: for comparisons just use std::same_as...

template <typename R>
struct _return_operator {};

#define $return _returner <<

template <typename R, typename U>
void operator<<(_return_operator <R>, const U &value)
{
	static_assert(std::is_convertible_v <U, R>);
	// _return(value);
}

// TODO: pass name explicitly in the decl case, otherwise generate unique ID
template <typename R>
struct _def_operator {};

#define $def(R)		_def_operator <R> () << [_returner = _return_operator <R> ()]
#define $scoped_def(R)	_def_operator <R> () << [&, _returner = _return_operator <R> ()]

template <typename R, typename ... Args>
// struct function_t : thunder::TrackedBuffer {
struct function_t {
	using reflection = decltype(function_reflection_generator <R, Args...> ());
};

template <typename R, typename ... Args>
struct signature_t {
	using args = std::tuple <std::decay_t <Args> ...>;
	using returns = R;
	using function = function_t <R, Args...>;

	template <typename Rt>
	using with_return = signature_t <Rt, Args...>;
};

template <typename R, typename ... Args>
constexpr auto signature_generator(std::function <R (Args...)> &&) -> signature_t <R, Args...>;

// TODO: custom vertex assembler stage

template <typename R, typename F>
auto operator<<(_def_operator <R>, F ftn)
{
	using function = decltype(std::function(ftn));
	using signature = decltype(signature_generator(function()));
	static_assert(std::same_as <typename signature::returns, void>);

	using fixed = typename signature::template with_return <R>;
	// TODO: use classified = ...

	// TODO: classify the function as a vertex/fragment/... stage or ordinary subroutine

	typename fixed::function result;

	// auto &em = Emitter::active;

	// em.push(result);
	{
		typename fixed::args args;

		// em.display_assembly();
		
		// TODO: generate plceholders during injection, after first filtration

		// TODO: injection
		std::apply(ftn, args);
	}
	// em.pop();

	return result;
}
