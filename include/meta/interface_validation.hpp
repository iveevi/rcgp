#pragma once

#include "shader_stage.hpp"
#include "intrinsics_vertex.hpp"
#include "intrinsics_mesh.hpp"
#include "intrinsics_raytracing.hpp"
#include "type_string_overrides.hpp"

namespace rcgp {

template <std::size_t I, typename Tlist>
struct nth_type;

template <typename T0, typename ... Ts>
struct nth_type <0, Tlist <T0, Ts...>> { using type = T0; };

template <std::size_t I, typename T0, typename ... Ts>
requires (I > 0)
struct nth_type <I, Tlist <T0, Ts...>> {
	using type = typename nth_type <I - 1, Tlist <Ts...>> ::type;
};

template <std::size_t I, typename Tlist>
using nth_type_t = typename nth_type <I, Tlist> ::type;

template <typename A, typename B>
struct first_mismatch_index;

template <>
struct first_mismatch_index <Tlist <>, Tlist <>>
	: std::integral_constant <std::size_t, 0> {};

template <typename A, typename ... As, typename B, typename ... Bs>
struct first_mismatch_index <Tlist <A, As...>, Tlist <B, Bs...>>
	: std::integral_constant <std::size_t,
		std::is_same_v <A, B>
			? 1 + first_mismatch_index <Tlist <As...>, Tlist <Bs...>> ::value
			: 0> {};

template <typename ... Ts>
consteval auto join_pretty_types()
{
	if constexpr (sizeof...(Ts) == 0) {
		return ""_ss;
	} else {
		return [] <typename Head, typename ... Rest>
			(std::type_identity <Head>, std::type_identity <Rest> ...) {
			auto head = $ss_type(Head);
			if constexpr (sizeof...(Rest) == 0)
				return head;
			else
				return head + ", "_ss + join_pretty_types <Rest...> ();
		} (std::type_identity <Ts> {}...);
	}
}

template <typename Tl>
struct tlist_to_paren_string_impl;

template <typename ... Ts>
struct tlist_to_paren_string_impl <Tlist <Ts...>> {
	static consteval auto eval()
	{
		return "("_ss + join_pretty_types <Ts...> () + ")"_ss;
	}
};

template <typename Tl>
consteval auto tlist_to_paren_string()
{
	return tlist_to_paren_string_impl <Tl> ::eval();
}

template <typename Tl>
struct tlist_size;

template <typename ... Ts>
struct tlist_size <Tlist <Ts...>>
	: std::integral_constant <std::size_t, sizeof...(Ts)> {};

// Unwrap interpolation qualifiers from vertex/mesh return types
template <typename T>
struct unwrap_output { using type = Tlist <T>; };

template <>
struct unwrap_output <void> { using type = Tlist <>; };

template <builtin T, RateProperties P>
struct unwrap_output <Interpolant <T, P>> { using type = Tlist <T>; };

template <builtin T>
struct unwrap_output <Smooth <T>> { using type = Tlist <T>; };

template <builtin T>
struct unwrap_output <Flat <T>> { using type = Tlist <T>; };

template <builtin T>
struct unwrap_output <NoPerspective <T>> { using type = Tlist <T>; };

template <typename ... Ts>
struct unwrap_output <std::tuple <Ts...>> {
	using type = tlist_concat_t <typename unwrap_output <Ts> ::type...>;
};

template <typename T>
using unwrap_output_t = typename unwrap_output <T> ::type;

// Filter stage IO types (traced params) from shader args
template <typename T>
constexpr bool is_stage_io_v =
	!is_implicit_context_v <T>
	and !is_contract_v <T>
	and !std::is_same_v <T, jems::null>
	and !is_read_only_intrinsic_v <T>
	and !is_write_only_intrinsic_v <T>
	and !is_workgroup_v <T>
	and !is_task_payload_v <T>;

template <typename T>
struct is_stage_io : std::bool_constant <is_stage_io_v <T>> {};

template <typename Shader>
using stage_inputs_t = tlist_filter_t <is_stage_io, typename Shader::args>;

// Vertex -> Fragment: outputs must match inputs
template <typename VertexShader, typename FragmentShader>
constexpr bool vertex_fragment_io_compatible =
	std::is_same_v <
		unwrap_output_t <typename VertexShader::return_type>,
		stage_inputs_t <FragmentShader>
	>;

// Task -> Mesh: payload types must match
template <typename T>
struct extract_task_payload { using type = void; };

template <typename T>
struct extract_task_payload <TaskPayload <T>> { using type = T; };

template <typename Shader>
using task_payload_from_return_t =
	typename extract_task_payload <typename Shader::return_type> ::type;

template <typename Shader>
using task_payload_from_args_t = typename extract_task_payload <
	typename tlist_filter_t <is_task_payload, typename Shader::args> ::template get <0>
> ::type;

template <typename TaskShader, typename MeshShader>
constexpr bool task_mesh_payload_compatible =
	std::is_same_v <
		task_payload_from_return_t <TaskShader>,
		task_payload_from_args_t <MeshShader>
	>;

// Mesh -> Fragment: outputs must match inputs
template <typename T>
struct meshlet_output_types { using type = Tlist <>; };

template <MeshPrimitive P, uint32_t MaxV, uint32_t MaxP, user_defined T>
struct meshlet_output_types <MeshletPayload <P, MaxV, MaxP, T>> {
	template <typename F>
	struct element_of { using type = typename F::element_type; };

