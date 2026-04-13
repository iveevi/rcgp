#pragma once

#include <type_traits>

#include "../dsl/jems.hpp"
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
		static_assert(false, "unsupported layout for global resource"_ss);
}

template <typename T, template <typename> typename L>
jems::handle resource_intrinsic(const PushConstant <T, L> &, uint32_t binding)
{
	return jems::global_resource(GlobalResource {
		reconstruct_type <T> (),
		GlobalResourceKind::ePushConstant,
		layout_of <L> (),
		GlobalResourceAccess::eReadWrite,
		std::nullopt,
		binding
	});
}

template <typename T, template <typename> typename L>
jems::handle resource_intrinsic(const UniformBuffer <T, L> &, uint32_t binding)
{
	return jems::global_resource(GlobalResource {
		reconstruct_type <T> (),
		GlobalResourceKind::eUniformBuffer,
		layout_of <L> (),
		GlobalResourceAccess::eRead,
		std::nullopt,
		binding
	});
}

template <typename T, template <typename> typename L, GlobalResourceAccess A>
jems::handle resource_intrinsic(const StorageBuffer <T, L, A> &, uint32_t binding)
{
	return jems::global_resource(GlobalResource {
		reconstruct_type <T> (),
		GlobalResourceKind::eStorageBuffer,
		layout_of <L> (),
		A,
		std::nullopt,
		binding
	});
}

template <typename T, size_t D, GlobalResourceAccess A>
jems::handle resource_intrinsic(const StorageImage <T, D, A> &, uint32_t binding)
{
	return jems::global_resource(GlobalResource {
		jems::type(primitive_of <T, D> ()),
		GlobalResourceKind::eStorageImage,
		GlobalResourceLayout::eNone,
		A,
		std::nullopt,
		binding
	});
}

template <typename T, size_t D>
jems::handle resource_intrinsic(const Sampler <T, D> &, uint32_t binding)
{
	return jems::global_resource(GlobalResource {
		jems::type(primitive_of <T, D> ()),
		GlobalResourceKind::eSampler,
		GlobalResourceLayout::eNone,
		GlobalResourceAccess::eRead,
		std::nullopt,
		binding
	});
}

template <auto &target_ref>
jems::handle resource_intrinsic(const sampler <target_ref> &, uint32_t binding)
{
	// TODO: deal with type and dimension using reference base
	return jems::global_resource(GlobalResource {
		jems::type(primitive_of <float, 2> ()),
		GlobalResourceKind::eSampler,
		GlobalResourceLayout::eNone,
		GlobalResourceAccess::eRead,
		std::nullopt,
		binding
	});
}

// TODO: array slot for global resources
template <typename R, int64_t D>
requires is_global_resource_v <R>
jems::handle resource_intrinsic(const array <R, D> &, uint32_t binding)
{
	auto proxy = R();
	auto handle = resource_intrinsic(proxy, binding);
	static_cast <Reference> (handle)->as <GlobalResource> ().count = D;
	return handle;
}

} // namespace rcgp
