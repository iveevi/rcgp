#include <format>

#include <fmt/format.h>

#include "dsl/instructions.hpp"

namespace rcgp {

std::string Type::repr() const
{
	vswitch ((*this)) {
	vcase(Primitive): {
		const auto &primitive = as <Primitive> ();
		return std::string(rcgp::repr(primitive));
	}
	vcase(Struct): {
		const auto &agg = as <Struct> ();
		std::string repr = "Struct(";
		for (size_t i = 0; i < agg.size(); i++) {
			if (i)
				repr += ", ";
			repr += agg[i] ? agg[i]->repr() : "null";
		}
		repr += ")";
		return repr;
	}
	vcase(Array): {
		const auto &array = as <Array> ();
		auto base_repr = array.base ? array.base->repr() : "null";
		return "Array(size=" + std::to_string(array.size) + " base=" + base_repr + ")";
	}
	default:
		break;
	}

	return "Type(unknown)";
}

std::string Constant::repr() const
{
	return std::visit([](auto x) {
		return fmt::format("{}", x);
	}, *this);
}

std::string Operation::repr() const
{
	return fmt::format("Operation({})", rcgp::repr(code));
}

std::string Construct::repr() const
{
	return "Construct";
}

std::string Invocation::repr() const
{
	return "Invocation";
}

std::string Branch::repr() const
{
	return "Branch";
}

std::string Loop::repr() const
{
	return "Loop";
}

std::string Local::repr() const
{
	return "Local";
}

std::string BuiltinIntrinsic::repr() const
{
	return std::format("BuiltinIntrinsic({})", rcgp::repr(code));
}

std::string Swizzle::repr() const
{
	return std::format("Swizzle({})", rcgp::repr(code));
}

std::string FieldAccess::repr() const
{
	return "FieldAccess";
}

std::string ArrayAccess::repr() const
{
	return "ArrayAccess";
}

std::string Store::repr() const
{
	return "Store";
}

std::string GlobalResource::repr() const
{
	return std::format("GlobalResource({}, {})", rcgp::repr(kind), rcgp::repr(layout));
}

std::string Argument::repr() const
{
	return std::format("Argument({})", argi);
}

std::string StageInput::repr() const
{
	return std::format("StageInput({})", argi);
}

std::string StageOutput::repr() const
{
	return std::format("StageOutput({}, {})", argi, rcgp::repr(properties));
}

std::string Return::repr() const
{
	return std::format("Return({})", argi);
}

} // namespace rcgp
