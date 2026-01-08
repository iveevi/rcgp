#pragma once

#include <type_traits>

#include "scalar.hpp"
#include "local.hpp"

template <native_scalar T, size_t N, size_t M>
class matrix : public jems::handle {
	explicit matrix(const jems::handle &h) : handle(h) {}
public:
	using reflection = primitive_reflection <matrix <T, N, M>>;
	DEFINE_REFLECTION_STAMP();

	matrix() {
		if constexpr (std::is_same_v <T, float>
			&& ((N == 3 && M == 3) || (N == 4 && M == 4))) {
			if (Tracer::singleton.records.empty())
				return;
			auto type = jems::type_loc(std::source_location::current(), MatrixType <T, N, M> ());
			init_local_if_tracing(*this, type);
		}
	}

	template <size_t A, size_t B>
	explicit matrix(const matrix <T, A, B> &other)
		: handle(jems::construct(
			jems::type(MatrixType <T, N, M> ()),
			other
		)) {}

	static auto reinterpret(const jems::handle &h) {
		return matrix(h);
	}

	matrix &operator=(const matrix &rhs) {
		if (Tracer::singleton.records.empty()) {
			Reference &self_ref = *this;
			const Reference &rhs_ref = rhs;
			self_ref = rhs_ref;
			return *this;
		}

		// TODO: this is wierd, fix later
		if constexpr (std::is_same_v <T, float>
			&& ((N == 3 && M == 3) || (N == 4 && M == 4))) {
			auto type = jems::type_loc(std::source_location::current(), MatrixType <T, N, M> ());
			assign_or_store(*this, rhs, type);
		} else {
			Reference &self_ref = *this;
			const Reference &rhs_ref = rhs;
			self_ref = rhs_ref;
		}
		return *this;
	}
	
	// Arithmetic operations
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend matrix operator*(const U &s, const matrix &m) {
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (s), m));
	}

	friend matrix operator-(const matrix &m) {
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (-1), m));
	}
};

extern template class matrix <int32_t, 2, 2>;
extern template class matrix <int32_t, 3, 3>;
extern template class matrix <int32_t, 4, 4>;
extern template class matrix <uint32_t, 2, 2>;
extern template class matrix <uint32_t, 3, 3>;
extern template class matrix <uint32_t, 4, 4>;
extern template class matrix <float, 2, 2>;
extern template class matrix <float, 3, 3>;
extern template class matrix <float, 4, 4>;
