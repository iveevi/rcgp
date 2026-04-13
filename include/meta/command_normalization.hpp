#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "../util/cti.hpp"
#include "../util/tlist.hpp"
#include "barrier.hpp"
#include "command_effects.hpp"
#include "static_string.hpp"

namespace rcgp {

enum class KeyKind : std::uint8_t {
	eNone,
	eRef,
	eTarget,
	eSampledDecl,
	eIndex,
};

struct norm_key {
	KeyKind kind = KeyKind::eNone;
	void *addr = nullptr;
	Topology topology = Topology::eTriangleList;

	constexpr bool operator==(const norm_key &) const = default;
};

enum class NormOp : std::uint8_t {
	eNone,
	eDep,
	eRes,
	ePendingDep,
};

struct norm_phase {
	PipelineStage stage = PipelineStage::eNone;
	ResourceAccess access = ResourceAccess::eNone;

	constexpr bool operator==(const norm_phase &) const = default;
	constexpr bool valid() const { return stage != PipelineStage::eNone; }
};

struct norm_bar {
	norm_phase src {};
	norm_phase dst {};

	constexpr bool operator==(const norm_bar &) const = default;
	constexpr bool valid() const { return src.valid() || dst.valid(); }
};

struct norm_entry {
	norm_key key {};
	NormOp op = NormOp::eNone;
	norm_bar bar {};

	constexpr bool operator==(const norm_entry &) const = default;
	constexpr bool empty() const { return op == NormOp::eNone && !bar.valid(); }
};

constexpr NormOp combine_op(NormOp a, NormOp b)
{
	if (a == NormOp::eNone)
		return b;
	if (b == NormOp::eNone)
		return a;
	if (a == b)
		return a;
	if ((a == NormOp::eDep && b == NormOp::eRes) ||
	    (a == NormOp::eRes && b == NormOp::eDep))
		return NormOp::eNone;
	// pending_dep is demoted to dep once it meets any resolved op
	if (a == NormOp::ePendingDep)
		return combine_op(NormOp::eDep, b);
	if (b == NormOp::ePendingDep)
		return combine_op(a, NormOp::eDep);
	return NormOp::eNone;
}

// lossy: non-chaining phases collapse to no-bar
constexpr norm_bar combine_bar(norm_bar a, norm_bar b)
{
	if (!a.valid()) return b;
	if (!b.valid()) return a;
	if (a.dst == b.src) return { a.src, b.dst };
	if (b.dst == a.src) return { b.src, a.dst };
	return {};
}

constexpr norm_entry combine(norm_entry a, norm_entry b)
{
	return { a.key, combine_op(a.op, b.op), combine_bar(a.bar, b.bar) };
}

enum class EffectKind : std::uint8_t {
	eUnsupported,
	eOrdinary,
	eBarrier,
	eIndicator,
	eEnforcer,
	eSentinel,
};

struct effect_projection {
	EffectKind kind = EffectKind::eUnsupported;
	norm_entry entry {};
};

template <typename T>
constexpr effect_projection project_effect(const T &)
{
	return { EffectKind::eUnsupported, {} };
}

template <auto &ref>
constexpr effect_projection project_effect(const Dependency <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eRef, (void *) &ref }, NormOp::eDep, {} } };
}

template <auto &ref>
constexpr effect_projection project_effect(const Resolvant <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eRef, (void *) &ref }, NormOp::eRes, {} } };
}

template <auto &ref>
constexpr effect_projection project_effect(const TargetWrite <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eTarget, (void *) &ref }, NormOp::eRes, {} } };
}

template <auto &ref>
constexpr effect_projection project_effect(const TargetRead <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eTarget, (void *) &ref }, NormOp::eDep, {} } };
}

template <auto &ref>
constexpr effect_projection project_effect(const DeclaresSampled <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eSampledDecl, (void *) &ref }, NormOp::eRes, {} } };
}

template <auto &ref>
constexpr effect_projection project_effect(const SamplesTarget <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eSampledDecl, (void *) &ref }, NormOp::eDep, {} } };
}

template <Topology T>
constexpr effect_projection project_effect(const DependencyForIndexBuffer <T> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eIndex, nullptr, T }, NormOp::eDep, {} } };
}

template <Topology T>
constexpr effect_projection project_effect(const ResolvantForIndexBuffer <T> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eIndex, nullptr, T }, NormOp::eRes, {} } };
}

