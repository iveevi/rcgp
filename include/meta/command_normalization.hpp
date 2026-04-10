#pragma once

#include <type_traits>
#include <utility>

#include "../util/cti.hpp"
#include "../util/tlist.hpp"
#include "command_effects.hpp"
#include "static_string.hpp"

namespace rcgp {

// Dependency/resolution state for a key
struct tag_dep {};
struct tag_res {};
struct tag_none {};

template <typename Src, typename Dst>
struct tag_bar {
	using src = Src;
	using dst = Dst;
};

struct no_bar {};

template <auto &ref>
struct key_ref {};

template <auto &ref>
struct key_target {};

template <Topology T>
struct key_index {};

// Summary entry for a single key (resource or index buffer)
template <typename Key, typename DepRes, typename Bar>
struct effect_entry {
	using key = Key;
	using dep = DepRes;
	using bar = Bar;
};

TYPE_TRAIT(is_dependency_effect);
TYPE_TRAIT(is_resolvant_effect);
TYPE_TRAIT(is_barrier_effect);
TYPE_TRAIT(is_resolvant_index_effect);
TYPE_TRAIT(is_dependency_index_effect);
TYPE_TRAIT(is_indicator_index_effect);
TYPE_TRAIT(is_enforcer_index_effect);
TYPE_TRAIT(is_dependency_sentinel_effect);

TYPE_TRAIT(is_target_write_effect);
TYPE_TRAIT(is_target_read_effect);

template <auto &ref>
TYPE_TRAIT_INCLUDES(is_dependency_effect, Dependency <ref>);

template <auto &ref>
TYPE_TRAIT_INCLUDES(is_resolvant_effect, Resolvant <ref>);

template <auto &ref, typename Src, typename Dst>
TYPE_TRAIT_INCLUDES(is_barrier_effect, BarrierEffect <ref, Src, Dst>);

template <Topology T>
TYPE_TRAIT_INCLUDES(is_resolvant_index_effect, ResolvantForIndexBuffer <T>);

template <Topology T>
TYPE_TRAIT_INCLUDES(is_dependency_index_effect, DependencyForIndexBuffer <T>);

template <Topology T>
TYPE_TRAIT_INCLUDES(is_indicator_index_effect, DependencyIndicatorForIndexBuffer <T>);

template <>
TYPE_TRAIT_INCLUDES(is_enforcer_index_effect, DependencyEnforcerForIndexBuffer);

template <>
TYPE_TRAIT_INCLUDES(is_dependency_sentinel_effect, DependencySentinel);

template <auto &ref>
TYPE_TRAIT_INCLUDES(is_target_write_effect, TargetWrite <ref>);

template <auto &ref>
TYPE_TRAIT_INCLUDES(is_target_read_effect, TargetRead <ref>);

template <typename Head, typename ... Tail>
consteval auto tlist_prepend(Tlist <Tail...>, Head) -> Tlist <Head, Tail...>;

template <typename Head, typename List>
using tlist_prepend_t = decltype(tlist_prepend(List(), Head()));

// Combine dependency/resolution state
consteval auto tag_operator(tag_none, tag_none) -> tag_none;
consteval auto tag_operator(tag_none, tag_dep) -> tag_dep;
consteval auto tag_operator(tag_none, tag_res) -> tag_res;
consteval auto tag_operator(tag_dep, tag_none) -> tag_dep;
consteval auto tag_operator(tag_res, tag_none) -> tag_res;
consteval auto tag_operator(tag_dep, tag_dep) -> tag_dep;
consteval auto tag_operator(tag_res, tag_res) -> tag_res;
consteval auto tag_operator(tag_dep, tag_res) -> tag_none;
consteval auto tag_operator(tag_res, tag_dep) -> tag_none;

// Combine barrier phase transitions for a key
consteval auto tag_operator(no_bar, no_bar) -> no_bar;

template <typename Src, typename Dst>
consteval auto tag_operator(no_bar, tag_bar <Src, Dst>) -> tag_bar <Src, Dst>;

template <typename Src, typename Dst>
consteval auto tag_operator(tag_bar <Src, Dst>, no_bar) -> tag_bar <Src, Dst>;

template <typename SrcA, typename DstA, typename SrcB, typename DstB>
consteval auto tag_operator(tag_bar <SrcA, DstA>, tag_bar <SrcB, DstB>)
{
	if constexpr (std::is_same_v <DstA, SrcB>) {
		return tag_bar <SrcA, DstB> ();
	} else if constexpr (std::is_same_v <DstB, SrcA>) {
		return tag_bar <SrcB, DstA> ();
	} else {
		return no_bar();
	}
}

template <typename A, typename B>
using tag_operator_t = decltype(tag_operator(A(), B()));

// Update or remove a key entry in the summary map
template <typename Key, typename DepRes, typename Bar>
consteval auto map_apply(Key, DepRes, Bar, Tlist <>)
{
	if constexpr (std::is_same_v <DepRes, tag_none> and std::is_same_v <Bar, no_bar>) {
		return Tlist <> ();
	} else {
		return Tlist <effect_entry <Key, DepRes, Bar>> ();
	}
}

template <typename Key, typename DepRes, typename Bar, typename Head, typename ... Tail>
consteval auto map_apply(Key, DepRes, Bar, Tlist <Head, Tail...>)
{
	if constexpr (std::is_same_v <typename Head::key, Key>) {
		using dep = tag_operator_t <typename Head::dep, DepRes>;
		using bar = tag_operator_t <typename Head::bar, Bar>;
		if constexpr (std::is_same_v <dep, tag_none> and std::is_same_v <bar, no_bar>) {
			return Tlist <Tail...> ();
		} else {
			return Tlist <effect_entry <Key, dep, bar>, Tail...> ();
		}
	} else {
		auto rest = map_apply(Key(), DepRes(), Bar(), Tlist <Tail...> ());
		using rest_t = decltype(rest);
		return tlist_prepend_t <Head, rest_t> ();
	}
}

template <typename Key, typename DepRes, typename Bar, typename Map>
using map_apply_t = decltype(map_apply(Key(), DepRes(), Bar(), Map()));

template <typename ... Entries>
consteval bool map_has_dep(Tlist <Entries...>)
{
	return (false || ... || std::is_same_v <typename Entries::dep, tag_dep>);
}

template <typename T>
struct is_target_key : std::false_type {};

template <auto &ref>
struct is_target_key <key_target <ref>> : std::true_type {};

template <typename ... Entries>
consteval bool map_has_non_target_dep(Tlist <Entries...>)
{
	return (false || ... ||
		(std::is_same_v <typename Entries::dep, tag_dep> && !is_target_key <typename Entries::key> ::value));
}

// Track pending index-buffer indicators (Option B)
template <typename Key, typename List, size_t ... Is>
consteval bool contains_key(std::index_sequence <Is...>)
{
	return (false || ... || std::is_same_v <typename List::template get <Is>, Key>);
}

template <typename Key, typename ... Entries>
consteval auto indicators_add(Tlist <Entries...>, Key)
{
	using list = Tlist <Entries...>;
	if constexpr (contains_key <Key, list> (std::make_index_sequence <list::size> ())) {
		return list();
	} else {
		return tlist_concat(list(), Tlist <Key> ());
	}
}

template <typename Indicators, typename Key>
using indicators_add_t = decltype(indicators_add(Indicators(), Key()));

template <typename Map>
consteval auto enforce_index_deps(Map, Tlist <>)
{
	return Map();
}

template <typename Map, typename Head, typename ... Tail>
consteval auto enforce_index_deps(Map, Tlist <Head, Tail...>)
{
	auto next = map_apply(Head(), tag_dep(), no_bar(), Map());
	return enforce_index_deps(next, Tlist <Tail...> ());
}

template <typename Map, typename Indicators>
using enforce_index_deps_t = decltype(enforce_index_deps(Map(), Indicators()));

template <typename Map, typename Indicators, typename Effect>
consteval auto normalize_step(Map, Indicators, Effect)
{
	if constexpr (is_dependency_sentinel_effect_v <Effect>) {
		if constexpr (map_has_non_target_dep(Map()))
			static_assert(false, "command recording has unresolved dependencies"_ss);
		return std::pair <Map, Indicators> ();
	} else if constexpr (is_enforcer_index_effect_v <Effect>) {
		return std::pair <enforce_index_deps_t <Map, Indicators>, Tlist <>> ();
	} else if constexpr (is_indicator_index_effect_v <Effect>) {
		return std::pair <Map, indicators_add_t <Indicators, key_index <Effect::topology>>> ();
	} else if constexpr (is_dependency_effect_v <Effect>) {
		return std::pair <
			map_apply_t <key_ref <Effect::handle>, tag_dep, no_bar, Map>,
			Indicators
		> ();
	} else if constexpr (is_resolvant_effect_v <Effect>) {
		return std::pair <
			map_apply_t <key_ref <Effect::handle>, tag_res, no_bar, Map>,
			Indicators
		> ();
	} else if constexpr (is_barrier_effect_v <Effect>) {
		return std::pair <
			map_apply_t <
				key_ref <Effect::handle>,
				tag_none,
				tag_bar <
					typename Effect::src_phase,
					typename Effect::dst_phase
				>,
				Map
			>,
			Indicators
		> ();
	} else if constexpr (is_resolvant_index_effect_v <Effect>) {
		return std::pair <
			map_apply_t <key_index <Effect::topology>, tag_res, no_bar, Map>,
			Indicators
		> ();
	} else if constexpr (is_dependency_index_effect_v <Effect>) {
		return std::pair <
			map_apply_t <key_index <Effect::topology>, tag_dep, no_bar, Map>,
			Indicators
		> ();
	} else if constexpr (is_target_write_effect_v <Effect>) {
		return std::pair <
			map_apply_t <key_target <Effect::handle>, tag_res, no_bar, Map>,
			Indicators
		> ();
	} else if constexpr (is_target_read_effect_v <Effect>) {
		return std::pair <
			map_apply_t <key_target <Effect::handle>, tag_dep, no_bar, Map>,
			Indicators
		> ();
	} else {
		static_assert(false, "unsupported command effect"_ss);
		return std::pair <Map, Indicators> ();
	}
}

template <typename Map, typename Indicators, typename Effect>
using normalize_step_t = decltype(normalize_step(Map(), Indicators(), Effect()));

// Fold a list of effects into the summary map + indicator list
template <typename Map, typename Indicators, typename ... Effects>
struct normalize_fold;

template <typename Map, typename Indicators>
struct normalize_fold <Map, Indicators> {
	using map = Map;
};

template <typename Map, typename Indicators, typename Head, typename ... Tail>
struct normalize_fold <Map, Indicators, Head, Tail...> {
	using step = normalize_step_t <Map, Indicators, Head>;
	using next_map = typename step::first_type;
	using next_indicators = typename step::second_type;
	using map = typename normalize_fold <next_map, next_indicators, Tail...> ::map;
};

template <typename Entry>
struct entry_effects;

template <auto &ref, typename Bar>
struct bar_effects;

template <auto &ref>
struct bar_effects <ref, no_bar> {
	using type = Tlist <>;
};

template <auto &ref, typename Src, typename Dst>
struct bar_effects <ref, tag_bar <Src, Dst>> {
	using type = Tlist <BarrierEffect <ref, Src, Dst>>;
};

template <auto &ref, typename DepRes, typename Bar>
struct entry_effects <effect_entry <key_ref <ref>, DepRes, Bar>> {
	using dep = std::conditional_t <
		std::is_same_v <DepRes, tag_dep>,
		Tlist <Dependency <ref>>,
		std::conditional_t <
			std::is_same_v <DepRes, tag_res>,
			Tlist <Resolvant <ref>>,
			Tlist <>
		>
	>;
	using bar = typename bar_effects <ref, Bar> ::type;
	using type = tlist_concat_t <dep, bar>;
};

template <auto &ref, typename DepRes, typename Bar>
struct entry_effects <effect_entry <key_target <ref>, DepRes, Bar>> {
	static_assert(std::is_same_v <Bar, no_bar>, "render target cannot carry barrier effects");
	using type = std::conditional_t <
		std::is_same_v <DepRes, tag_dep>,
		Tlist <TargetRead <ref>>,
		std::conditional_t <
			std::is_same_v <DepRes, tag_res>,
			Tlist <TargetWrite <ref>>,
			Tlist <>
		>
	>;
};

template <Topology T, typename DepRes, typename Bar>
struct entry_effects <effect_entry <key_index <T>, DepRes, Bar>> {
	static_assert(std::is_same_v <Bar, no_bar>, "index buffer cannot carry barrier effects");
	using type = std::conditional_t <
		std::is_same_v <DepRes, tag_dep>,
		Tlist <DependencyForIndexBuffer <T>>,
		std::conditional_t <
			std::is_same_v <DepRes, tag_res>,
			Tlist <ResolvantForIndexBuffer <T>>,
			Tlist <>
		>
	>;
};

// Convert the per-key summary map back into a canonical effect list
template <typename ... Entries>
consteval auto map_to_effects(Tlist <Entries...>)
{
	if constexpr (sizeof...(Entries) == 0) {
		return Tlist <> ();
	} else if constexpr (sizeof...(Entries) == 1) {
		return (typename entry_effects <Entries> ::type(), ...);
	} else {
		return tlist_concat(typename entry_effects <Entries> ::type()...);
	}
}

template <typename List>
struct normalize_effects;

template <typename ... Effects>
struct normalize_effects <Tlist <Effects...>> {
	using map = typename normalize_fold <Tlist <>, Tlist <>, Effects...> ::map;
	using type = decltype(map_to_effects(map()));
};

template <typename List>
using normalize_effects_t = typename normalize_effects <List> ::type;

} // namespace rcgp
