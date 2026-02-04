#include "common.hpp"

#define SUITE "recording"

// Recording:
// Testing that our DSL records the correct instructions into the Blocks.

struct RecordingPair {
	f32 x;
	i32 y;

	$reflection(x, y);
};

add_test(scalar_constant)
{
	auto sbr = record {
		i32 a = 3;
	};

	// TODO: match source location as well eventually
	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Local $0
	  $2 = 3
	  Store $1 $2
	}
	)");
};

add_test(binary_op)
{
	auto sbr = record {
		i32 a = 1;
		i32 b = 2;
		i32 c = a + b;
	};

	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Local $0
	  $2 = 1
	  Store $1 $2
	  $3 = Local $0
	  $4 = 2
	  Store $3 $4
	  $5 = Add $1 $3
	}
	)");
};

add_test(unary_op)
{
	auto sbr = record {
		i32 a = 1;
		i32 b = -a;
	};

	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Local $0
	  $2 = 1
	  Store $1 $2
	  $3 = Local $0
	  $4 = -1
	  Store $3 $4
	  $5 = Multiply $3 $1
	}
	)");
};

add_test(assignment_store)
{
	auto sbr = record {
		i32 a;
		i32 b = 7;
		a = b;
	};
	
	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Local $0
	  $2 = Local $0
	  $3 = 7
	  Store $2 $3
	  Store $1 $2
	}
	)");
};

add_test(increment_decrement)
{
	auto sbr = record {
		i32 a = 1;
		++a;
		a++;
		--a;
		a--;
	};
	
	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Local $0
	  $2 = 1
	  Store $1 $2
	  $3 = Local $0
	  $4 = 1
	  Store $3 $4
	  $5 = Add $1 $3
	  Store $1 $5
	  $6 = Local $0
	  $7 = 1
	  Store $6 $7
	  $8 = Add $1 $6
	  Store $1 $8
	  $9 = Local $0
	  $10 = 1
	  Store $9 $10
	  $11 = Subtract $1 $9
	  Store $1 $11
	  $12 = Local $0
	  $13 = 1
	  Store $12 $13
	  $14 = Subtract $1 $12
	  Store $1 $14
	}
	)");
};

add_test(type_caching)
{
	auto sbr = record {
		i32 a;
		i32 b;
	};

	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Local $0
	  $2 = Local $0
	}
	)");
};

add_test(construct_vec3)
{
	auto sbr = record {
		f32 x = 1.0f;
		f32 y = 2.0f;
		f32 z = 3.0f;
		vec3 v(x, y, z);
	};
	
	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Float
	  $1 = Local $0
	  $2 = 1
	  Store $1 $2
	  $3 = Local $0
	  $4 = 2
	  Store $3 $4
	  $5 = Local $0
	  $6 = 3
	  Store $5 $6
	  $7 = Vec3
	  $8 = New $7($1, $3, $5)
	  $9 = Local $7
	  Store $9 $8
	}
	)");
};

add_test(swizzle_xyz)
{
	auto sbr = record {
		vec4 v;
		vec3 xyz = v.xyz;
	};
	
	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Vec4
	  $1 = Local $0
	  $2 = Swizzle($1: xyz)
	}
	)");
};

add_test(field_access)
{
	auto sbr = record {
		RecordingPair p;
		auto type = reconstruct_type <RecordingPair> ();
		inject_reference(p, jems::local(type));
		auto x = p.x;
		auto y = p.y;
		f32 z = x + y;
	};

	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Float
	  $1 = Local $0
	  $2 = Int32
	  $3 = Local $2
	  $4 = RecordingPair { f0: $0, f1: $2 }
	  $5 = Local $4
	  $6 = $5.f0
	  $7 = $5.f1
	  $8 = Add $6 $7
	}
	)");
};

add_test(array_access)
{
	auto sbr = record {
		array <i32, 4> values;
		auto type = reconstruct_type <array <i32, 4>> ();
		inject_reference(values, jems::local(type));
		i32 v = values[2];
	};

	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Int32[4]
	  $2 = Local $1
	  $3 = Local $0
	  $4 = Local $0
	  $5 = 2
	  Store $4 $5
	  $6 = $2[$4]
	}
	)");
};

