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

#include "brx_pal_d3d12_device.h"
#include <assert.h>

brx_pal_d3d12_graphics_pipeline::brx_pal_d3d12_graphics_pipeline() : m_pipeline_state(NULL)
{
}

void brx_pal_d3d12_graphics_pipeline::init(ID3D12Device *device, brx_pal_render_pass const *wrapped_render_pass, brx_pal_pipeline_layout const *wrapped_pipeline_layout, size_t vertex_shader_module_code_size, void const *vertex_shader_module_code, size_t fragment_shader_module_code_size, void const *fragment_shader_module_code, bool enable_back_face_cull, bool front_ccw, bool enable_depth, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION wrapped_depth_compare_operation, bool enable_blend, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION wrapped_blend_operation)
{
	D3D12_PRIMITIVE_TOPOLOGY_TYPE const new_primitive_topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	this->m_primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	assert(NULL != wrapped_render_pass);
	assert(NULL != wrapped_pipeline_layout);
	uint32_t const color_attachment_count = static_cast<brx_pal_d3d12_render_pass const *>(wrapped_render_pass)->get_color_attachment_count();
	BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const *const color_attachment_formats = static_cast<brx_pal_d3d12_render_pass const *>(wrapped_render_pass)->get_color_attachment_formats();
	BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT const *const depth_stencil_attachment_format = static_cast<brx_pal_d3d12_render_pass const *>(wrapped_render_pass)->get_depth_stencil_attachment_format();
	ID3D12RootSignature *const root_signature = static_cast<brx_pal_d3d12_pipeline_layout const *>(wrapped_pipeline_layout)->get_root_signature();

	D3D12_COMPARISON_FUNC depth_func;
	switch (wrapped_depth_compare_operation)
	{
	case BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_UNKNOWN:
		assert(!enable_depth);
		depth_func = D3D12_COMPARISON_FUNC_LESS;
		break;
	case BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_GREATER:
		depth_func = D3D12_COMPARISON_FUNC_GREATER;
		break;
	case BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_LESS:
		depth_func = D3D12_COMPARISON_FUNC_LESS;
		break;
	case BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION_ALWAYS:
		depth_func = D3D12_COMPARISON_FUNC_ALWAYS;
		break;
	default:
		assert(false);
		depth_func = static_cast<D3D12_COMPARISON_FUNC>(-1);
	}

	D3D12_BLEND src_blend;
	D3D12_BLEND dest_blend;
	D3D12_BLEND_OP blend_op;
	D3D12_BLEND src_blend_alpha;
	D3D12_BLEND dest_blend_alpha;
	D3D12_BLEND_OP blend_op_alpha;
	UINT8 render_target_write_mask;
	switch (wrapped_blend_operation)
	{
	case BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_UNKNOWN:
	{
		assert(!enable_blend);
		src_blend = D3D12_BLEND_ONE;
		dest_blend = D3D12_BLEND_ZERO;
		blend_op = D3D12_BLEND_OP_ADD;
		src_blend_alpha = D3D12_BLEND_ONE;
		dest_blend_alpha = D3D12_BLEND_ZERO;
		blend_op_alpha = D3D12_BLEND_OP_ADD;
		render_target_write_mask = D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE | D3D12_COLOR_WRITE_ENABLE_ALPHA;
	}
	break;
	case BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION_OVER:
	{
		// The over operation is only meaningful when there is only one color attachment
		assert(1U == color_attachment_count);
		src_blend = D3D12_BLEND_SRC_ALPHA;
		dest_blend = D3D12_BLEND_INV_SRC_ALPHA;
		blend_op = D3D12_BLEND_OP_ADD;
		src_blend_alpha = D3D12_BLEND_ONE;
		dest_blend_alpha = D3D12_BLEND_ONE;
		blend_op_alpha = D3D12_BLEND_OP_MAX;
		render_target_write_mask = D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;
	}
	break;
	default:
	{
		assert(false);
		src_blend = static_cast<D3D12_BLEND>(-1);
		dest_blend = static_cast<D3D12_BLEND>(-1);
		blend_op = static_cast<D3D12_BLEND_OP>(-1);
		src_blend_alpha = static_cast<D3D12_BLEND>(-1);
		dest_blend_alpha = static_cast<D3D12_BLEND>(-1);
		blend_op_alpha = static_cast<D3D12_BLEND_OP>(-1);
		render_target_write_mask = static_cast<UINT8>(-1);
	}
	}

	DXGI_FORMAT rtv_formats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
	{
		switch (color_attachment_formats[color_attachment_index])
		{
		case BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_UNORM:
			rtv_formats[color_attachment_index] = DXGI_FORMAT_B8G8R8A8_UNORM;
			break;
		case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_UNORM:
			rtv_formats[color_attachment_index] = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2B10G10R10_UNORM_PACK32:
			assert(false);
			rtv_formats[color_attachment_index] = static_cast<DXGI_FORMAT>(-1);
			break;
		case BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32:
			rtv_formats[color_attachment_index] = DXGI_FORMAT_R10G10B10A2_UNORM;
			break;
		case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16_UNORM:
			rtv_formats[color_attachment_index] = DXGI_FORMAT_R16G16_UNORM;
			break;
		case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R32G32_UINT:
			rtv_formats[color_attachment_index] = DXGI_FORMAT_R32G32_UINT;
			break;
		case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R32G32B32A32_UINT:
			rtv_formats[color_attachment_index] = DXGI_FORMAT_R32G32B32A32_UINT;
			break;
		default:
			assert(false);
			rtv_formats[color_attachment_index] = static_cast<DXGI_FORMAT>(-1);
		}
	}
	for (uint32_t color_attachment_index = color_attachment_count; color_attachment_index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++color_attachment_index)
	{
		rtv_formats[color_attachment_index] = DXGI_FORMAT_UNKNOWN;
	}

	DXGI_FORMAT dsv_format;
	if (NULL != depth_stencil_attachment_format)
	{
		switch (*depth_stencil_attachment_format)
		{
		case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT:
			dsv_format = DXGI_FORMAT_D32_FLOAT;
			break;
		case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT_S8_UINT:
			dsv_format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			break;
		case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D24_UNORM_S8_UINT:
			dsv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		default:
			assert(false);
			dsv_format = static_cast<DXGI_FORMAT>(-1);
		}
	}
	else
	{
		dsv_format = DXGI_FORMAT_UNKNOWN;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC const desc = {
		root_signature,
		{vertex_shader_module_code, vertex_shader_module_code_size},
		{fragment_shader_module_code, fragment_shader_module_code_size},
		{NULL, 0U},
		{NULL, 0U},
		{NULL, 0U},
		{NULL, 0U, NULL, 0U, 0U},
		{FALSE, FALSE, {{enable_blend ? TRUE : FALSE, FALSE, src_blend, dest_blend, blend_op, src_blend_alpha, dest_blend_alpha, blend_op_alpha, D3D12_LOGIC_OP_NOOP, render_target_write_mask}}},
		0XFFFFFFFFU,
		{D3D12_FILL_MODE_SOLID, enable_back_face_cull ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE, front_ccw ? TRUE : FALSE, 0, 0.0F, 0.0F, TRUE, FALSE, FALSE, 0U, D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF},
		{enable_depth ? TRUE : FALSE, D3D12_DEPTH_WRITE_MASK_ALL, depth_func, FALSE, D3D12_DEFAULT_STENCIL_READ_MASK, D3D12_DEFAULT_STENCIL_WRITE_MASK, {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS}, {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS}},
		{NULL, 0U},
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
		new_primitive_topology_type,
		color_attachment_count,
		{rtv_formats[0], rtv_formats[1], rtv_formats[2], rtv_formats[3], rtv_formats[4], rtv_formats[5], rtv_formats[6], rtv_formats[7]},
		dsv_format,
		{1U, 0U},
		0U,
		{NULL, 0U},
		D3D12_PIPELINE_STATE_FLAG_NONE};

	assert(NULL == this->m_pipeline_state);
	HRESULT const hr_create_graphics_pipeline_state = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&this->m_pipeline_state));
	assert(SUCCEEDED(hr_create_graphics_pipeline_state));
}

