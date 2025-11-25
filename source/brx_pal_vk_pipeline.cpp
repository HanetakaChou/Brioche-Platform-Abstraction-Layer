//
// Copyright (C) YuqiaoZhang(HanetakaChou)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "brx_pal_vk_device.h"
#include <assert.h>

brx_pal_vk_graphics_pipeline::brx_pal_vk_graphics_pipeline() : m_pipeline(VK_NULL_HANDLE)
{
}

void brx_pal_vk_graphics_pipeline::init(PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, VkAllocationCallbacks const *allocation_callbacks, brx_pal_render_pass const *wrapped_render_pass, brx_pal_pipeline_layout const *wrapped_pipeline_layout, size_t vertex_shader_module_code_size, void const *vertex_shader_module_code, size_t fragment_shader_module_code_size, void const *fragment_shader_module_code, bool enable_back_face_cull, bool front_ccw, bool enable_depth_clip, uint32_t forced_sample_count, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION wrapped_depth_compare_operation, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION wrapped_blend_operation)
{
	PFN_vkCreateShaderModule const pfn_create_shader_module = reinterpret_cast<PFN_vkCreateShaderModule>(pfn_get_device_proc_addr(device, "vkCreateShaderModule"));
	assert(pfn_create_shader_module);
	PFN_vkDestroyShaderModule const pfn_destroy_shader_module = reinterpret_cast<PFN_vkDestroyShaderModule>(pfn_get_device_proc_addr(device, "vkDestroyShaderModule"));
	assert(pfn_destroy_shader_module);
	PFN_vkCreateGraphicsPipelines const pfn_create_graphics_pipelines = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(pfn_get_device_proc_addr(device, "vkCreateGraphicsPipelines"));
	assert(pfn_create_graphics_pipelines);

	// NOTE: single subpass is enough
	// input attachment is NOT necessary
	// use VK_ARM_rasterization_order_attachment_access (VK_EXT_rasterization_order_attachment_access) instead
	constexpr uint32_t const subpass_index = 0U;

	assert(NULL != wrapped_render_pass);
	VkRenderPass render_pass = static_cast<brx_pal_vk_render_pass const *>(wrapped_render_pass)->get_render_pass();

	assert(NULL != wrapped_pipeline_layout);
	VkPipelineLayout pipeline_layout = static_cast<brx_pal_vk_pipeline_layout const *>(wrapped_pipeline_layout)->get_pipeline_layout();

	VkShaderModule vertex_shader_module = VK_NULL_HANDLE;
	{
		VkShaderModuleCreateInfo const shader_module_create_info = {
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			NULL,
			0U,
			vertex_shader_module_code_size,
			static_cast<uint32_t const *>(vertex_shader_module_code)};

		VkResult const res_create_shader_module = pfn_create_shader_module(device, &shader_module_create_info, NULL, &vertex_shader_module);
		assert(VK_SUCCESS == res_create_shader_module);
	}

	VkShaderModule fragment_shader_module = VK_NULL_HANDLE;
	{
		VkShaderModuleCreateInfo const shader_module_create_info = {
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			NULL,
			0U,
			fragment_shader_module_code_size,
			static_cast<uint32_t const *>(fragment_shader_module_code)};

		VkResult const res_create_shader_module = pfn_create_shader_module(device, &shader_module_create_info, NULL, &fragment_shader_module);
		assert(VK_SUCCESS == res_create_shader_module);
	}

	VkPipelineShaderStageCreateInfo const stages[2] =
		{
			{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			 NULL,
			 0U,
			 VK_SHADER_STAGE_VERTEX_BIT,
			 vertex_shader_module,
			 "main",
			 NULL},
			{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			 NULL,
			 0U,
			 VK_SHADER_STAGE_FRAGMENT_BIT,
			 fragment_shader_module,
			 "main",
			 NULL}};

	VkPipelineVertexInputStateCreateInfo const vertex_input_state = {
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		NULL,
		0U,
		0U,
		NULL,
		0U,
		NULL};

	VkPipelineInputAssemblyStateCreateInfo const input_assembly_state = {
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		NULL,
		0U,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE};

	VkPipelineViewportStateCreateInfo const viewport_state = {
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		NULL,
		0U,
		1U,
		NULL,
		1U,
		NULL};

	VkPipelineRasterizationDepthClipStateCreateInfoEXT const rasterization_state_depth_clip_state = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT,
		NULL,
		0U,
		enable_depth_clip ? VK_TRUE : VK_FALSE};

	VkPipelineRasterizationStateCreateInfo const rasterization_state = {
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		&rasterization_state_depth_clip_state,
		0U,
		VK_FALSE,
		VK_FALSE,
		VK_POLYGON_MODE_FILL,
		enable_back_face_cull ? static_cast<VkCullModeFlags>(VK_CULL_MODE_BACK_BIT) : static_cast<VkCullModeFlags>(VK_CULL_MODE_NONE),
		front_ccw ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,
		VK_FALSE,
		0.0F,
		0.0F,
		0.0F,
		1.0F};

	VkSampleCountFlagBits rasterization_samples;
	switch (forced_sample_count)
	{
	case 1:
		rasterization_samples = VK_SAMPLE_COUNT_1_BIT;
		break;
	case 2:
		rasterization_samples = VK_SAMPLE_COUNT_2_BIT;
		break;
	case 4:
		rasterization_samples = VK_SAMPLE_COUNT_4_BIT;
		break;
	case 8:
		rasterization_samples = VK_SAMPLE_COUNT_8_BIT;
		break;
	case 16:
		rasterization_samples = VK_SAMPLE_COUNT_16_BIT;
		break;
	case 32:
		rasterization_samples = VK_SAMPLE_COUNT_32_BIT;
		break;
	case 64:
		rasterization_samples = VK_SAMPLE_COUNT_64_BIT;
		break;
	default:
		assert(false);
		rasterization_samples = static_cast<VkSampleCountFlagBits>(-1);
	}

	VkPipelineMultisampleStateCreateInfo const multisample_state = {
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		NULL,
		0U,
		rasterization_samples,
		VK_FALSE,
		0.0F,
		NULL,
		VK_FALSE,
		VK_FALSE};

	VkBool32 depth_test_enable;
	VkCompareOp depth_compare_op;
	switch (wrapped_depth_compare_operation)
	{
	case BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_DISABLE:
	{
		depth_test_enable = VK_FALSE;
		depth_compare_op = VK_COMPARE_OP_ALWAYS;
	}
	break;
	case BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER:
	{
		depth_test_enable = VK_TRUE;
		depth_compare_op = VK_COMPARE_OP_GREATER;
	}
	break;
	case BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_LESS:
	{
		depth_test_enable = VK_TRUE;
		depth_compare_op = VK_COMPARE_OP_LESS;
	}
	break;
	default:
	{
		assert(false);
		depth_test_enable = VK_FALSE;
		depth_compare_op = VK_COMPARE_OP_ALWAYS;
	}
	}

	VkPipelineDepthStencilStateCreateInfo const depth_stencil_state = {
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		NULL,
		0U,
		depth_test_enable,
		VK_TRUE,
		depth_compare_op,
		VK_FALSE,
		VK_FALSE,
		{VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 255, 255, 255},
		{VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS, 255, 255, 255},
		0.0F,
		1.0F};

	uint32_t const color_attachment_count = static_cast<brx_pal_vk_render_pass const *>(wrapped_render_pass)->get_color_attachment_count();
	mcrt_vector<VkPipelineColorBlendAttachmentState> attachments(color_attachment_count);
	switch (wrapped_blend_operation)
	{
	case BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_DISABLE:
	{
		for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
		{
			attachments[color_attachment_index] = VkPipelineColorBlendAttachmentState{
				VK_FALSE,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
		}
	}
	break;
	case BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER_FIRST:
	case BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER_FIRST_AND_SECOND:
	{
		uint32_t const operation_over_attachment_count = (BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER_FIRST == wrapped_blend_operation) ? 1U : ((BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER_FIRST_AND_SECOND == wrapped_blend_operation) ? 2U : 0U);

		assert(color_attachment_count >= operation_over_attachment_count);

		for (uint32_t render_target_index = 0U; render_target_index < operation_over_attachment_count; ++render_target_index)
		{
			attachments[render_target_index] = VkPipelineColorBlendAttachmentState{
				VK_TRUE,
				VK_BLEND_FACTOR_SRC_ALPHA,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
		}

		for (uint32_t color_attachment_index = operation_over_attachment_count; color_attachment_index < color_attachment_count; ++color_attachment_index)
		{
			attachments[color_attachment_index] = VkPipelineColorBlendAttachmentState{
				VK_FALSE,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
		}
	}
	break;
	default:
	{
		assert(false);
		for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
		{
			attachments[color_attachment_index] = VkPipelineColorBlendAttachmentState{
				VK_FALSE,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE,
				VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
		}
	}
	}

	VkPipelineColorBlendStateCreateInfo const color_blend_state = {
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		NULL,
		0U,
		VK_FALSE,
		VK_LOGIC_OP_CLEAR,
		color_attachment_count,
		attachments.data(),
		{0.0F, 0.0F, 0.0F, 0.0F}};

	VkDynamicState const dynamic_states[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo const dynamic_state = {
		VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		NULL,
		0U,
		sizeof(dynamic_states) / sizeof(dynamic_states[0]),
		dynamic_states};

	VkGraphicsPipelineCreateInfo const graphics_pipeline_create_info = {
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		NULL,
		0U,
		sizeof(stages) / sizeof(stages[0]),
		stages,
		&vertex_input_state,
		&input_assembly_state,
		NULL,
		&viewport_state,
		&rasterization_state,
		&multisample_state,
		&depth_stencil_state,
		&color_blend_state,
		&dynamic_state,
		pipeline_layout,
		render_pass,
		subpass_index,
		VK_NULL_HANDLE,
		0U};
	assert(VK_NULL_HANDLE == this->m_pipeline);
	VkResult const res_create_graphics_pipelines = pfn_create_graphics_pipelines(device, VK_NULL_HANDLE, 1U, &graphics_pipeline_create_info, allocation_callbacks, &this->m_pipeline);
	assert(VK_SUCCESS == res_create_graphics_pipelines);

	pfn_destroy_shader_module(device, vertex_shader_module, allocation_callbacks);
	pfn_destroy_shader_module(device, fragment_shader_module, allocation_callbacks);
}

void brx_pal_vk_graphics_pipeline::uninit(PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, VkAllocationCallbacks const *allocation_callbacks)
{
	PFN_vkDestroyPipeline const pfn_destroy_pipeline = reinterpret_cast<PFN_vkDestroyPipeline>(pfn_get_device_proc_addr(device, "vkDestroyPipeline"));
	assert(NULL != pfn_destroy_pipeline);

	assert(VK_NULL_HANDLE != this->m_pipeline);

	pfn_destroy_pipeline(device, this->m_pipeline, allocation_callbacks);

	this->m_pipeline = VK_NULL_HANDLE;
}

brx_pal_vk_graphics_pipeline::~brx_pal_vk_graphics_pipeline()
{
	assert(VK_NULL_HANDLE == this->m_pipeline);
}

VkPipeline brx_pal_vk_graphics_pipeline::get_pipeline() const
{
	return this->m_pipeline;
}

brx_pal_vk_compute_pipeline::brx_pal_vk_compute_pipeline() : m_pipeline(VK_NULL_HANDLE)
{
}

void brx_pal_vk_compute_pipeline::init(PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, VkAllocationCallbacks const *allocation_callbacks, brx_pal_pipeline_layout const *wrapped_pipeline_layout, size_t compute_shader_module_code_size, void const *compute_shader_module_code)
{
	PFN_vkCreateShaderModule const pfn_create_shader_module = reinterpret_cast<PFN_vkCreateShaderModule>(pfn_get_device_proc_addr(device, "vkCreateShaderModule"));
	assert(pfn_create_shader_module);
	PFN_vkDestroyShaderModule const pfn_destroy_shader_module = reinterpret_cast<PFN_vkDestroyShaderModule>(pfn_get_device_proc_addr(device, "vkDestroyShaderModule"));
	assert(pfn_destroy_shader_module);
	PFN_vkCreateComputePipelines const pfn_create_compute_pipelines = reinterpret_cast<PFN_vkCreateComputePipelines>(pfn_get_device_proc_addr(device, "vkCreateComputePipelines"));
	assert(pfn_create_compute_pipelines);

	assert(NULL != wrapped_pipeline_layout);
	VkPipelineLayout pipeline_layout = static_cast<brx_pal_vk_pipeline_layout const *>(wrapped_pipeline_layout)->get_pipeline_layout();

	VkShaderModule compute_shader_module = VK_NULL_HANDLE;
	{
		VkShaderModuleCreateInfo const shader_module_create_info = {
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			NULL,
			0U,
			compute_shader_module_code_size,
			static_cast<uint32_t const *>(compute_shader_module_code)};

		VkResult const res_create_shader_module = pfn_create_shader_module(device, &shader_module_create_info, NULL, &compute_shader_module);
		assert(VK_SUCCESS == res_create_shader_module);
	}

	VkComputePipelineCreateInfo const compute_pipeline_create_info = {
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		NULL,
		0U,
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0U, VK_SHADER_STAGE_COMPUTE_BIT, compute_shader_module, "main", NULL},
		pipeline_layout,
		VK_NULL_HANDLE,
		0U};
	assert(VK_NULL_HANDLE == this->m_pipeline);
	VkResult const res_create_compute_pipelines = pfn_create_compute_pipelines(device, VK_NULL_HANDLE, 1U, &compute_pipeline_create_info, allocation_callbacks, &this->m_pipeline);
	assert(VK_SUCCESS == res_create_compute_pipelines);

	pfn_destroy_shader_module(device, compute_shader_module, allocation_callbacks);
}

void brx_pal_vk_compute_pipeline::uninit(PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, VkAllocationCallbacks const *allocation_callbacks)
{
	PFN_vkDestroyPipeline const pfn_destroy_pipeline = reinterpret_cast<PFN_vkDestroyPipeline>(pfn_get_device_proc_addr(device, "vkDestroyPipeline"));
	assert(NULL != pfn_destroy_pipeline);

	assert(VK_NULL_HANDLE != this->m_pipeline);

	pfn_destroy_pipeline(device, this->m_pipeline, allocation_callbacks);

	this->m_pipeline = VK_NULL_HANDLE;
}

brx_pal_vk_compute_pipeline::~brx_pal_vk_compute_pipeline()
{
	assert(VK_NULL_HANDLE == this->m_pipeline);
}

VkPipeline brx_pal_vk_compute_pipeline::get_pipeline() const
{
	return this->m_pipeline;
}
