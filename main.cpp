#include <fmt/printf.h>

#include "ugp.hpp"

struct VertexAttributes {
	Topology::Triangle <vec3> position;
	Topology::Triangle <vec3> normal;
	Topology::Triangle <vec3> uv;

	// TODO: intrinscs must be listed as members... (or parameters...)

	$reflection(position, normal, uv);
};

struct RasterForward {
	Position svpos;
	Interpolant::Smooth <vec3> position;
	Interpolant::Smooth <vec3> normal;
	Interpolant::Smooth <vec2> uv;

	$reflection(svpos, position, normal, uv);
};

// RasterForward == FragmentAttributes after discarding Position
// and intrinscs on both sides and including order
struct FragmentAttributes {
	vec3 position;
	vec3 normal;
	vec2 uv;

	// NOTE: Interpolation qualifiers are inferred (should not necessary be re-declared here...)

	$reflection(position, normal, uv);
};

struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	$reflection(model, view, proj);
};

ParameterBlock <MVP> mvp;

struct Sampler2D {
	vec4 sample(vec2 uv) {
		// return vec4(1, 0, 0, 1);
	}

	// TODO: disable copies, etc. unique placeholder ID per instance...
};

struct PBRTextures {
	Sampler2D diffuse;
	Sampler2D specular;
	
	$reflection(diffuse, specular);
};

// TODO: index overwrite using layout... same for vertex input attributes...
ParameterBlock <PBRTextures> textures;

// auto y = $stage(Vertex) $def(RasterForward) ($import(mvp)) {};
// target syntax is def 'Vertex y(&mvp, va: VertexAttributes) -> RasterForward { ... }
// the "&mvp" declaration attaches mvp to the context
// or... $stage(Vertex) [](...) -> RasterForward
// or... $stage(Vertex) $fn(...) -> $returns(RasterForward)
// or... $S(Vertex) $fn(...) -> $R(RasterForward)

template <int>
struct proxy_tag {};

template <typename T>
struct foil {
	using unfoil = T;
};

namespace fn_return_injection {

template <typename T>
struct Reader {
	friend constexpr auto adl_lever(Reader <T>);
};

template <typename T, typename U>
struct Writer {
	friend constexpr auto adl_lever(Reader <T>) {
		return foil <U> {};
	}
};

inline void adl_lever();

template <typename T>
using Read = std::remove_pointer_t <decltype(adl_lever(Reader <T> {}))>;

}

template <Stage, int>
struct _fn_operator {};

template <Stage>
struct _stage_operator {};

#define $stage(C) _stage_operator <Stage::C> () *

#define $vertex		$stage(Vertex)
#define $fragment	$stage(Fragment)
#define $compute	$stage(Compute)

#define $returns(T) decltype(fn_return_injection::Writer <decltype(_return_proxy), RasterForward> {}, void())
#define $return (_return_operator <fn_return_injection::Read <decltype(_return_proxy)> ::unfoil> ()) << 
#define $fn (_fn_operator <Stage::Undefined, __COUNTER__ + 1> ()) << [_return_proxy = proxy_tag <__COUNTER__> ()]
#define $cafn(...) (_fn_operator <Stage::Undefined, __COUNTER__ + 1> ()) << [__VA_ARGS__ __VA_OPT__(,) _return_proxy = proxy_tag <__COUNTER__> ()]

template <Stage S, int I>
auto operator<<(_fn_operator <S, I>, auto lambda)
{
	using R = typename fn_return_injection::Read <proxy_tag <I>> ::unfoil;
	return (_def_operator <S, R> ()) << lambda;
}

template <Stage C, int I>
auto operator*(_stage_operator <C>, _fn_operator <Stage::Undefined, I>)
{
	return _fn_operator <C, I> ();
}

auto z = $vertex $fn(i32 x, $import(mvp)) -> $returns(RasterForward)
{
	$return RasterForward {
		.svpos = vec4(),
		.position = vec3(),
		.normal = vec3(),
		.uv = vec2(),
	};
};

auto f(int y)
{
	return $compute $cafn(&)(i32 x) -> $returns(RasterForward)
	{
		int ww = y;

		$return RasterForward {
			.svpos = vec4(),
			.position = vec3(),
			.normal = vec3(),
			.uv = vec2(),
		};
	};
}

#define $fuse(A, B)

int main()
{
	// TODO: static_reference as a basic unit for testing reference identity
	// static_assert(std::is_same_v <R1, R2>);
	// static_assert(std::is_same_v <R1, R3>,
	//        $ss_format(
	// 		$ss("see the following diagnostic from Javelin:\n\n"
	//        			"\tJavelin: blah blah blah: {}\n"),
	//        		$ss_type_indented(VertexAttributes::reflection, 1)
	//    	).view()
	// );
}
