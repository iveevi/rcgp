#pragma once

#include "jems.hpp"

inline void init_local_if_tracing(jems::handle &self, const Reference &type_ref)
{
	if (!Tracer::singleton.records.empty())
		self.override_reference(jems::local(type_ref));
}

inline void assign_or_store(jems::handle &self, const jems::handle &rhs, const Reference &type_ref)
{
	// TODO: this is wierd overall, fix later
	Reference &self_ref = self;
	const Reference &rhs_ref = rhs;

	if (Tracer::singleton.records.empty()) {
		self_ref = rhs_ref;
		return;
	}

	if (!self_ref && rhs_ref) {
		auto local = jems::local(type_ref);
		jems::store(local, rhs_ref);
		self_ref = local;
		return;
	}

	if (self_ref && rhs_ref) {
		jems::store(self_ref, rhs_ref);
		return;
	}

	self_ref = rhs_ref;
}
