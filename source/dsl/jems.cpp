#include "dsl/jems.hpp"
#include "dsl/block.hpp"
#include "dsl/instructions.hpp"

namespace rcgp::jems {

scope::scope(const SharedBlockReference &sbr)
{
	Tracer::singleton.records.emplace(sbr);
}

scope::~scope()
{
	Tracer::singleton.records.pop();
}

scope::operator bool() const
{
	return true;
}

void null::override_reference(const Reference &) {}

handle::handle(const Reference &ref_) : _ref(ref_) {}

void handle::override_reference(const Reference &ref_)
{
	_ref = ref_;
}

handle::operator Reference &()
{
	return _ref;
}

handle::operator const Reference &() const
{
	return _ref;
}

operation::operation(
	OperationCode code,
	const Reference &a,
	const Reference &b,
	const std::source_location &loc
) : handle($tsb.add(Instruction(Operation { code, a, b }, DebugInfo(loc)))) {}

constant::constant(const Constant &value, const std::source_location &loc)
	: handle($tsb.add(Instruction(value, DebugInfo(loc)))) {}

invocation::invocation(
	const SharedBlockReference &sbr,
	const std::vector <Reference> &args,
	const std::vector <Reference> &returns,
	const std::source_location &loc
) : handle($tsb.add(Instruction(Invocation { sbr, args, returns }, DebugInfo(loc)))) {}

construct::construct(
	const Reference &type,
	const std::vector <Reference> &args,
	const std::source_location &loc
) : handle($tsb.add(Instruction(Construct { type, args }, DebugInfo(loc)))) {}

argument::argument(
	const Reference &type,
	uint32_t argi,
	const std::source_location &loc
) : handle($tsb.add(Instruction(Argument { type, argi }, DebugInfo(loc)))) {}

returns::returns(
	const Reference &type,
	uint32_t argi,
	const std::source_location &loc
) : handle($tsb.add(Instruction(Return { type, argi }, DebugInfo(loc)))) {}

stage_input::stage_input(
	const Reference &type,
	uint32_t argi,
	RateProperties properties,
	const std::source_location &loc
) : handle($tsb.add(Instruction(StageInput { type, argi, properties }, DebugInfo(loc)))) {}

stage_output::stage_output(
	const Reference &type,
	uint32_t argi,
	RateProperties properties,
	const std::source_location &loc
) : handle($tsb.add(Instruction(StageOutput { type, argi, properties }, DebugInfo(loc)))) {}

global_resource::global_resource(
	const Reference &type,
	GlobalResourceKind kind,
	GlobalResourceLayout layout,
	GlobalResourceAccess access,
	std::optional <uint32_t> group,
	std::optional <uint32_t> index,
	std::optional <uint32_t> offset,
	const std::source_location &loc
) : handle($tsb.add(Instruction(
	GlobalResource { type, kind, layout, access, group, index, offset },
	DebugInfo(loc)
))) {}

system_value::system_value(SystemValue value, const std::source_location &loc)
	: handle($tsb.add(Instruction(value, DebugInfo(loc)))) {}

builtin_intrinsic::builtin_intrinsic(
	BuiltinIntrinsicCode code,
	const std::vector <Reference> &args,
	const std::source_location &loc
) : handle($tsb.add(Instruction(BuiltinIntrinsic { code, args }, DebugInfo(loc)))) {}

swizzle::swizzle(
	SwizzleCode code,
	const Reference &value,
	const std::source_location &loc
) : handle($tsb.add(Instruction(Swizzle { code, value }, DebugInfo(loc)))) {}

array_access::array_access(
	const Reference &value,
	const Reference &index,
	const std::source_location &loc
) : handle($tsb.add(Instruction(ArrayAccess { value, index }, DebugInfo(loc)))) {}

field_access::field_access(
	const Reference &value,
	uint32_t fidx,
	const std::source_location &loc
) : handle($tsb.add(Instruction(FieldAccess { value, fidx }, DebugInfo(loc)))) {}

store::store(
	const Reference &destination,
	const Reference &source,
	const std::source_location &loc
) : handle($tsb.add(Instruction(Store { destination, source }, DebugInfo(loc)))) {}

local::local(const Reference &type, const std::source_location &loc)
	: handle($tsb.add(Instruction(Local { type }, DebugInfo(loc)))) {}

loop::loop(const SharedBlockReference &body, const std::source_location &loc)
	: handle($tsb.add(Instruction(Loop { body }, DebugInfo(loc)))) {}

type::type(const Type &t, const std::source_location &loc)
{
	auto key = t.repr();
	auto &cache = Tracer::singleton.type_cache;
	if (auto it = cache.find(key); it != cache.end()) {
		_ref = it->second;
		auto &blk = Tracer::singleton.active();
		bool already_active = false;
		for (const auto &existing : blk) {
			if (existing == _ref) {
				already_active = true;
				break;
			}
		}
		if (not already_active)
			blk.insert(blk.begin(), _ref);
		return;
	}
	_ref = $tsb.add(Instruction(t, DebugInfo(loc)));
	cache.emplace(key, _ref);
}

} // namespace rcgp::jems