add_test(while_loop)
{
	auto sbr = record {
		auto typei32 = jems::type(Primitive::eInt32);
		auto typef32 = jems::type(Primitive::eFloat);

		i32 arg0;
		i32 arg1;
		arg0.override_reference(jems::argument(typei32, 0));
		arg1.override_reference(jems::argument(typei32, 1));

		// TODO: tests to ensure that arithmetic is done right...
		f32 sum = 0;
		i32 i = 0;
		$while (i < arg0) {
			sum = sum + i;
			i = i + arg1;
		};

		auto ret = jems::returns(typef32, 0);
		jems::store(ret, sum);
	};
	
	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Float
	  $2 = Local $0
	  $3 = Local $0
	  $4 = Argument 0: $0
	  $5 = Argument 1: $0
	  $6 = Local $1
	  $7 = 0
	  Store $6 $7
	  $8 = Local $0
	  $9 = 0
	  Store $8 $9
	  $10 = Block {
	    $0 = Int32
	    $1 = Float
	    $11 = Less $8 $4
	    $12 = Block {
	      Break
	    }
	    Branch $11: $12
	    $13 = Add $6 $8
	    Store $6 $13
	    $14 = Add $8 $5
	    Store $8 $14
	  }
	  Loop $10
	  $15 = Return 0: $1
	  Store $15 $6
	}
	)");
};

add_test(for_loop)
{
	auto sbr = record {
		auto typei32 = jems::type(Primitive::eInt32);
		auto typef32 = jems::type(Primitive::eFloat);

		i32 arg0;
		i32 arg1;
		arg0.override_reference(jems::argument(typei32, 0));
		arg1.override_reference(jems::argument(typei32, 1));

		f32 sum = 0;
		$for (i32 i = 0, i < arg0, i = i + arg1) {
			sum = sum + i;
		};

		auto ret = jems::returns(typef32, 0);
		jems::store(ret, sum);
	};
	
	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Float
	  $2 = Local $0
	  $3 = Local $0
	  $4 = Argument 0: $0
	  $5 = Argument 1: $0
	  $6 = Local $1
	  $7 = 0
	  Store $6 $7
	  $8 = Local $0
	  $9 = 0
	  Store $8 $9
	  $10 = Block {
	    $0 = Int32
	    $1 = Float
	    $11 = Less $8 $4
	    $12 = Block {
	      Break
	    }
	    Branch $11: $12
	    $13 = Add $6 $8
	    Store $6 $13
	    $14 = Add $8 $5
	    Store $8 $14
	  }
	  Loop $10
	  $15 = Return 0: $1
	  Store $15 $6
	}
	)");
};

add_test(branching)
{
	auto sbr = record {
		i32 c = 12;
		$if (c > 11) {
			// TODO: support for c + 1 plainly
			c = c + i32(1);
		} $elif (c < 11 and c > 5) {
			c = c + i32(2);
		} $else {
			c = c + i32(3);
		};
	};
	
	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	  }
	  $0 = Int32
	  $1 = Local $0
	  $2 = 12
	  Store $1 $2
	  $3 = Local $0
	  $4 = 11
	  Store $3 $4
	  $5 = Less $1 $3
	  $6 = Local $0
	  $7 = 5
	  Store $6 $7
	  $8 = Greater $1 $6
	  $9 = LogicalAnd $5 $8
	  $10 = Local $0
	  $11 = 11
	  Store $10 $11
	  $12 = Greater $1 $10
	  $13 = Block {
	    $0 = Int32
	    $14 = Local $0
	    $15 = 1
	    Store $14 $15
	    $16 = Add $1 $14
	    Store $1 $16
	  }
	  $17 = Block {
	    $0 = Int32
	    $18 = Local $0
	    $19 = 2
	    Store $18 $19
	    $20 = Add $1 $18
	    Store $1 $20
	  }
	  $21 = Block {
	    $0 = Int32
	    $22 = Local $0
	    $23 = 3
	    Store $22 $23
	    $24 = Add $1 $22
	    Store $1 $24
	  }
	  Branch $12: $13, $9: $17, else: $21
	}
	)");
};
