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
#include "type_string_overrides.hpp"

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
	const char *name = nullptr;

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
	return {
		a.key,
		combine_op(a.op, b.op),
		combine_bar(a.bar, b.bar),
		(a.name != nullptr) ? a.name : b.name,
	};
}

// Static-storage null-terminated buffer of `ref`'s bare name; its `.data()`
// is stashed in `norm_entry::name` so dep diagnostics can name the resource.
template <auto &ref>
inline constexpr auto ref_name_storage = [] {
	constexpr auto v = ref_name_view <ref> ();
	std::array <char, v.size() + 1> buf {};
	for (std::size_t i = 0; i < v.size(); ++i)
		buf[i] = v[i];
	return buf;
} ();

template <auto &ref>
inline constexpr const char *ref_name_cstr = ref_name_storage <ref> .data();

enum class EffectKind : std::uint8_t {
	eUnsupported,
	eOrdinary,
	eBarrier,
	eIndicator,
	eEnforcer,
	eActivatePipeline,
	eBegin,
	eSentinel,
};

struct effect_projection {
	EffectKind kind = EffectKind::eUnsupported;
	norm_entry entry {};
	PipelineKind pipeline_kind = PipelineKind::eNone;
};

template <typename T>
constexpr effect_projection project_effect(const T &)
{
	return { EffectKind::eUnsupported, {} };
}

template <auto &ref>
constexpr effect_projection project_effect(const Dependency <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eRef, (void *) &ref }, NormOp::eDep, {}, ref_name_cstr <ref> } };
}

template <auto &ref>
constexpr effect_projection project_effect(const Resolvant <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eRef, (void *) &ref }, NormOp::eRes, {}, ref_name_cstr <ref> } };
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
	return { EffectKind::eOrdinary, { { KeyKind::eSampledDecl, (void *) &ref }, NormOp::eRes, {}, ref_name_cstr <ref> } };
}

template <auto &ref>
constexpr effect_projection project_effect(const SamplesTarget <ref> &)
{
	return { EffectKind::eOrdinary, { { KeyKind::eSampledDecl, (void *) &ref }, NormOp::eDep, {}, ref_name_cstr <ref> } };
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

template <PipelineKind K>
constexpr effect_projection project_effect(const ActivatePipeline <K> &)
{
	return { EffectKind::eActivatePipeline, {}, K };
}

template <PipelineKind K>
constexpr effect_projection project_effect(const TerminalSentinel <K> &)
{
	return { EffectKind::eSentinel, {}, K };
}

constexpr effect_projection project_effect(const Begin &)
{
	return { EffectKind::eBegin, {} };
}

template <std::size_t N>
struct norm_state {
	std::array <norm_entry, N> entries {};
	std::size_t count = 0;
	bool sentinel_error = false;
	bool pipeline_kind_error = false;
	bool backstop_active = false;
	PipelineKind current_pipeline = PipelineKind::eNone;
	PipelineKind expected_pipeline = PipelineKind::eNone;
	norm_key first_error_key {};

	constexpr std::size_t find(norm_key k) const
	{
		for (std::size_t i = 0; i < count; i++) {
			if (entries[i].key == k)
				return i;
		}

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
		// strict check when backstop is active (submission context) or a
		// pipeline is already bound; fragments without either pass through
		const bool strict = state.backstop_active ||
			state.current_pipeline != PipelineKind::eNone;
		if (strict && state.current_pipeline != p.pipeline_kind) {
			state.pipeline_kind_error = true;
			state.expected_pipeline = p.pipeline_kind;
		}
	} else if constexpr (p.kind == EffectKind::eEnforcer) {
		state.enforce_index_deps();
	} else if constexpr (p.kind == EffectKind::eActivatePipeline) {
		state.current_pipeline = p.pipeline_kind;
	} else if constexpr (p.kind == EffectKind::eBegin) {
		state.backstop_active = true;
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
auto decode_entries(std::index_sequence <Is...>)
	-> Tlist <NormalEntry <State.entries[Is]>...>;

template <PipelineKind K, typename EntriesList>
struct prepend_pipeline_marker {
	using type = EntriesList;
};

template <PipelineKind K, typename ... Ts>
requires (K != PipelineKind::eNone)
struct prepend_pipeline_marker <K, Tlist <Ts...>> {
	using type = Tlist <ActivatePipeline <K>, Ts...>;
};

constexpr bool entry_is_unresolved(const norm_entry &e)
{
	return e.op == NormOp::eDep
		&& (e.key.kind == KeyKind::eRef
			|| e.key.kind == KeyKind::eSampledDecl
			|| e.key.kind == KeyKind::eIndex);
}

consteval std::size_t cstrlen(const char *s)
{
	std::size_t n = 0;
	while (s && s[n])
		++n;
	return n;
}

template <auto State>
consteval std::size_t unresolved_names_total_len()
{
	std::size_t total = 0;
	bool first = true;
	for (std::size_t i = 0; i < State.count; ++i) {
		const auto &e = State.entries[i];
		if (!entry_is_unresolved(e))
			continue;
		if (!first) total += 2; // ", "
		first = false;
		if (e.name != nullptr) {
			total += cstrlen(e.name);
		} else if (e.key.kind == KeyKind::eIndex) {
			total += sizeof("<index buffer>") - 1;
		} else {
			total += sizeof("<unnamed>") - 1;
		}
	}
	return total;
}

template <auto State>
consteval auto unresolved_names_string()
{
	constexpr std::size_t L = unresolved_names_total_len <State> ();
	static_string <L> result;
	std::size_t out = 0;
	bool first = true;
	for (std::size_t i = 0; i < State.count; ++i) {
		const auto &e = State.entries[i];
		if (!entry_is_unresolved(e))
			continue;
		if (!first) {
			result.elements[out++] = ',';
			result.elements[out++] = ' ';
		}
		first = false;
		auto write = [&] (const char *s) {
			for (std::size_t j = 0; s[j]; ++j)
				result.elements[out++] = s[j];
		};
		if (e.name != nullptr) {
			write(e.name);
		} else if (e.key.kind == KeyKind::eIndex) {
			write("<index buffer>");
		} else {
			write("<unnamed>");
		}
	}
	return result;
}

template <typename ... Effects>
consteval auto normalize_effects(Tlist <Effects...>)
{
	constexpr auto state = fold_effects <Effects...> ();
	using entries_list = decltype(decode_entries <state> (std::make_index_sequence <state.count> ()));
	using tlist_t = typename prepend_pipeline_marker <state.current_pipeline, entries_list> ::type;

	if constexpr (state.sentinel_error) {
		return std::pair {
			tlist_t {},
			"commands combinator detected dispatch before all dependencies\nwere provided:\n\tmissing dependencies: "_ss
				+ unresolved_names_string <state> ()
		};
	} else if constexpr (state.pipeline_kind_error) {
		return std::pair { tlist_t {}, "command recording: terminal directive expects a different pipeline kind than the one currently bound"_ss };
	} else {
		return std::pair { tlist_t {}, 0 };
	}
}

} // namespace rcgp
