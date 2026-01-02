#pragma once

#include "reference.hpp"
#include "mirror.hpp"

// Reference to resource handle
template <auto &rsrc>
using ResourceTypeOf = ResourceType <reference_base_t <rsrc>>;

template <auto &rsrc>
using DataTypeOf = DataType <reference_base_t <rsrc>>;

template <auto &rsrc>
using DynamicDataTypeOf = DynamicDataType <reference_base_t <rsrc>>;
