#pragma once

#include <type_traits>

#include "../dsl/jems.hpp"
#include "../util/cti.hpp"
#include "layouts.hpp"
#include "reconstruct_type.hpp"
#include "resources.hpp"

namespace rcgp {

// Layout detection
template <template <typename> typename L>
consteval GlobalResourceLayout layout_of()
{
	using sample = vector <int, 3>;
	if constexpr (std::is_same_v <L <sample>, layouts::std430 <sample>>)
		return GlobalResourceLayout::eStd430;
	else if constexpr (std::is_same_v <L <sample>, layouts::scalar <sample>>)
		return GlobalResourceLayout::eScalar;
	else
		static_error("unsupported layout for global resource"_ss);
}

template <typename T, template <typename> typename L>
jems::handle resource_intrinsic(const PushConstant <T, L> &, uint32_t binding)
{
	return jems::global_resource(
		reconstruct_type <T> (),
		GlobalResourceKind::ePushConstant,
		layout_of <L> (),
		GlobalResourceAccess::eReadWrite,
		std::nullopt,
		binding
	);
}

template <typename T, template <typename> typename L>
jems::handle resource_intrinsic(const UniformBuffer <T, L> &, uint32_t binding)
{
	return jems::global_resource(
		reconstruct_type <T> (),
		GlobalResourceKind::eUniformBuffer,
		layout_of <L> (),
		GlobalResourceAccess::eRead,
		std::nullopt,
		binding
	);
}

template <typename T, template <typename> typename L, GlobalResourceAccess A>
jems::handle resource_intrinsic(const StorageBuffer <T, L, A> &, uint32_t binding)
{
	return jems::global_resource(
		reconstruct_type <T> (),
		GlobalResourceKind::eStorageBuffer,
		layout_of <L> (),
		A,
		std::nullopt,
		binding
	);
}

template <typename T, size_t D>
jems::handle resource_intrinsic(const Sampler <T, D> &, uint32_t binding)
{
	return jems::global_resource(
		jems::type(VectorType <T, D> ()),
		GlobalResourceKind::eSampler,
		GlobalResourceLayout::eNone,
		GlobalResourceAccess::eRead,
		std::nullopt,
		binding
	);
}

} // namespace rcgp
