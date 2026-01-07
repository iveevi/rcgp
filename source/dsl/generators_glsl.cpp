#include "dsl/generators_glsl.hpp"

namespace generators {

GLSL::GLSL(const Block &block_)
	: block(block_),
	reference(*this),
	expression(*this),
	statement(*this),
	type(*this)
{
}

std::string GLSL::layout_string(GlobalResourceLayout layout) const
{
	switch (layout) {
	case GlobalResourceLayout::eScalar: return "scalar, ";
	case GlobalResourceLayout::eStd430: return "std430, ";
	case GlobalResourceLayout::eUnknown:
	default:
		return "";
	}
}

std::string GLSL::generate_reference::impl(GlobalIntrinsic gi)
{
	switch (gi) {
	case GlobalIntrinsic::eScreenPosition: return "gl_Position";
	case GlobalIntrinsic::eInstanceIndex: return "gl_InstanceIndex";
	default:
		return "?";
	}
}

std::string GLSL::generate_reference::impl(GlobalResource grsrc)
{
	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
		auto idx = grsrc.push_constant_index.value_or(0);
		return fmt::format("pc{}", idx);
	}

	return fmt::format("r{}_i{}",
		grsrc.group.value_or(-1),
		grsrc.index.value_or(-1));
}

std::string GLSL::generate_reference::impl(ThreadOutput tout)
{
	return fmt::format("lout{}", tout.argi);
}

std::string GLSL::generate_reference::main(Reference reference)
{
	auto ftn = [&](auto x) { return impl(x); };
	return std::visit(ftn, *reference);
}

std::string GLSL::generate_reference::operator()(Reference r)
{
	return main(r);
}

std::string GLSL::generate_expression::impl(Constant value)
{
	vswitch (value) {
	vcase(int): return fmt::format("{}", value.as <int32_t> ());
	vcase(float): return fmt::format("{}", value.as <float> ());
	default:
		return "?";
	}
}

std::string GLSL::generate_expression::impl(Operation operation)
{
	std::string op = "?";
	switch (operation.code) {
	case OperationCode::eAdd: op = "+"; break;
	case OperationCode::eSubtract: op = "-"; break;
	case OperationCode::eMultiply: op = "*"; break;
	case OperationCode::eDivide: op = "/"; break;
	default:
		break;
	}

	return fmt::format("({} {} {})",
		main(operation.a), op, main(operation.b));
}

std::string GLSL::generate_expression::impl(ThreadInput tin)
{
	return parent.thread_inputs[tin.argi];
}

std::string GLSL::generate_expression::impl(Construct construct)
{
	std::string out = parent.type.main(construct.type) + "(";

	auto nargs = construct.args.size();
	for (size_t i = 0; i < nargs; i++) {
		out += main(construct.args[i]);
		if (i + 1 < nargs)
			out += ", ";
	}

	return out + ")";
}

std::string GLSL::generate_expression::impl(FieldAccess access)
{
	return fmt::format("{}.f{}", main(access.value), access.fidx);
}

std::string GLSL::generate_expression::impl(ArrayAccess access)
{
	return fmt::format("{}[{}]", main(access.value), main(access.index));
}

std::string GLSL::generate_expression::impl(Swizzle swizzle)
{
	return fmt::format("{}.{}",
		main(swizzle.value), swizzle_string(swizzle.code));
}

std::string GLSL::generate_expression::impl(GlobalResource grsrc)
{
	return parent.reference.impl(grsrc);
}

std::string GLSL::generate_expression::impl(GlobalIntrinsic intrinsic)
{
	return parent.reference.impl(intrinsic);
}

std::string GLSL::generate_expression::impl(const BuiltinIntrinsic &builtin)
{
	std::string out = "?";

	switch (builtin.code) {
	case BuiltinIntrinsicCode::eCross: out = "cross"; break;
	case BuiltinIntrinsicCode::eDFdx: out = "dFdx"; break;
	case BuiltinIntrinsicCode::eDFdxFine: out = "dFdxFine"; break;
	case BuiltinIntrinsicCode::eDFdy: out = "dFdy"; break;
	case BuiltinIntrinsicCode::eDFdyFine: out = "dFdyFine"; break;
	case BuiltinIntrinsicCode::eDot: out = "dot"; break;
	case BuiltinIntrinsicCode::eInverse: out = "inverse"; break;
	case BuiltinIntrinsicCode::eMax: out = "max"; break;
	case BuiltinIntrinsicCode::eNormalize: out = "normalize"; break;
	case BuiltinIntrinsicCode::eSample: out = "texture"; break;
	case BuiltinIntrinsicCode::eTranspose: out = "transpose"; break;
	default:
		break;
	}

	out += "(";

	auto nargs = builtin.args.size();
	for (size_t i = 0; i < nargs; i++) {
		out += main(builtin.args[i]);
		if (i + 1 < nargs)
			out += ", ";
	}

	return out + ")";
}

std::string GLSL::generate_expression::main(Reference expression)
{
	auto ftn = [&](auto x) { return impl(x); };
	return std::visit(ftn, *expression);
}

std::string GLSL::generate_expression::operator()(Reference r)
{
	return main(r);
}

void GLSL::generate_statement::impl(Store store)
{
	parent.result += fmt::format("    {} = {};\n",
		parent.reference(store.destination),
		parent.expression(store.source));
}

void GLSL::generate_statement::main(Reference instruction)
{
	auto ftn = [&](auto x) { return impl(x); };
	return std::visit(ftn, *instruction);
}