	using type = tlist_transform_t <typename T::fields, element_of>;
};

template <typename Shader>
using mesh_outputs_t =
	typename meshlet_output_types <typename Shader::return_type> ::type;

template <typename MeshShader, typename FragmentShader>
constexpr bool mesh_fragment_io_compatible =
	std::is_same_v <mesh_outputs_t <MeshShader>, stage_inputs_t <FragmentShader>>;

// Raytracing: extract dispatcher/receiver trace group addresses from contracts
template <typename T>
struct is_dispatcher_contract : std::false_type {};

template <auto &ref>
requires is_ray_dispatcher_v <reference_base_of <ref>>
struct is_dispatcher_contract <contract <ref>> : std::true_type {};

template <typename T>
struct is_receiver_contract : std::false_type {};

template <auto &ref>
requires is_ray_receiver_v <reference_base_of <ref>>
struct is_receiver_contract <contract <ref>> : std::true_type {};

template <typename T>
struct contract_address;

template <auto &ref>
struct contract_address <contract <ref>> {
	static constexpr void *value = reference_base_of <ref> ::address;
};

template <void *Addr>
struct address_tag {};

template <typename T>
struct contract_to_address_tag {
	using type = address_tag <contract_address <T> ::value>;
};

template <typename Shader>
using dispatcher_addresses_t = tlist_unique_t <tlist_transform_t <
	tlist_filter_t <is_dispatcher_contract, typename Shader::args>,
	contract_to_address_tag
>>;

template <typename Shader>
using receiver_addresses_t = tlist_unique_t <tlist_transform_t <
	tlist_filter_t <is_receiver_contract, typename Shader::args>,
	contract_to_address_tag
>>;

template <typename Subset, typename Superset>
struct tlist_subset_of;

template <typename Superset>
struct tlist_subset_of <Tlist <>, Superset> : std::true_type {};

template <typename Head, typename ... Rest, typename Superset>
struct tlist_subset_of <Tlist <Head, Rest...>, Superset>
	: std::bool_constant <
		tlist_contains <Head, Superset> ::value
		and tlist_subset_of <Tlist <Rest...>, Superset> ::value
	> {};

template <typename ... Shaders>
using all_receiver_addresses_t = tlist_unique_t <tlist_concat_t <
	receiver_addresses_t <Shaders>...
>>;

template <typename ... Shaders>
using all_dispatcher_addresses_t = tlist_unique_t <tlist_concat_t <
	dispatcher_addresses_t <Shaders>...
>>;

// Raytracing: every receiver must have a matching dispatcher
template <typename DispatcherAddresses, typename ReceiverAddresses>
constexpr bool raytracing_receivers_covered =
	tlist_subset_of <ReceiverAddresses, DispatcherAddresses> ::value;

template <typename Subset, typename Superset>
struct tlist_minus;

template <typename Superset>
struct tlist_minus <Tlist <>, Superset> { using type = Tlist <>; };

template <typename Head, typename ... Rest, typename Superset>
struct tlist_minus <Tlist <Head, Rest...>, Superset> {
	using rest = typename tlist_minus <Tlist <Rest...>, Superset> ::type;
	using type = std::conditional_t <
		tlist_contains <Head, Superset> ::value,
		rest,
		typename rest::template insert <Head>
	>;
};

template <typename Tag>
struct addr_of_tag;

template <void *A>
struct addr_of_tag <address_tag <A>> {
	static constexpr void *value = A;
};

template <typename Tags>
struct join_addr_tag_names;

template <>
struct join_addr_tag_names <Tlist <>> {
	static consteval auto eval() { return ""_ss; }
};

template <typename Head, typename ... Rest>
struct join_addr_tag_names <Tlist <Head, Rest...>> {
	static consteval auto eval()
	{
		auto head = $ss_addr_name(addr_of_tag <Head> ::value);
		if constexpr (sizeof...(Rest) == 0)
			return head;
		else
			return head + ", "_ss + join_addr_tag_names <Tlist <Rest...>> ::eval();
	}
};

// Per-combinator checks. Each returns 0 on success or a static_string with
// the diagnostic message; the caller does
//     if constexpr (not std::is_same_v <decltype(r), int>) static_assert(false, r);
// mirroring the normalize_effects pattern in command_normalization.hpp.

template <typename VertexShader, typename FragmentShader>
consteval auto check_vertex_fragment_io()
{
	using V = unwrap_output_t <typename VertexShader::return_type>;
	using F = stage_inputs_t <FragmentShader>;
	if constexpr (std::is_same_v <V, F>) {
		return 0;
	} else {
		constexpr auto vsize = tlist_size <V> ::value;
		constexpr auto fsize = tlist_size <F> ::value;
		if constexpr (vsize != fsize) {
			return "rasterization combinator got incompatible shader modules:\n"_ss
				+ "    vertex shader outputs: "_ss + tlist_to_paren_string <V> () + "\n"_ss
				+ "    fragment shader inputs: "_ss + tlist_to_paren_string <F> () + "\n"_ss
				+ "arity mismatch; vertex shader emits "_ss
				+ $ss_ulong_decimal(vsize) + " outputs while\nfragment shader requires "_ss
				+ $ss_ulong_decimal(fsize) + " inputs"_ss;
		} else {
			constexpr auto idx = first_mismatch_index <V, F> ::value;
			using Vmis = nth_type_t <idx, V>;
			using Fmis = nth_type_t <idx, F>;
			return "rasterization combinator got incompatible shader modules:\n"_ss
				+ "    vertex shader outputs: "_ss + tlist_to_paren_string <V> () + "\n"_ss
				+ "    fragment shader inputs: "_ss + tlist_to_paren_string <F> () + "\n"_ss
				+ "type mismatch in @"_ss + $ss_ulong_decimal(idx)
				+ "; vertex shader returns "_ss + $ss_type(Vmis)
				+ " while\nfragment shader requires "_ss + $ss_type(Fmis);
		}
	}
}

template <typename TaskShader, typename MeshShader>
consteval auto check_task_mesh_payload()
{
	using TaskPL = task_payload_from_return_t <TaskShader>;
	using MeshPL = task_payload_from_args_t <MeshShader>;
	if constexpr (std::is_same_v <TaskPL, MeshPL>) {
		return 0;
	} else {
		return "mesh-shading combinator got incompatible task and mesh payloads:\n"_ss
			+ "    task shader emits: "_ss + $ss_type(TaskPL) + "\n"_ss
			+ "    mesh shader expects: "_ss + $ss_type(MeshPL);
	}
}

template <typename MeshShader, typename FragmentShader>
consteval auto check_mesh_fragment_io()
{
	using M = mesh_outputs_t <MeshShader>;
	using F = stage_inputs_t <FragmentShader>;
	if constexpr (std::is_same_v <M, F>) {
		return 0;
	} else {
		constexpr auto msize = tlist_size <M> ::value;
		constexpr auto fsize = tlist_size <F> ::value;
		if constexpr (msize != fsize) {
			return "mesh-shading combinator got incompatible shader modules:\n"_ss
				+ "    mesh shader outputs: "_ss + tlist_to_paren_string <M> () + "\n"_ss
				+ "    fragment shader inputs: "_ss + tlist_to_paren_string <F> () + "\n"_ss
				+ "arity mismatch; mesh shader emits "_ss
				+ $ss_ulong_decimal(msize) + " outputs while\nfragment shader requires "_ss
				+ $ss_ulong_decimal(fsize) + " inputs"_ss;
		} else {
			constexpr auto idx = first_mismatch_index <M, F> ::value;
			using Mmis = nth_type_t <idx, M>;
			using Fmis = nth_type_t <idx, F>;
			return "mesh-shading combinator got incompatible shader modules:\n"_ss
				+ "    mesh shader outputs: "_ss + tlist_to_paren_string <M> () + "\n"_ss
				+ "    fragment shader inputs: "_ss + tlist_to_paren_string <F> () + "\n"_ss
				+ "type mismatch in @"_ss + $ss_ulong_decimal(idx)
				+ "; mesh shader returns "_ss + $ss_type(Mmis)
				+ " while\nfragment shader requires "_ss + $ss_type(Fmis);
		}
	}
}

template <typename RayGenerationShader, typename ... MissShaders, typename ... ClosestHitShaders>
consteval auto check_raytracing_coverage(Tlist <MissShaders...>, Tlist <ClosestHitShaders...>)
{
	using Disps = all_dispatcher_addresses_t <RayGenerationShader, MissShaders..., ClosestHitShaders...>;
	using Recvs = all_receiver_addresses_t <MissShaders..., ClosestHitShaders...>;
	if constexpr (tlist_subset_of <Recvs, Disps> ::value) {
		return 0;
	} else {
		using Uncovered = typename tlist_minus <Recvs, Disps> ::type;
		return "ray tracing combinator detected receivers without a matching dispatcher:\n"_ss
			+ "\tuncovered receivers: "_ss
			+ join_addr_tag_names <Uncovered> ::eval();
	}
}

} // namespace rcgp
