#pragma once

#include <type_traits>

#include "stage_intrinsics.hpp"
#include "shader_stage.hpp"

namespace rcgp {

template <ShaderStage S, typename T>
void return_handler(const TaskPayload <T> &, size_t &)
{
	static_assert(S == ShaderStage::eTask);
	$tsb.task_payload_type = reconstruct_type <T> ();
}

// TODO: can we do ..., void> or is that partial specialization
template <ShaderStage S, MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives, typename T>
requires std::is_void_v <T>
void return_handler(const MeshletPayload <P, MaxVertices, MaxPrimitives, T> &, size_t &)
{
	static_assert(S == ShaderStage::eMesh);
	$tsb.mesh_max_vertices = MaxVertices;
	$tsb.mesh_max_primitives = MaxPrimitives;
	$tsb.mesh_primitive_kind = P;
}

template <ShaderStage S, MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives, typename T>
void return_handler(const MeshletPayload <P, MaxVertices, MaxPrimitives, T> &, size_t &)
{
	static_assert(S == ShaderStage::eMesh);
	$tsb.mesh_max_vertices = MaxVertices;
	$tsb.mesh_max_primitives = MaxPrimitives;
	$tsb.mesh_primitive_kind = P;
}

template <typename T, RateProperties P>
void interpolation_return_handler(const Interpolant <T, P> &ret, size_t &argi)
{
	auto type = reconstruct_type <T> ();
	auto tout = ThreadOutput(type, argi, P);
	$tsb.add_thread_output(tout);

	// Fix the argument index of the original value
	ret._ref->template as <ThreadOutput> ().argi = argi++;
}

// TODO: shorten using type traits
template <ShaderStage S, primitive T>
void return_handler(const Smooth <T> &ret, size_t &argi)
{
	static_assert(S == ShaderStage::eVertex);
	interpolation_return_handler(ret, argi);
}

template <ShaderStage S, primitive T>
void return_handler(const Flat <T> &ret, size_t &argi)
{
	static_assert(S == ShaderStage::eVertex);
	interpolation_return_handler(ret, argi);
}

template <ShaderStage S, primitive T>
void return_handler(const NoPerspective <T> &ret, size_t &argi)
{
	static_assert(S == ShaderStage::eVertex);
	interpolation_return_handler(ret, argi);
}

template <ShaderStage S, primitive T>
void return_handler(const T &ret, size_t &argi)
{
	// TODO: Result instruction for subroutines...
	// or some other pathway for subroutines
	auto type = reconstruct_type <T> ();

	auto tout = ThreadOutput(type, argi++,
		(S == ShaderStage::eVertex)
		? RateProperties::eSmooth // default value
		: RateProperties::eNone);

	$tsb.add_thread_output(tout);

	jems::store(jems::thread_output(tout), ret);
}

template <ShaderStage S, aggregate T>
void return_handler(const T &ret, size_t &argi)
{
	// TODO: restrict to only subroutines?
	auto type = reconstruct_type <T> ();
	auto tout = ThreadOutput(type, argi++, RateProperties::eNone);
	$tsb.add_thread_output(tout);

	jems::store(jems::thread_output(tout), coerce_to_handle(ret));
}

template <ShaderStage S, typename ... Args>
void return_handler(const std::tuple <Args...> &ret, size_t &argi)
{
	constexpr_for(Is, sizeof...(Args),
		(return_handler <S> (std::get <Is> (ret), argi), ...)
	);
}

} // namespace rcgp