void GLSL::generate_statement::operator()(Reference r)
{
	main(r);
}

std::string GLSL::generate_type::impl(const PrimitiveType &primitive)
{
	vswitch (primitive) {
	vcase(int): return "int";
	vcase(float): return "float";
	vcase(VectorType <float, 2>): return "vec2";
	vcase(VectorType <float, 3>): return "vec3";
	vcase(VectorType <float, 4>): return "vec4";
	vcase(MatrixType <float, 3, 3>): return "mat3";
	vcase(MatrixType <float, 4, 4>): return "mat4";
	default:
		break;
	}

	return "?";
}

std::string GLSL::generate_type::impl(const Type &type)
{
	return std::visit([&](const auto &x) { return impl(x); }, type);
}

std::string GLSL::generate_type::impl(const ArrayType &array)
{
	auto size = (array.size > 0)
		? fmt::format("[{}]", array.size)
		: "[]";

	return main(array.base) + size;
}

std::string GLSL::generate_type::impl(const AggregateType &aggregate)
{
	auto addr = &aggregate;
	if (parent.aggregate_names.contains(addr))
		return parent.aggregate_names[addr];
	return "?";
}

std::string GLSL::generate_type::main(Reference type)
{
	return std::visit([&](const auto &x) { return impl(x); }, *type);
}

std::string GLSL::generate_type::operator()(Reference r)
{
	return main(r);
}

std::string GLSL::generate(size_t tabs)
{
	if (block.context.model != ExecutionModel::eAgnostic) {
		result = "#version 460\n";

		result += "#extension GL_EXT_scalar_block_layout : require\n\n";

		result += "\n";

		size_t aggregate_counter = 0;
		for (auto &instr : block) {
			if (not instr->is <Type> ())
				continue;

			auto &type = instr->as <Type> ();
			if (not type.is <AggregateType> ())
				continue;

			auto &agg = type.as <AggregateType> ();

			result += fmt::format("struct AGG{} {{\n", aggregate_counter++);

			size_t field_counter = 0;
			for (auto &field : agg) {
				result += fmt::format("    {} f{};\n",
					this->type.main(field), field_counter++);
			}

			result += "};\n\n";

			aggregate_names.emplace(&agg, fmt::format("AGG{}", aggregate_counter - 1));
		}

		thread_inputs.reserve(block.context.thread_inputs.size());
		for (auto &tin : block.context.thread_inputs) {
			thread_inputs.push_back(fmt::format("lin{}", tin.argi));
			result += fmt::format("layout (location = {}) in {} lin{};\n",
				tin.argi, type.main(tin.type), tin.argi);
		}

		if (block.context.thread_inputs.size())
			result += "\n";

		for (auto &tout : block.context.thread_outputs) {
			std::string qualifier = "? ";
			switch (tout.properties) {
			case RateProperties::eFlat: qualifier = "flat "; break;
			case RateProperties::eNone: qualifier = ""; break;
			case RateProperties::eNoPerspective: qualifier = "noperspective "; break;
			case RateProperties::eSmooth: qualifier = "smooth "; break;
			default:
				break;
			}

			result += fmt::format("layout (location = {}) {}out {} lout{};\n",
				tout.argi, qualifier, type.main(tout.type), tout.argi);
		}

		if (block.context.thread_outputs.size())
			result += "\n";

		uint32_t pcounter = 0;

		for (auto &[_, refs] : block.context.global_resources) {
			for (auto &ref : refs) {
				auto &grsrc = ref->as <GlobalResource> ();
				if (grsrc.kind == GlobalResourceKind::eSampler) {
					auto group = grsrc.group.value_or(-1);
					auto index = grsrc.index.value_or(-1);
					result += fmt::format("layout (set = {}, binding = {}) uniform sampler2D r{}_i{};\n\n",
						group, index, group, index);
				} else if (grsrc.kind == GlobalResourceKind::ePushConstant) {
					auto idx = grsrc.push_constant_index.value_or(pcounter++);
					grsrc.push_constant_index = idx;
					auto offset = grsrc.push_constant_offset.value_or(0);

					result += fmt::format("layout ({}push_constant) uniform PC{} {{\n",
						layout_string(grsrc.layout), idx);
					result += fmt::format("    layout (offset = {}) {} pc{};\n", offset, type.main(grsrc.type), idx);
					result += "};\n\n";
				} else {
					std::string modifier;
					switch (grsrc.kind) {
					case GlobalResourceKind::eUniformBuffer: modifier = "uniform"; break;
					case GlobalResourceKind::eStorageBuffer: modifier = "buffer"; break;
					default:
						fatal("unsupported global resource kind");
					}

					auto group = grsrc.group.value_or(-1);
					auto index = grsrc.index.value_or(-1);
					result += fmt::format("layout ({}set = {}, binding = {}) {} R{}{} {{\n",
						layout_string(grsrc.layout), group, index, modifier, group, index);
					result += fmt::format("    {} r{}_i{};\n", type.main(grsrc.type), group, index);
					result += "};\n\n";
				}
			}
		}

		result += "void main()\n";
		result += "{\n";

		std::vector <Reference> manifest;

		for (auto &instr : block) {
			vswitch ((*instr)) {
			vcase(Store):
				manifest.push_back(instr);
				break;
			default:
				break;
			}
		}

		for (auto &instr : manifest)
			statement(instr);

		result += "}";

		return result;
	} else {
		fatal("per-function generation is not supported");
	}
}

} // namespace generators
