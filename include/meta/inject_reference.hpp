#pragma once

#include "resources.hpp"

namespace rcgp {

// DSL handles
template <typename T>
requires aggregate <T> or primitive <T>
void inject_reference(T &value, Reference ref)
{
	value.override_reference(ref);
}

// TODO: this applies to all buffer handles...
template <typename T, template <typename> typename L>
void inject_reference(PushConstant <T, L> &value, Reference ref)
{
	return inject_reference(Tas <T &> (value), ref);
}

template <typename T, template <typename> typename L, GlobalResourceAccess A>
void inject_reference(StorageBuffer <T, L, A> &value, Reference ref)
{
	return inject_reference(Tas <T &> (value), ref);
}

template <typename T, template <typename> typename L>
void inject_reference(UniformBuffer <T, L> &value, Reference ref)
{
	return inject_reference(Tas <T &> (value), ref);
}

} // namespace rcgp
