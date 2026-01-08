#include "dsl/looping.hpp"
#include "util/logging.hpp"

_loop_holder::~_loop_holder()
{
	auto trace_body = [](const _loop_body &body) {
		auto sbr = std::make_shared <Block> ();
		if (auto s = jems::scope(sbr)) {
			body();
		}
		return sbr;
	};

	auto trace_cond = [](const _loop_cond &cond, Reference &cond_value) {
		auto sbr = std::make_shared <Block> ();
		if (auto s = jems::scope(sbr)) {
			auto value = cond();
			cond_value = value;
		}
		return sbr;
	};

	std::optional <SharedBlockReference> init_block;
	if (init.has_value())
		init_block = trace_body(init.value());

	Reference cond_value;
	auto cond_block = trace_cond(cond, cond_value);

	std::optional <SharedBlockReference> step_block;
	if (step.has_value())
		step_block = trace_body(step.value());

	auto body_block = trace_body(body);

	if (not cond_value)
		fatal("loop condition produced no value");

	Tracer::singleton.active().add(Loop {
		kind,
		std::move(init_block),
		std::move(cond_block),
		std::move(cond_value),
		std::move(step_block),
		std::move(body_block),
	}, Debug {});
}
