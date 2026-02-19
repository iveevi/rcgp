#include "common.hpp"

#define SUITE "recording_verbose"

// Recording (verbose):
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
	)", true);
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
	  $6 = Local $0
	  Store $6 $5
	}
	)", true);
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
	  $6 = Local $0
	  Store $6 $5
	}
	)", true);
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
	  $2 = 7
	  Store $1 $2
	  $3 = Local $0
	  Store $3 $1
	}
	)", true);
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
	  $6 = Local $0
	  Store $6 $5
	  Store $1 $6
	  $7 = Local $0
	  $8 = 1
	  Store $7 $8
	  $9 = Add $1 $7
	  $10 = Local $0
	  Store $10 $9
	  Store $1 $10
	  $11 = Local $0
	  $12 = 1
	  Store $11 $12
	  $13 = Subtract $1 $11
	  $14 = Local $0
	  Store $14 $13
	  Store $1 $14
	  $15 = Local $0
	  $16 = 1
	  Store $15 $16
	  $17 = Subtract $1 $15
	  $18 = Local $0
	  Store $18 $17
	  Store $1 $18
	}
	)", true);
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
	}
	)", true);
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
	)", true);
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
	  $1 = Swizzle($0: xyz)
	  $2 = Vec3
	  $3 = Local $2
	  Store $3 $1
	}
	)", true);
};

add_test(field_access)
{
	auto sbr = record {
		RecordingPair p;
		auto type = reconstruct_type <RecordingPair> ();
		p.override_reference(jems::local(type));
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
	  $1 = Int32
	  $2 = RecordingPair { x: $0, y: $1 }
	  $3 = Local $2
	  $4 = $3.x
	  $5 = $3.y
	  $6 = Add $4 $5
	  $7 = Local $0
	  Store $7 $6
	}
	)", true);
};

add_test(array_access)
{
	auto sbr = record {
		array <i32, 4> values;
		auto type = reconstruct_type <array <i32, 4>> ();
		values.override_reference(jems::local(type));
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
	  $4 = 2
	  Store $3 $4
	  $5 = $2[$3]
	}
	)", true);
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
	  $2 = Argument 0: $0
	  $3 = Argument 1: $0
	  $4 = Local $1
	  $5 = 0
	  Store $4 $5
	  $6 = Local $0
	  $7 = 0
	  Store $6 $7
	  $8 = Block {
	    $0 = Int32
	    $1 = Float
	    $9 = Less $6 $2
	    $10 = Bool
	    $11 = Local $10
	    Store $11 $9
	    $12 = LogicalNot $11
	    $13 = Local $10
	    Store $13 $12
	    $14 = Block {
	      Break
	    }
	    Branch $13: $14
	    $15 = Add $4 $6
	    $16 = Local $1
	    Store $16 $15
	    Store $4 $16
	    $17 = Add $6 $3
	    $18 = Local $0
	    Store $18 $17
	    Store $6 $18
	  }
	  Loop $8
	  $19 = Return 0: $1
	  Store $19 $4
	}
	)", true);
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
	  $2 = Argument 0: $0
	  $3 = Argument 1: $0
	  $4 = Local $1
	  $5 = 0
	  Store $4 $5
	  $6 = Local $0
	  $7 = 0
	  Store $6 $7
	  $8 = Block {
	    $0 = Int32
	    $1 = Float
	    $9 = Less $6 $2
	    $10 = Bool
	    $11 = Local $10
	    Store $11 $9
	    $12 = LogicalNot $11
	    $13 = Local $10
	    Store $13 $12
	    $14 = Block {
	      Break
	    }
	    Branch $13: $14
	    $15 = Add $4 $6
	    $16 = Local $1
	    Store $16 $15
	    Store $4 $16
	    $17 = Add $6 $3
	    $18 = Local $0
	    Store $18 $17
	    Store $6 $18
	  }
	  Loop $8
	  $19 = Return 0: $1
	  Store $19 $4
	}
	)", true);
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
	    $22 = Local $0
	    Store $22 $21
	    Store $1 $22
	  }
	  $23 = Block {
	    $0 = Int32
	    $24 = Local $0
	    $25 = 2
	    Store $24 $25
	    $26 = Add $1 $24
	    $27 = Local $0
	    Store $27 $26
	    Store $1 $27
	  }
	  $28 = Block {
	    $0 = Int32
	    $29 = Local $0
	    $30 = 3
	    Store $29 $30
	    $31 = Add $1 $29
	    $32 = Local $0
	    Store $32 $31
	    Store $1 $32
	  }
	  Branch $17: $18, $13: $23, else: $28
	}
	)", true);
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
	  $15 = Local $0
	  Store $15 $14
	  $16 = Swizzle($9: x)
	  $17 = Local $0
	  Store $17 $16
	  $18 = New $3($17, $15)
	  $19 = Local $3
	  Store $19 $18
	  Store $9 $19
	}
	)", true);
};
