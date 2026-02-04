#pragma once

#include <type_traits>

#include "coerce_to_handle.hpp"
#include "shader_stage.hpp"
#include "stage_intrinsics.hpp"

namespace rcgp {

template <ShaderStage S, typename T>
void return_handler(TaskPayload <T> &, size_t &)
{
	static_assert(S == ShaderStage::eTask);
	$tsb.task_payload_type = reconstruct_type <T> ();
}

// TODO: can we do ..., void> or is that partial specialization
template <ShaderStage S, MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives, typename T>
requires std::is_void_v <T>
void return_handler(MeshletPayload <P, MaxVertices, MaxPrimitives, T> &, size_t &)
{
	static_assert(S == ShaderStage::eMesh);
	$tsb.mesh_max_vertices = MaxVertices;
	$tsb.mesh_max_primitives = MaxPrimitives;
	$tsb.mesh_primitive_kind = P;
}

template <ShaderStage S, MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives, typename T>
void return_handler(MeshletPayload <P, MaxVertices, MaxPrimitives, T> &, size_t &)
{
	static_assert(S == ShaderStage::eMesh);
	$tsb.mesh_max_vertices = MaxVertices;
	$tsb.mesh_max_primitives = MaxPrimitives;
	$tsb.mesh_primitive_kind = P;
}

template <typename T, RateProperties P>
void interpolation_return_handler(Interpolant <T, P> &ret, size_t &argi)
{
	auto type = reconstruct_type <T> ();
	auto sout = StageOutput(type, argi, P);
	$tsb.add_stage_output(sout);

	Reference &ref = ret;
	ref->as <StageOutput> ().argi = argi++;
}

// TODO: shorten using type traits
template <ShaderStage S, primitive T>
void return_handler(Smooth <T> &ret, size_t &argi)
{
	static_assert(S == ShaderStage::eVertex);
	interpolation_return_handler(ret, argi);
}

template <ShaderStage S, primitive T>
void return_handler(Flat <T> &ret, size_t &argi)
{
	static_assert(S == ShaderStage::eVertex);
	interpolation_return_handler(ret, argi);
}

template <ShaderStage S, primitive T>
void return_handler(NoPerspective <T> &ret, size_t &argi)
{
	static_assert(S == ShaderStage::eVertex);
	interpolation_return_handler(ret, argi);
}

template <ShaderStage S, primitive T>
void return_handler(T &value, size_t &argi)
{
	auto type = reconstruct_type <T> ();

	Reference ref;
	if constexpr (S == ShaderStage::eSubroutine) {
		auto ret = Return(type, argi++);
		$tsb.add_return(ret);
		ref = jems::returns(ret);
	} else if constexpr (S == ShaderStage::eVertex) {
		auto sout = StageOutput(type, argi++, RateProperties::eSmooth);
		$tsb.add_stage_output(sout);
		ref = jems::stage_output(sout);
	} else if constexpr (S == ShaderStage::eFragment) {
		auto sout = StageOutput(type, argi++, RateProperties::eNone);
		$tsb.add_stage_output(sout);
		ref = jems::stage_output(sout);
	} else {
		static_assert(false);
	}
	
	jems::store(ref, value);
}

template <ShaderStage S, aggregate T>
void return_handler(T &value, size_t &argi)
{
	static_assert(S == ShaderStage::eSubroutine);

	auto type = reconstruct_type <T> ();
	auto ret = Return(type, argi++);
	$tsb.add_return(ret);
	auto ref = jems::returns(ret);
	jems::store(ref, coerce_to_handle(value));
}

template <ShaderStage S, typename ... Args>
void return_handler(std::tuple <Args...> &ret, size_t &argi)
{
	constexpr_for(Is, sizeof...(Args),
		(return_handler <S> (std::get <Is> (ret), argi), ...)
	);
}

} // namespace rcgp
