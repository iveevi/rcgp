#include "dsl/generators_glsl.hpp"

#include <fmt/format.h>

namespace generators {

namespace {

bool is_same_type(const Reference &a, const Reference &b)
{
	if (a.get() == b.get())
		return true;
	if (!a || !b)
		return false;
	if (!a->is <Type> () || !b->is <Type> ())
		return false;

	auto &ta = a->as <Type> ();
	auto &tb = b->as <Type> ();
	if (ta.index() != tb.index())
		return false;

	vswitch (ta) {
	vcase(PrimitiveType):
		return ta.as <PrimitiveType> ().index() == tb.as <PrimitiveType> ().index();
	vcase(AggregateType): {
		auto &aa = ta.as <AggregateType> ();
		auto &bb = tb.as <AggregateType> ();
		if (aa.size() != bb.size())
			return false;
		for (size_t i = 0; i < aa.size(); i++) {
			if (!is_same_type(aa[i], bb[i]))
				return false;
		}
		return true;
	}
	vcase(ArrayType): {
		auto &aa = ta.as <ArrayType> ();
		auto &bb = tb.as <ArrayType> ();
		if (aa.size != bb.size)
			return false;
		return is_same_type(aa.base, bb.base);
	}
	default:
		break;
	}

	return false;
}

std::string rate_qualifier(RateProperties properties)
{
	switch (properties) {
	case RateProperties::eFlat: return "flat ";
	case RateProperties::eNone: return "";
	case RateProperties::eNoPerspective: return "noperspective ";
	case RateProperties::eSmooth: return "smooth ";
	default:
		break;
	}

	return "? ";
}

} // namespace

GLSL::GLSL(const SharedBlockReference &sbr)
	: block(*sbr.get()),
	reference(*this),
	expression(*this),
	statement(*this),
	type(*this) {}

std::string GLSL::layout_string(GlobalResourceLayout layout) const
{
	switch (layout) {
	case GlobalResourceLayout::eScalar: return "scalar, ";
	case GlobalResourceLayout::eStd430: return "std430, ";
	case GlobalResourceLayout::eNone:
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
	if (parent.active_block
		&& parent.active_block->context.model == ShaderStage::eSubroutine
		&& parent.local_thread_outputs.contains(tout.argi)) {
		return parent.local_thread_outputs[tout.argi];
	}

	return fmt::format("lout{}", tout.argi);
}

std::string GLSL::generate_reference::impl(Argument arg)
{
	if (arg.argi < parent.argument_names.size())
		return parent.argument_names[arg.argi];
	return fmt::format("arg{}", arg.argi);
}

std::string GLSL::generate_reference::impl(Local local, Reference ref)
{
	auto ptr = ref.get();
	auto it = parent.local_names.find(ptr);
	if (it != parent.local_names.end())
		return it->second;

	auto name = fmt::format("lvar{}", parent.local_count++);
	parent.local_names.emplace(ptr, name);
	return name;
}

std::string GLSL::generate_reference::main(Reference reference)
{
	if (reference->is <Local> ())
		return impl(reference->as <Local> (), reference);

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
	vcase(int32_t): return fmt::format("{}", value.as <int32_t> ());
	vcase(uint32_t): return fmt::format("{}", value.as <uint32_t> ());
	vcase(bool): return fmt::format("{}", value.as <bool> ());
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
	case OperationCode::eEqual: op = "=="; break;
	case OperationCode::eNotEqual: op = "!="; break;
	case OperationCode::eLess: op = "<"; break;
	case OperationCode::eLessEqual: op = "<="; break;
	case OperationCode::eGreater: op = ">"; break;
	case OperationCode::eGreaterEqual: op = ">="; break;
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

std::string GLSL::generate_expression::impl(Argument arg)
{
	if (arg.argi < parent.argument_names.size())
		return parent.argument_names[arg.argi];
	return fmt::format("arg{}", arg.argi);
}

std::string GLSL::generate_expression::impl(Invocation invocation)
{
	const Block *callee = invocation.sbr.get();
	auto it = parent.subroutine_names.find(callee);
	std::string name = (it != parent.subroutine_names.end())
		? it->second
		: fmt::format("fn_{}", (void *) callee);

	std::string out = name + "(";
	auto nargs = invocation.args.size();
	for (size_t i = 0; i < nargs; i++) {
		out += main(invocation.args[i]);
		if (i + 1 < nargs)
			out += ", ";
	}

	return out + ")";
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
	if (expression->is <Local> ())
		return parent.reference(expression);

	auto ftn = [&](auto x) { return impl(x); };
	return std::visit(ftn, *expression);
}

std::string GLSL::generate_expression::operator()(Reference r)
{
	return main(r);
}

void GLSL::generate_statement::impl(Store store)
{
	parent.result += fmt::format("{}{} = {};\n",
		parent.indent,
		parent.reference(store.destination),
		parent.expression(store.source));
}

void GLSL::generate_statement::impl(Invocation invocation)
{
	parent.result += fmt::format("{}{};\n",
		parent.indent,
		parent.expression.impl(invocation));
}

void GLSL::generate_statement::impl(Branch branch)
{
	auto emit_body = [&](const SharedBlockReference &blk) {
		auto prev = parent.indent;
		parent.indent += "    ";
		parent.emit_block_statements(*blk);
		parent.indent = prev;
	};

	for (size_t i = 0; i < branch.segments.size(); i++) {
		auto &segment = branch.segments[i];
		auto kw = (i == 0) ? "if" : "else if";
		parent.result += fmt::format("{}{} ({})\n",
			parent.indent, kw, parent.expression(segment.cond));
		parent.result += fmt::format("{}{{\n", parent.indent);
		emit_body(segment.body);
		parent.result += fmt::format("{}}}\n", parent.indent);
	}

	if (branch.fallback.has_value()) {
		parent.result += fmt::format("{}else\n", parent.indent);
		parent.result += fmt::format("{}{{\n", parent.indent);
		emit_body(branch.fallback.value());
		parent.result += fmt::format("{}}}\n", parent.indent);
	}
}

void GLSL::generate_statement::impl(Loop loop)
{
	auto emit_body = [&](const SharedBlockReference &blk) {
		auto prev = parent.indent;
		parent.indent += "    ";
		parent.emit_block_statements(*blk);
		parent.indent = prev;
	};

	auto cond_expr = parent.expression(loop.cond_value);

	if (loop.kind == LoopKind::eFor && loop.init.has_value())
		parent.emit_block_statements(*loop.init.value());

	auto loop_kw = (loop.kind == LoopKind::eFor) ? "for (;;)" : "while (true)";
	parent.result += fmt::format("{}{}\n", parent.indent, loop_kw);
	parent.result += fmt::format("{}{{\n", parent.indent);
	emit_body(loop.cond);
	parent.result += fmt::format("{}    if (!({}))\n", parent.indent, cond_expr);
	parent.result += fmt::format("{}        break;\n", parent.indent);
	emit_body(loop.body);
	if (loop.kind == LoopKind::eFor && loop.step.has_value())
		emit_body(loop.step.value());
	parent.result += fmt::format("{}}}\n", parent.indent);
}

void GLSL::generate_statement::impl(Local local, Reference ref)
{
	auto name = parent.reference(ref);
	parent.result += fmt::format("{}{} {};\n",
		parent.indent, parent.type.main(local.type), name);
}

void GLSL::generate_statement::main(Reference instruction)
{
	if (instruction->is <Local> ()) {
		impl(instruction->as <Local> (), instruction);
		return;
	}
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
	vcase(int32_t): return "int";
	vcase(uint32_t): return "uint";
	vcase(float): return "float";
	vcase(bool): return "bool";
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

void GLSL::set_active_block(const Block &blk)
{
	active_block = &blk;
}

void GLSL::reset_state()
{
	result.clear();
	aggregate_names.clear();
	subroutine_names.clear();
	emitted_subroutines.clear();
	thread_inputs.clear();
	argument_names.clear();
	local_thread_outputs.clear();
	local_names.clear();
	local_count = 0;
	set_active_block(block);
}

void GLSL::collect_blocks(std::vector <const Block *> &blocks) const
{
	std::set <const Block *> visited;

	auto visit = [&](auto &&self, const Block &blk) -> void {
		if (visited.contains(&blk))
			return;
		visited.emplace(&blk);
		blocks.push_back(&blk);

		for (auto &instr : blk) {
			if (instr->is <Invocation> ()) {
				auto &inv = instr->as <Invocation> ();
				self(self, *inv.sbr);
			} else if (instr->is <Branch> ()) {
				auto &branch = instr->as <Branch> ();
				for (auto &segment : branch.segments)
					self(self, *segment.body);
				if (branch.fallback.has_value())
					self(self, *branch.fallback.value());
			} else if (instr->is <Loop> ()) {
				auto &loop = instr->as <Loop> ();
				if (loop.init.has_value())
					self(self, *loop.init.value());
				self(self, *loop.cond);
				if (loop.step.has_value())
					self(self, *loop.step.value());
				self(self, *loop.body);
			}
		}
	};

	visit(visit, block);
}

void GLSL::emit_preamble()
{
	result = "// Preamble\n";
	result += "#version 460\n\n";
	result += "#extension GL_EXT_scalar_block_layout : require\n\n";
}

void GLSL::emit_aggregate_decls()
{
	result += "// Types\n";

	std::vector <const Block *> blocks;
	collect_blocks(blocks);

	size_t aggregate_counter = 0;
	for (auto *blk : blocks) {
		for (auto &instr : *blk) {
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
	}

	result += "\n";
}

void GLSL::emit_thread_inputs()
{
	result += "// Inputs\n";

	thread_inputs.reserve(block.context.thread_inputs.size());
	for (auto &tin : block.context.thread_inputs) {
		thread_inputs.push_back(fmt::format("lin{}", tin.argi));
		result += fmt::format("layout (location = {}) in {} lin{};\n",
			tin.argi, type.main(tin.type), tin.argi);
	}

	if (block.context.thread_inputs.size())
		result += "\n";
	else
		result += "\n";
}

void GLSL::emit_thread_outputs()
{
	result += "// Outputs\n";

	std::map <uint32_t, ThreadOutput> outputs;
	std::vector <const Block *> blocks;
	collect_blocks(blocks);

	for (auto *blk : blocks) {
		for (auto &tout : blk->context.thread_outputs) {
			auto it = outputs.find(tout.argi);
			if (it == outputs.end()) {
				outputs.emplace(tout.argi, tout);
				continue;
			}

			if (!is_same_type(it->second.type, tout.type)
				|| it->second.properties != tout.properties)
				fatal("thread output mismatch for location %u", tout.argi);
		}
	}

	for (auto &[_, tout] : outputs) {
		auto qualifier = rate_qualifier(tout.properties);
		result += fmt::format("layout (location = {}) {}out {} lout{};\n",
			tout.argi, qualifier, type.main(tout.type), tout.argi);
	}

	if (outputs.size())
		result += "\n";
	else
		result += "\n";
}

void GLSL::emit_global_resources()
{
	std::vector <const Block *> blocks;
	collect_blocks(blocks);

	std::map <void *, uint32_t> pc_indices;
	collect_push_constant_indices(blocks, pc_indices);

	result += "// Resources\n";

	std::set <std::string> emitted;
	for (auto *blk : blocks) {
		for (auto &[_, refs] : blk->context.global_resources) {
			for (auto &ref : refs) {
				auto &grsrc = ref->as <GlobalResource> ();
				auto key = resource_key(grsrc);
				if (emitted.contains(key))
					continue;
				emitted.emplace(key);
				emit_resource_decl(grsrc);
			}
		}
	}

	result += "\n";
}

void GLSL::collect_push_constant_indices(const std::vector <const Block *> &blocks,
	std::map <void *, uint32_t> &pc_indices)
{
	uint32_t pcounter = 0;
	for (auto *blk : blocks) {
		for (auto &[addr, refs] : blk->context.global_resources) {
			for (auto &ref : refs) {
				auto &grsrc = ref->as <GlobalResource> ();
				if (grsrc.kind != GlobalResourceKind::ePushConstant)
					continue;

				auto it = pc_indices.find(addr);
				if (it == pc_indices.end())
					it = pc_indices.emplace(addr, pcounter++).first;
				grsrc.push_constant_index = it->second;
			}
		}
	}
}

std::string GLSL::resource_key(const GlobalResource &grsrc) const
{
	if (grsrc.kind == GlobalResourceKind::eSampler) {
		auto group = grsrc.group.value_or(-1);
		auto index = grsrc.index.value_or(-1);
		return fmt::format("sampler:{}:{}", group, index);
	}

	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
		auto idx = grsrc.push_constant_index.value_or(0);
		auto offset = grsrc.push_constant_offset.value_or(0);
		return fmt::format("pc:{}:{}:{}:{}",
			idx, offset, (int) grsrc.layout, (void *) grsrc.type.get());
	}

	auto group = grsrc.group.value_or(-1);
	auto index = grsrc.index.value_or(-1);
	return fmt::format("buf:{}:{}:{}:{}",
		(int) grsrc.kind, group, index, (int) grsrc.layout);
}

void GLSL::emit_resource_decl(GlobalResource &grsrc)
{
	if (grsrc.kind == GlobalResourceKind::eSampler) {
		auto group = grsrc.group.value_or(-1);
		auto index = grsrc.index.value_or(-1);
		result += fmt::format("layout (set = {}, binding = {}) uniform sampler2D r{}_i{};\n\n",
			group, index, group, index);
		return;
	}

	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
		auto idx = grsrc.push_constant_index.value_or(0);
		grsrc.push_constant_index = idx;
		auto offset = grsrc.push_constant_offset.value_or(0);

		result += fmt::format("layout ({}push_constant) uniform PC{} {{\n",
			layout_string(grsrc.layout), idx);
		result += fmt::format("    layout (offset = {}) {} pc{};\n", offset, type.main(grsrc.type), idx);
		result += "};\n\n";
		return;
	}

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

void GLSL::emit_block_statements(const Block &blk)
{
	for (auto &instr : blk) {
		if (instr->is <Store> () || instr->is <Invocation> ()
			|| instr->is <Branch> () || instr->is <Loop> ()
			|| instr->is <Local> ())
			statement(instr);
	}
}

std::string GLSL::subroutine_return_type(const Block &blk, uint32_t &out_argi)
{
	out_argi = 0;
	if (blk.context.thread_outputs.empty())
		return "void";
	if (blk.context.thread_outputs.size() > 1)
		fatal("subroutine return with multiple outputs is not supported");

	auto &tout = blk.context.thread_outputs.front();
	out_argi = tout.argi;
	return type.main(tout.type);
}

void GLSL::emit_subroutine_function(const Block &blk, const std::string &name)
{
	set_active_block(blk);
	argument_names.clear();
	local_thread_outputs.clear();

	for (auto &arg : blk.context.arguments)
		argument_names.push_back(fmt::format("arg{}", arg.argi));

	uint32_t ret_argi = 0;
	auto return_type = subroutine_return_type(blk, ret_argi);

	result += fmt::format("{} {}(", return_type, name);
	for (size_t i = 0; i < blk.context.arguments.size(); i++) {
		auto &arg = blk.context.arguments[i];
		result += fmt::format("{} {}", type.main(arg.type), argument_names[arg.argi]);
		if (i + 1 < blk.context.arguments.size())
			result += ", ";
	}
	result += ")\n{\n";

	// TODO: should just be one set of returns...
	// for perf maybe even prefer out parameters for tuples
	for (auto &tout : blk.context.thread_outputs) {
		auto name = fmt::format("lout{}", tout.argi);
		local_thread_outputs.emplace(tout.argi, name);
		result += fmt::format("    {} {};\n", type.main(tout.type), name);
	}

	if (blk.context.thread_outputs.size())
		result += "\n";

	emit_block_statements(blk);

	if (return_type != "void") {
		auto name_it = local_thread_outputs.find(ret_argi);
		auto local_name = (name_it != local_thread_outputs.end())
			? name_it->second
			: fmt::format("lout{}", ret_argi);
		result += fmt::format("    return {};\n", local_name);
	}

	result += "}\n\n";
	set_active_block(block);
}

void GLSL::emit_subroutine_functions()
{
	result += "// Subroutines\n";

	std::vector <const Block *> order;
	std::set <const Block *> visited;

	auto visit = [&](auto &&self, const Block &blk) -> void {
		for (auto &instr : blk) {
			if (instr->is <Invocation> ()) {
				auto &inv = instr->as <Invocation> ();
				const Block *callee = inv.sbr.get();
				if (visited.contains(callee))
					continue;
				visited.emplace(callee);
				self(self, *callee);
				order.push_back(callee);
			}
		}
	};

	visit(visit, block);

	for (auto *callee : order) {
		if (callee->context.model != ShaderStage::eSubroutine)
			continue;

		auto it = subroutine_names.find(callee);
		if (it == subroutine_names.end()) {
			auto name = fmt::format("fn_{}", (void *) callee);
			it = subroutine_names.emplace(callee, name).first;
		}

		if (emitted_subroutines.contains(callee))
			continue;

		emitted_subroutines.emplace(callee);
		emit_subroutine_function(*callee, it->second);
	}
}

void GLSL::emit_main_function()
{
	result += "// Main\n";
	result += "void main()\n";
	result += "{\n";

	emit_block_statements(block);

	result += "}";
}

std::string GLSL::generate(size_t tabs)
{
	reset_state();

	if (block.context.model == ShaderStage::eSubroutine) {
		emit_aggregate_decls();
		auto name = fmt::format("fn_{}", (void *) &block);
		subroutine_names.emplace(&block, name);
		emit_subroutine_function(block, name);
		return result;
	}

	emit_preamble();
	emit_aggregate_decls();
	emit_thread_inputs();
	emit_thread_outputs();
	emit_global_resources();
	emit_subroutine_functions();
	emit_main_function();

	return result;
}

} // namespace generators
