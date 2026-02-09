#include "dsl/instructions.hpp"
#include "dsl/block.hpp"
#include "util/error.hpp"

namespace rcgp {

const Type &get_type(const SharedBlockReference &sbr, const Reference &ref)
{
	vswitch (*ref) {
	vcase(Local): {
		auto &local = ref->as <Local> ();
		return get_type(sbr, local.type);
	}
	vcase(Type): {
		return ref->as <Type> ();
	}
	vcase(GlobalResource): {
		auto &grsrc = ref->as <GlobalResource> ();
		assertion(grsrc.kind == GlobalResourceKind::ePushConstant
			or grsrc.kind == GlobalResourceKind::eStorageBuffer);
		return get_type(sbr, grsrc.type);
	}
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		auto &type = get_type(sbr, aacc.value);
		assertion(type.is <Array> ());
		return get_type(sbr, type.as <Array> ().base);
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		auto &type = get_type(sbr, facc.value);
		assertion(type.is <Struct> ());
		return get_type(sbr, type.as <Struct>().at(facc.fidx));
	}
	vcase(SystemValue): {
		auto &sv = ref->as <SystemValue> ();
		switch (sv) {
		case SystemValue::eTaskPayload:
			return get_type(sbr, sbr->task_payload_type.value());
		default:
			break;
		}

		break;
	}
	default:
		break;
	}

	fatal("unhandled instruction in get_type: {}", ref->repr());
}

const Struct &get_struct(const SharedBlockReference &sbr, const Reference &ref)
{
	auto &type = get_type(sbr, ref);
	return type.as <Struct> ();
}

} // namespace rcgp
