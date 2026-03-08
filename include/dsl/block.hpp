#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

#include "instruction_nodes.hpp"

namespace rcgp {

// GRV to group index
using group_allocation_map = std::map <void *, uint32_t>;

// Push constant to offset
using push_constant_allocation_map = std::map <void *, uint32_t>;

// TODO: should transition to CFG style...
// CFG nodes and a top level Module which include context info
struct Block : std::vector <Reference> {
	ShaderStage stage = ShaderStage::eSubroutine;
	std::map <uint32_t, bool> mesh_perprimitive_outputs;
	std::map <void *, std::vector <Reference>> global_resources;
	std::optional <MeshPrimitive> mesh_primitive_kind;
	std::optional <Reference> hit_attribute_type;
	std::optional <Reference> task_payload_type;
	std::optional <std::array <uint32_t, 3>> workgroup_size;
	std::optional <uint32_t> mesh_max_primitives;
	std::optional <uint32_t> mesh_max_vertices;
	std::string name;
	std::vector <Argument> arguments;
	std::vector <Return> returns;
	std::vector <StageInput> stage_inputs;
	std::vector <StageOutput> stage_outputs;
	uint32_t mesh_output_counter = 0;
	
	void add_argument(const Argument &arg);
	void add_return(const Return &ret);
	void add_stage_input(const StageInput &tin);
	void add_stage_output(const StageOutput &tout);
	void add_global_resource(void *addr, const Reference &resource);
	void set_workgroup_size(uint32_t x, uint32_t y, uint32_t z);
	void apply_group_allocation_map(const group_allocation_map &map);
	void apply_push_constant_allocation_map(const push_constant_allocation_map &map);

	Reference add(const Instruction &instr);
	Reference add(size_t index, const Instruction &instr);

	std::string repr() const;
};

} // namespace rcgp
