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

// DebugInfo information
struct DebugInfo {
	// TODO: node name
	std::source_location origin;
};

// TODO: should transition to CFG style...
// CFG nodes and a top level Module which include context info
struct Block : std::vector <Reference> {
	std::string name;
	std::vector <Argument> arguments;
	std::vector <Return> returns;
	std::vector <StageInput> stage_inputs;
	std::vector <StageOutput> stage_outputs;
	std::map <void *, Reference> global_resources;
	std::optional <std::array <uint32_t, 3>> workgroup_size;
	std::optional <Reference> task_payload_type;
	std::optional <uint32_t> mesh_max_vertices;
	std::optional <uint32_t> mesh_max_primitives;
	std::optional <MeshPrimitive> mesh_primitive_kind;
	std::map <uint32_t, bool> mesh_perprimitive_outputs;
	uint32_t mesh_output_counter = 0;
	ShaderStage stage = ShaderStage::eSubroutine;
	
	void add_argument(const Argument &arg);
	void add_return(const Return &ret);
	void add_stage_input(const StageInput &tin);
	void add_stage_output(const StageOutput &tout);
	void add_global_resource(void *addr, const Reference &resource);
	void set_workgroup_size(uint32_t x, uint32_t y, uint32_t z);
	void apply_group_allocation_map(const group_allocation_map &map);
	void apply_push_constant_allocation_map(const push_constant_allocation_map &map);

	Reference add(Instruction instruction);

	std::string repr() const;
};

} // namespace rcgp