void brx_pal_d3d12_graphics_pipeline::uninit()
{
	assert(NULL != this->m_pipeline_state);

	this->m_pipeline_state->Release();

	this->m_pipeline_state = NULL;
}

brx_pal_d3d12_graphics_pipeline::~brx_pal_d3d12_graphics_pipeline()
{
	assert(NULL == this->m_pipeline_state);
}

D3D12_PRIMITIVE_TOPOLOGY brx_pal_d3d12_graphics_pipeline::get_primitive_topology() const
{
	return this->m_primitive_topology;
}

ID3D12PipelineState *brx_pal_d3d12_graphics_pipeline::get_pipeline() const
{
	return this->m_pipeline_state;
}

brx_pal_d3d12_compute_pipeline::brx_pal_d3d12_compute_pipeline() : m_pipeline_state(NULL)
{
}

void brx_pal_d3d12_compute_pipeline::init(ID3D12Device *device, brx_pal_pipeline_layout const *wrapped_pipeline_layout, size_t compute_shader_module_code_size, void const *compute_shader_module_code)
{
	assert(NULL != wrapped_pipeline_layout);
	ID3D12RootSignature *const root_signature = static_cast<brx_pal_d3d12_pipeline_layout const *>(wrapped_pipeline_layout)->get_root_signature();

	D3D12_COMPUTE_PIPELINE_STATE_DESC const desc = {
		root_signature,
		{compute_shader_module_code, compute_shader_module_code_size},
		0U,
		{NULL, 0U},
		D3D12_PIPELINE_STATE_FLAG_NONE};

	assert(NULL == this->m_pipeline_state);
	HRESULT const hr_create_compute_pipeline_state = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&this->m_pipeline_state));
	assert(SUCCEEDED(hr_create_compute_pipeline_state));
}

void brx_pal_d3d12_compute_pipeline::uninit()
{
	assert(NULL != this->m_pipeline_state);

	this->m_pipeline_state->Release();

	this->m_pipeline_state = NULL;
}

brx_pal_d3d12_compute_pipeline::~brx_pal_d3d12_compute_pipeline()
{
	assert(NULL == this->m_pipeline_state);
}

ID3D12PipelineState *brx_pal_d3d12_compute_pipeline::get_pipeline() const
{
	return this->m_pipeline_state;
}