template <auto &ref, typename Src, typename Dst>
constexpr effect_projection project_effect(const BarrierEffect <ref, Src, Dst> &)
{
	return {
		EffectKind::eBarrier,
		{
			{ KeyKind::eRef, (void *) &ref },
			NormOp::eNone,
			{ { Src::stage, Src::access }, { Dst::stage, Dst::access } },
		},
	};
}

template <Topology T>
constexpr effect_projection project_effect(const DependencyIndicatorForIndexBuffer <T> &)
{
	return { EffectKind::eIndicator, { { KeyKind::eIndex, nullptr, T }, NormOp::ePendingDep, {} } };
}

constexpr effect_projection project_effect(const DependencyEnforcerForIndexBuffer &)
{
	return { EffectKind::eEnforcer, {} };
}

constexpr effect_projection project_effect(const DependencySentinel &)
{
	return { EffectKind::eSentinel, {} };
}

template <std::size_t N>
struct norm_state {
	std::array <norm_entry, N> entries {};
	std::size_t count = 0;
	bool sentinel_error = false;
	norm_key first_error_key {};

	constexpr std::size_t find(norm_key k) const
	{
		for (std::size_t i = 0; i < count; ++i)
			if (entries[i].key == k) return i;
		return N;
	}

	constexpr void apply(norm_entry e)
	{
		std::size_t i = find(e.key);
		if (i == N) {
			entries[count++] = e;
			return;
		}
		entries[i] = combine(entries[i], e);
		if (entries[i].empty()) {
			entries[i] = entries[--count];
			entries[count] = {};
		}
	}

	// target deps are excluded: cross-pass ordering may span module boundaries
	constexpr bool has_non_target_dep() const
	{
		for (std::size_t i = 0; i < count; ++i) {
			const auto &e = entries[i];
			if (e.op == NormOp::eDep && e.key.kind != KeyKind::eTarget)
				return true;
		}
		return false;
	}

	constexpr void enforce_index_deps()
	{
		for (std::size_t i = 0; i < count; ++i) {
			if (entries[i].op == NormOp::ePendingDep)
				entries[i].op = NormOp::eDep;
		}
	}
};

template <typename Effect, std::size_t N>
constexpr void apply_effect(norm_state <N> &state)
{
	constexpr auto p = project_effect(Effect {});
	static_assert(p.kind != EffectKind::eUnsupported,
		"normalize: unsupported command effect");

	if constexpr (p.kind == EffectKind::eSentinel) {
		if (state.has_non_target_dep()) {
			state.sentinel_error = true;
			for (std::size_t i = 0; i < state.count; ++i) {
				const auto &e = state.entries[i];
				if (e.op == NormOp::eDep && e.key.kind != KeyKind::eTarget) {
					state.first_error_key = e.key;
					break;
				}
			}
		}
	} else if constexpr (p.kind == EffectKind::eEnforcer) {
		state.enforce_index_deps();
	} else {
		state.apply(p.entry);
	}
}

template <typename ... Effects>
consteval auto fold_effects()
{
	norm_state <sizeof...(Effects) + 1> state {};
	(apply_effect <Effects> (state), ...);
	return state;
}

// opaque carrier for a normalized entry; projects back to itself on re-normalization
template <norm_entry Value>
struct NormalEntry {};

template <norm_entry Value>
constexpr effect_projection project_effect(const NormalEntry <Value> &)
{
	return {
		(Value.bar.valid() && Value.op == NormOp::eNone)
			? EffectKind::eBarrier
			: EffectKind::eOrdinary,
		Value,
	};
}

template <auto State, std::size_t ... Is>
auto decode_state(std::index_sequence <Is...>)
	-> Tlist <NormalEntry <State.entries[Is]>...>;

// Returns a pair (normalized Tlist, error). `error` is int(0) when the fold
// is clean, or a static_string with a diagnostic when the sentinel caught an
// unresolved dep. The call site (operator|) discriminates on the error type
// and fires the static_assert locally — keeping the instantiation chain short.
template <typename ... Effects>
consteval auto normalize_effects(Tlist <Effects...>)
{
	constexpr auto state = fold_effects <Effects...> ();
	using tlist_t = decltype(decode_state <state> (std::make_index_sequence <state.count> ()));

	if constexpr (state.sentinel_error)
		return std::pair { tlist_t {}, "command recording has unresolved dependencies"_ss };
	else
		return std::pair { tlist_t {}, 0 };
}

} // namespace rcgp
