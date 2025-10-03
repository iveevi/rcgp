#pragma once

// Adapted from LuisaCompute: https://github.com/LuisaGroup/LuisaCompute/blob/stable/include/luisa/core/macro.h
#define MACRO_EVAL0(...) __VA_ARGS__
#define MACRO_EVAL1(...) MACRO_EVAL0(MACRO_EVAL0(MACRO_EVAL0(__VA_ARGS__)))
#define MACRO_EVAL2(...) MACRO_EVAL1(MACRO_EVAL1(MACRO_EVAL1(__VA_ARGS__)))
#define MACRO_EVAL3(...) MACRO_EVAL2(MACRO_EVAL2(MACRO_EVAL2(__VA_ARGS__)))
#define MACRO_EVAL(...) MACRO_EVAL3(__VA_ARGS__)

#define MACRO_EMPTY()
#define MACRO_DEFER(id) id MACRO_EMPTY()

#define MAP_END(...)
#define MAP_OUT
#define MAP_GET_END2() 0, MAP_END
#define MAP_GET_END1(...) MAP_GET_END2
#define MAP_GET_END(...) MAP_GET_END1
#define MAP_NEXT0(test, next, ...) next MAP_OUT
#define MAP_NEXT1(test, next) MACRO_DEFER(MAP_NEXT0)(test, next, 0)
#define MAP_NEXT(test, next) MAP_NEXT1(MAP_GET_END test, next)

// MAP(f, context, ...) - Apply function f to each variadic argument with context
#define MAP(f, context, ...) MACRO_EVAL(MAP1(f, context, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define MAP0(f, context, x, peek, ...) f(context, x) MACRO_DEFER(MAP_NEXT(peek, MAP1))(f, context, peek, __VA_ARGS__)
#define MAP1(f, context, x, peek, ...) f(context, x) MACRO_DEFER(MAP_NEXT(peek, MAP0))(f, context, peek, __VA_ARGS__)
