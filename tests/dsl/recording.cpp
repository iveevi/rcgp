#include "common.hpp"

using namespace rcgp;

struct RecordingPair {
	f32 x;
	i32 y;

	$reflection(x, y);
};

auto operator*(std::nullptr_t, std::function <void ()> &&fn)
{
	Tracer::singleton.type_cache.clear();
	auto sbr = std::make_shared <Block> ();
	{
		jems::scope scope(sbr);
		fn();
	}
	return sbr;
}

#define record nullptr * [&]

add_test(scalar_constant)
{
	auto sbr = record {
		i32 a = 3;
	};

	assert_assembly_match(sbr, R"(
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = i32
	  $1 = local $0
	  $2 = 3
	  store $1 $2
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
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = i32
	  $1 = local $0
	  $2 = 1
	  store $1 $2
	  $3 = local $0
	  $4 = 2
	  store $3 $4
	  $5 = add($1, $3)
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
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = i32
	  $1 = local $0
	  $2 = 1
	  store $1 $2
	  $3 = local $0
	  $4 = -1
	  store $3 $4
	  $5 = mul($3, $1)
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
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = i32
	  $1 = local $0
	  $2 = local $0
	  $3 = 7
	  store $2 $3
	  store $1 $2
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
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = i32
	  $1 = local $0
	  $2 = 1
	  store $1 $2
	  $3 = local $0
	  $4 = 1
	  store $3 $4
	  $5 = add($1, $3)
	  store $1 $5
	  $6 = local $0
	  $7 = 1
	  store $6 $7
	  $8 = add($1, $6)
	  store $1 $8
	  $9 = local $0
	  $10 = 1
	  store $9 $10
	  $11 = sub($1, $9)
	  store $1 $11
	  $12 = local $0
	  $13 = 1
	  store $12 $13
	  $14 = sub($1, $12)
	  store $1 $14
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
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = i32
	  $1 = local $0
	  $2 = local $0
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
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = f32
	  $1 = local $0
	  $2 = 1
	  store $1 $2
	  $3 = local $0
	  $4 = 2
	  store $3 $4
	  $5 = local $0
	  $6 = 3
	  store $5 $6
	  $7 = float3
	  $8 = new $7($1, $3, $5)
	  $9 = local $7
	  store $9 $8
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
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = float4
	  $1 = local $0
	  $2 = swizzle($1, xyz)
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
	};

	assert_assembly_match(sbr, R"(
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = f32
	  $1 = local $0
	  $2 = i32
	  $3 = local $2
	  $4 = RecordingPair($0, $2)
	  $5 = local $4
	  $6 = field $5:0
	  $7 = field $5:1
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
	block {
	  context {
	    model: subroutine,
	  }
	  $0 = i32
	  $1 = array($0, 4)
	  $2 = local $1
	  $3 = local $0
	  $4 = local $0
	  $5 = 2
	  store $4 $5
	  $6 = index($2, $4)
	}
	)");
};
