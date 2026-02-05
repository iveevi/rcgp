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
	    name: recorded,
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
	    name: recorded,
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
	    name: recorded,
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
	    name: recorded,
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
	    name: recorded,
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
	    name: recorded,
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
	    name: recorded,
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
	    name: recorded,
	  }
	  $0 = Vec4
	  $1 = Local $0
	  $2 = Swizzle($1: xyz)
	  $3 = Vec3
	  $4 = Local $3
	  Store $4 $2
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
	    name: recorded,
	  }
	  $0 = Float
	  $1 = Local $0
	  $2 = Int32
	  $3 = Local $2
	  $4 = RecordingPair { x: $0, y: $2 }
	  $5 = Local $4
	  $6 = $5.x
	  $7 = $5.y
	  $8 = Add $6 $7
	  $9 = Local $0
	  Store $9 $8
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
	    name: recorded,
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
	    name: recorded,
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
	    $12 = Bool
	    $13 = Local $12
	    Store $13 $11
	    $14 = LogicalNot $13
	    $15 = Block {
	      Break
	    }
	    Branch $14: $15
	    $16 = Add $6 $8
	    $17 = Local $1
	    Store $17 $16
	    Store $6 $17
	    $18 = Add $8 $5
	    Store $8 $18
	  }
	  Loop $10
	  $19 = Return 0: $1
	  Store $19 $6
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
	    name: recorded,
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
	    $12 = Bool
	    $13 = Local $12
	    Store $13 $11
	    $14 = LogicalNot $13
	    $15 = Block {
	      Break
	    }
	    Branch $14: $15
	    $16 = Add $6 $8
	    $17 = Local $1
	    Store $17 $16
	    Store $6 $17
	    $18 = Add $8 $5
	    Store $8 $18
	  }
	  Loop $10
	  $19 = Return 0: $1
	  Store $19 $6
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
	    name: recorded,
	  }
	  $0 = Int32
	  $1 = Local $0
	  $2 = 12
	  Store $1 $2
	  $3 = Local $0
	  $4 = 11
	  Store $3 $4
	  $5 = Less $1 $3
	  $6 = Bool
	  $7 = Local $6
	  Store $7 $5
	  $8 = Local $0
	  $9 = 5
	  Store $8 $9
	  $10 = Greater $1 $8
	  $11 = Local $6
	  Store $11 $10
	  $12 = LogicalAnd $7 $11
	  $13 = Local $6
	  Store $13 $12
	  $14 = Local $0
	  $15 = 11
	  Store $14 $15
	  $16 = Greater $1 $14
	  $17 = Local $6
	  Store $17 $16
	  $18 = Block {
	    $0 = Int32
	    $19 = Local $0
	    $20 = 1
	    Store $19 $20
	    $21 = Add $1 $19
	    Store $1 $21
	  }
	  $22 = Block {
	    $0 = Int32
	    $23 = Local $0
	    $24 = 2
	    Store $23 $24
	    $25 = Add $1 $23
	    Store $1 $25
	  }
	  $26 = Block {
	    $0 = Int32
	    $27 = Local $0
	    $28 = 3
	    Store $27 $28
	    $29 = Add $1 $27
	    Store $1 $29
	  }
	  Branch $17: $18, $13: $22, else: $26
	}
	)");
};

add_test(vector_store)
{
	auto sbr = record {
		float2 pos = float2(1) - 1;
		pos = float2(pos.x, -pos.y);
	};

	assert_assembly_match(sbr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	    name: recorded,
	  }
	  $0 = Float
	  $1 = Local $0
	  $2 = 1
	  Store $1 $2
	  $3 = Vec2
	  $4 = New $3($1, $1)
	  $5 = Local $3
	  Store $5 $4
	  $6 = Local $0
	  $7 = 1
	  Store $6 $7
	  $8 = Subtract $5 $6
	  $9 = Local $3
	  Store $9 $8
	  $10 = Swizzle($9: y)
	  $11 = Local $0
	  Store $11 $10
	  $12 = Local $0
	  $13 = -1
	  Store $12 $13
	  $14 = Multiply $12 $11
	  $15 = Swizzle($9: x)
	  $16 = Local $0
	  Store $16 $15
	  $17 = New $3($16, $14)
	  $18 = Local $3
	  Store $18 $17
	  Store $9 $18
	}
	)");
};
