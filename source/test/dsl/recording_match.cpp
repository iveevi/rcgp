static bool match_ref(
	const Reference &actual,
	const Reference &expected,
	const std::unordered_map <const Instruction *, const Instruction *> &map
)
{
	if (!expected)
		return actual.get() == nullptr;
	auto it = map.find(expected.get());
	if (it == map.end())
		return false;
	return it->second == actual.get();
}

static bool match_type(
	const Type &actual,
	const Type &expected,
	const std::unordered_map <const Instruction *, const Instruction *> &map
);

static bool match_primitive(const PrimitiveType &actual, const PrimitiveType &expected)
{
	return actual.index() == expected.index();
}

static bool match_constant(const Constant &actual, const Constant &expected)
{
	if (actual.index() != expected.index())
		return false;
	return std::visit([](const auto &a, const auto &b) {
		if constexpr (std::is_same_v <std::decay_t <decltype(a)>, std::decay_t <decltype(b)>>)
			return a == b;
		else
			return false;
	}, actual, expected);
}

static bool match_type(
	const Type &actual,
	const Type &expected,
	const std::unordered_map <const Instruction *, const Instruction *> &map
)
{
	if (actual.index() != expected.index())
		return false;
	vswitch (actual) {
	vcase(PrimitiveType): {
		return match_primitive(actual.as <PrimitiveType> (), expected.as <PrimitiveType> ());
	}
	vcase(ArrayType): {
		const auto &a = actual.as <ArrayType> ();
		const auto &b = expected.as <ArrayType> ();
		if (a.size != b.size)
			return false;
		return match_ref(a.base, b.base, map);
	}
	vcase(AggregateType): {
		const auto &a = actual.as <AggregateType> ();
		const auto &b = expected.as <AggregateType> ();
		if (a.name != b.name || a.size() != b.size())
			return false;
		for (size_t i = 0; i < a.size(); i++) {
			if (!match_ref(a[i], b[i], map))
				return false;
		}
		return true;
	}
	default:
		break;
	}
	return false;
}

static bool match_instruction(
	const Instruction &actual,
	const Instruction &expected,
	const std::unordered_map <const Instruction *, const Instruction *> &map
)
{
	if (actual.index() != expected.index())
		return false;
	vswitch (actual) {
	vcase(Constant): {
		return match_constant(actual.as <Constant> (), expected.as <Constant> ());
	}
	vcase(Operation): {
		const auto &a = actual.as <Operation> ();
		const auto &b = expected.as <Operation> ();
		return a.code == b.code
			&& match_ref(a.a, b.a, map)
			&& match_ref(a.b, b.b, map);
	}
	vcase(Construct): {
		const auto &a = actual.as <Construct> ();
		const auto &b = expected.as <Construct> ();
		if (!match_ref(a.type, b.type, map) || a.args.size() != b.args.size())
			return false;
		for (size_t i = 0; i < a.args.size(); i++) {
			if (!match_ref(a.args[i], b.args[i], map))
				return false;
		}
		return true;
	}
	vcase(Local): {
		const auto &a = actual.as <Local> ();
		const auto &b = expected.as <Local> ();
		return match_ref(a.type, b.type, map);
	}
	vcase(Store): {
		const auto &a = actual.as <Store> ();
		const auto &b = expected.as <Store> ();
		return match_ref(a.destination, b.destination, map)
			&& match_ref(a.source, b.source, map);
	}
	vcase(Swizzle): {
		const auto &a = actual.as <Swizzle> ();
		const auto &b = expected.as <Swizzle> ();
		return a.code == b.code && match_ref(a.value, b.value, map);
	}
	vcase(FieldAccess): {
		const auto &a = actual.as <FieldAccess> ();
		const auto &b = expected.as <FieldAccess> ();
		return a.fidx == b.fidx && match_ref(a.value, b.value, map);
	}
	vcase(ArrayAccess): {
		const auto &a = actual.as <ArrayAccess> ();
		const auto &b = expected.as <ArrayAccess> ();
		return match_ref(a.value, b.value, map)
			&& match_ref(a.index, b.index, map);
	}
	vcase(Type): {
		return match_type(actual.as <Type> (), expected.as <Type> (), map);
	}
	default:
		break;
	}
	return false;
}

static bool match_block(const Block &actual, const Block &expected)
{
	if (actual.size() != expected.size())
		return false;
	std::unordered_map <const Instruction *, const Instruction *> map;
	map.reserve(expected.size());
	for (size_t i = 0; i < actual.size(); i++) {
		const auto &a = actual[i];
		const auto &b = expected[i];
		if (!a || !b)
			return a.get() == b.get();
		if (!match_instruction(*a, *b, map))
			return false;
		map.emplace(b.get(), a.get());
	}
	return true;
}

static void assert_block_sequence(
	const char *label,
	const SharedBlockReference &actual,
	const SharedBlockReference &expected
)
{
	if (actual.get() == nullptr || expected.get() == nullptr) {
		assertion(false, "expected blocks to be created");
		return;
	}
	if (match_block(*actual, *expected))
		return;
	info("%s actual:\n%s", label, generate_assembly(actual).c_str());
	info("%s expected:\n%s", label, generate_assembly(expected).c_str());
	assertion(false, "expected %s sequence to match", label);
}
