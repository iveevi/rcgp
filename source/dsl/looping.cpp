#include "dsl/branching.hpp"
#include "dsl/block.hpp"
#include "dsl/jems.hpp"
#include "dsl/looping.hpp"

namespace rcgp {

_loop_holder::~_loop_holder()
{
	auto sbr = std::make_shared <Block> ();
	if (auto s = jems::scope(sbr)) {
		boolean b = cond();
		$if (not b) { $break; };
		body();
		if (step)
			(*step)();
	}

	auto l = jems::loop(sbr);
}

} // namespace rcgp
