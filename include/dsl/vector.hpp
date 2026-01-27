#pragma once

#include <type_traits>

#include "vector_base.hpp"

namespace rcgp {

template <native_scalar T, size_t N>
struct vector : public vector_base <T, N> {
	using vector_base <T, N> ::vector_base;
	
	static auto reinterpret(const jems::handle &h) {
		return vector(h);
	}

	vector &operator=(const vector &rhs) {
		if (Tracer::singleton.records.empty()) {
			Reference &self_ref = *this;
			const Reference &rhs_ref = rhs;
			self_ref = rhs_ref;
			return *this;
		}

		auto type = jems::type_loc(std::source_location::current(), VectorType <T, N> ());
		assign_or_store(*this, rhs, type);
		return *this;
	}

	// Arithmetic operations
	// TODO: move to a header/generate with a script?
	friend vector operator+(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eAdd, a, b));
	}
	
	friend vector operator*(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eMultiply, a, b));
	}

	friend vector operator-(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eSubtract, a, b));
	}

	friend vector operator-(const vector &v) {
		// TODO: negative operation
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (-1), v));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator+(const U &s, const vector &v) {
		return reinterpret(jems::operation(OperationCode::eAdd, scalar <T> (s), v));
	}
	
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator+(const vector &v, const U &s) {
		return reinterpret(jems::operation(OperationCode::eAdd, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator-(const vector &v, const U &s) {
		return reinterpret(jems::operation(OperationCode::eSubtract, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator-(const U &s, const vector &v) {
		return reinterpret(jems::operation(OperationCode::eSubtract, scalar <T> (s), v));
	}
	
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator*(const U &s, const vector &v) {
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (s), v));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator*(const vector &v, const U &s) {
		return reinterpret(jems::operation(OperationCode::eMultiply, v, scalar <T> (s)));
	}

	friend vector operator/(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eDivide, a, b));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator/(const vector &v, const U &s) {
		return reinterpret(jems::operation(OperationCode::eDivide, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator/(const U &s, const vector &v) {
		return reinterpret(jems::operation(OperationCode::eDivide, scalar <T> (s), v));
	}
};

static_assert(sizeof(vector <int32_t, 2>) == sizeof(jems::handle));
static_assert(sizeof(vector <uint32_t, 3>) == sizeof(jems::handle));
static_assert(sizeof(vector <float, 4>) == sizeof(jems::handle));

// TODO: separate header with all the specializations
extern template struct vector <int32_t, 2>;
extern template struct vector <int32_t, 3>;
extern template struct vector <int32_t, 4>;
extern template struct vector <uint32_t, 2>;
extern template struct vector <uint32_t, 3>;
extern template struct vector <uint32_t, 4>;
extern template struct vector <float, 2>;
extern template struct vector <float, 3>;
extern template struct vector <float, 4>;

} // namespace rcgp
