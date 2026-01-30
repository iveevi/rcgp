#include <rcgp.hpp>
#include <util/logging.hpp>

#include "dsl/recording.cpp"

using namespace rcgp;

// TODO: switch to pytest for compile tests?
int main()
{
	test_recording_scalar_constant();
	test_recording_binary_op_add();
	test_recording_unary_op_negate();
	test_recording_assignment_store();
	test_recording_increment_decrement();
	test_recording_type_caching();
	test_recording_construct_vec3();
	test_recording_swizzle_xyz();
	test_recording_field_access();
	test_recording_array_access();
	ok("dsl recording tests passed");
}
