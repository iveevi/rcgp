#pragma once

#include "../../dsl/generators.hpp"
#include "../../rhi/pipelines.hpp"
#include "../../rhi/shader_compiler.hpp"
#include "../../util/logging.hpp"
#include "../../util/timer.hpp"
#include "../collect_gvrs.hpp"
#include "../collect_streams.hpp"
#include "../group_allocation.hpp"
#include "../implicit_context.hpp"
#include "../input_assembly.hpp"
#include "../pipeline/rasterization.hpp"
#include "../shader_stage.hpp"

template <typename ... Ts>
auto sequence_to_group_allocation(const Tlist <Ts...> &)
{
	return constexpr_for(Is, sizeof...(Ts),
		return Tlist <group_allocation_record <Ts::type::handle, Is>...> {}
	);
}

TYPE_TRAIT(is_push_constant_wrapper);
	template <auto &ref, ShaderStage ... Ss>
	requires (is_push_constant_v <reference_base_t <ref>>)
	TYPE_TRAIT_INCLUDES(is_push_constant_wrapper, stage_wrapper <reference <ref>, Ss...>);

TYPE_TRAIT(is_descriptable_wrapper);
	template <auto &ref, ShaderStage ... Ss>
	requires (not is_push_constant_v <reference_base_t <ref>>)
	TYPE_TRAIT_INCLUDES(is_descriptable_wrapper, stage_wrapper <reference <ref>, Ss...>);

template <typename List>
using push_constant_resources_t = tlist_filter_t <is_push_constant_wrapper, List>;

template <typename List>
using descriptable_resources_t = tlist_filter_t <is_descriptable_wrapper, List>;

// TODO: need to manage names...
// TODO: also need to clean this stuff up...
template <auto &ref, ShaderStage ... Ss>
auto reference_to_descriptor_set_layout(const Device &device, const stage_wrapper <reference <ref>, Ss...> &)
{
	using Reference = reference_base_t <ref>;

	auto stage_flags = (stage_to_flag(Ss) | ...);
	if constexpr (is_resource_group_v <Reference>) {
		using Structure = Reference::value_type;
		static_assert(aggregate <Structure>);

		constexpr size_t bindings = Structure::reflection::field_count;
		std::array <vk::DescriptorSetLayoutBinding, bindings> dslbs {};

		auto fill_one = [&] <size_t I> () {
			using Resource = Structure::reflection::template field_type <I>;

			vk::DescriptorType dtype = vk::DescriptorType::eUniformBuffer;
			if constexpr (is_sampler_v <Resource>) {
				dtype = vk::DescriptorType::eCombinedImageSampler;
			} else if constexpr (is_storage_buffer_v <Resource>) {
				dtype = vk::DescriptorType::eStorageBuffer;
			}

			dslbs[I] = vk::DescriptorSetLayoutBinding()
				.setBinding(I)
				.setDescriptorCount(1)
				.setDescriptorType(dtype)
				.setStageFlags(stage_flags);
		};

		constexpr_for(Is, bindings,
			(fill_one.template operator() <Is> (), ...)
		);

		auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
			.setBindings(dslbs);

		return device.logical.createDescriptorSetLayout(dsl_info);
	} else {
		// TODO: method for generating DescriptorType for global resources
		vk::DescriptorType dtype = vk::DescriptorType::eUniformBuffer;
		if constexpr (is_sampler_v <Reference>) {
			dtype = vk::DescriptorType::eCombinedImageSampler;
		} else if constexpr (is_storage_buffer_v <Reference>) {
			dtype = vk::DescriptorType::eStorageBuffer;
		}

		auto dslbs = std::array {
			vk::DescriptorSetLayoutBinding()
				.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(dtype)
				.setStageFlags(stage_flags)
		};

		auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
			.setBindings(dslbs);

		return device.logical.createDescriptorSetLayout(dsl_info);
	}
}

template <typename ... Ts>
auto reference_sequence_to_descriptor_set_layouts(const Device &device, const Tlist <Ts...> &)
{
	// TODO: separate arrays for dsl and pc ranges
	if constexpr (sizeof...(Ts) == 0) {
		return std::array <vk::DescriptorSetLayout, 0> ();
	} else {
		return std::array {
			reference_to_descriptor_set_layout(device, Ts())...
		};
	}
}

template <auto &ref, ShaderStage ... Ss>
auto reference_to_push_constant_range(const stage_wrapper <reference <ref>, Ss...> &, uint32_t offset)
{
	using info = push_constant_info <stage_wrapper <reference <ref>, Ss...>>;

	auto range = vk::PushConstantRange()
		.setOffset(offset)
		.setSize(info::size)
		.setStageFlags(info::stage_flags);

	return range;
}

template <typename ... Ts>
auto reference_sequence_to_push_constant_ranges(const Tlist <Ts...> &)
{
	if constexpr (sizeof...(Ts) == 0) {
		return std::array <vk::PushConstantRange, 0> ();
	} else {
		std::array <vk::PushConstantRange, sizeof...(Ts)> ranges {};
		uint32_t offset = 0;
		size_t index = 0;

		auto add = [&](auto tag) {
			using info = push_constant_info <decltype(tag)>;
			offset = align_up_u32(offset, info::alignment);
			ranges[index++] = reference_to_push_constant_range(tag, offset);
			offset += info::size;
		};

		(add(Ts()), ...);
		return ranges;
	}
}

template <typename ... Ts>
auto reference_sequence_to_push_constant_allocation_map(const Tlist <Ts...> &)
{
	push_constant_allocation_map map;
	if constexpr (sizeof...(Ts) > 0) {
		uint32_t offset = 0;
		uint32_t index = 0;

		auto add = [&](auto tag) {
			using info = push_constant_info <decltype(tag)>;
			offset = align_up_u32(offset, info::alignment);
			map.emplace(info::addr, PushConstantAllocation { index++, offset });
			offset += info::size;
		};

		(add(Ts()), ...);
	}

	return map;
}

consteval vk::PrimitiveTopology translate_topology(Topology T)
{
	using enum vk::PrimitiveTopology;

	switch (T) {
	case Topology::eTriangleList: return eTriangleList;
	case Topology::eTriangleFan: return eTriangleFan;
	}

	return vk::PrimitiveTopology::eTriangleList;
}

template <auto T>
struct marker {};

template <auto T>
static constexpr auto marker_v = marker <T> {};

template <Topology T, typename ... Subpasses>
struct RasterizationCombinator {
	const marker <T> &topology;
	const Device &device;
	const ShaderCompiler &compiler;
	const RenderPass <Subpasses...> &render_pass;
	RasterizationOptions options;

	template <typename VRet, typename ... As, typename FRet, typename ... Bs>
	auto operator()(
		shader_stage <ShaderStage::eVertex, VRet, As...> &vertex,
		shader_stage <ShaderStage::eFragment, FRet, Bs...> &fragment
	) const {
		TSCOPE("rasterization combinator");

		// TODO: check between vshader output and fshader input
		// TODO: store # of attachments required from # of fshader outputs
		using vertex_icontext = find_implicit_context <As...> ::type;
		using fragment_icontext = find_implicit_context <Bs...> ::type;

		// Collect vertex attribute streams
		auto streams0 = Tlist <> {};
		auto streams = add_stream_from_implicit_context(streams0, vertex_icontext());

		// Generate vertex input bindings and attributes
		auto vertex_bindings = sequence_to_vertex_bindings(streams);
		auto vertex_attributes = sequence_to_vertex_attributes(streams);

		// Collect global resources
		auto gvrs0 = Tlist <> {};
		auto gvrs1 = add_gvr_from_implicit_context <ShaderStage::eVertex> (gvrs0, vertex_icontext());
		auto gvrs = add_gvr_from_implicit_context <ShaderStage::eFragment> (gvrs1, fragment_icontext());

		auto descriptor_gvrs = descriptable_resources_t <decltype(gvrs)> {};
		auto push_constant_gvrs = push_constant_resources_t <decltype(gvrs)> {};

		auto alloc = sequence_to_group_allocation(descriptor_gvrs);
		auto gamap = new_group_allocation_map(alloc);
		vertex->apply_group_allocation_map(gamap);
		fragment->apply_group_allocation_map(gamap);

		auto pcmap = reference_sequence_to_push_constant_allocation_map(push_constant_gvrs);
		vertex->apply_push_constant_allocation_map(pcmap);
		fragment->apply_push_constant_allocation_map(pcmap);

		// Compile the shaders
		auto vshader = generate_glsl(vertex);
		// info("vertex shader:\n%s", vshader.c_str());

		auto fshader = generate_glsl(fragment);
		// info("fragment shader:\n%s", fshader.c_str());
	
		auto vspv = compiler.glsl_to_spirv(vshader, EShLangVertex);
		auto fspv = compiler.glsl_to_spirv(fshader, EShLangFragment);

		auto vertex_shader_module = compiler.spirv_to_shader_module(vspv);
		auto fragment_shader_module = compiler.spirv_to_shader_module(fspv);

		// Generate the pipeline and descriptor set layouts
		auto dsls = reference_sequence_to_descriptor_set_layouts(device, descriptor_gvrs);
		auto pcrs = reference_sequence_to_push_constant_ranges(push_constant_gvrs);

		auto layout_info = vk::PipelineLayoutCreateInfo().setSetLayouts(dsls);
		if constexpr (push_constant_gvrs.size > 0)
			layout_info.setPushConstantRanges(pcrs);
		auto layout = device.logical.createPipelineLayout(layout_info);

		// TODO: stuff above is constant per device & signature; move to a method and cache
		auto pipeline = compile_rasterization_pipeline(
			device,
			render_pass,
			translate_topology(T),
			vertex_shader_module,
			fragment_shader_module,
			"main",
			"main",
			layout,
			vertex_bindings,
			vertex_attributes,
			options
		);

		return AnnotatedRasterizationPipeline <
			T,
			decltype(streams),
			decltype(alloc),
			decltype(gvrs)
		> {
			.device = device.logical,
			.handle = pipeline,
			.layout = layout,
			.dsls = dsls,
		};
	}
};
