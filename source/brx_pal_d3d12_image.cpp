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

brx_pal_d3d12_color_attachment_intermediate_image::brx_pal_d3d12_color_attachment_intermediate_image() : m_resource(NULL), m_allocation(NULL), m_render_target_view_descriptor_heap(NULL), m_render_target_view_descriptor{0U}
{
}

void brx_pal_d3d12_color_attachment_intermediate_image::init(ID3D12Device *device, D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *color_attachment_intermediate_image_memory_pool, BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT wrapped_color_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image)
{
	DXGI_FORMAT unwrapped_format;
	switch (wrapped_color_attachment_image_format)
	{
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_UNORM:
		unwrapped_format = DXGI_FORMAT_B8G8R8A8_UNORM;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_UNORM:
		unwrapped_format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2B10G10R10_UNORM_PACK32:
		assert(false);
		unwrapped_format = static_cast<DXGI_FORMAT>(-1);
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32:
		unwrapped_format = DXGI_FORMAT_R10G10B10A2_UNORM;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R32G32_UINT:
		unwrapped_format = DXGI_FORMAT_R32G32_UINT;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R32G32B32A32_UINT:
		unwrapped_format = DXGI_FORMAT_R32G32B32A32_UINT;
		break;
	default:
		assert(false);
		unwrapped_format = static_cast<DXGI_FORMAT>(-1);
	}

	D3D12MA::ALLOCATION_DESC allocation_desc = {
		D3D12MA::ALLOCATION_FLAG_NONE,
		D3D12_HEAP_TYPE_CUSTOM,
		D3D12_HEAP_FLAG_NONE,
		color_attachment_intermediate_image_memory_pool,
		NULL};
	D3D12_RESOURCE_DESC const resource_desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		width,
		height,
		1U,
		1U,
		unwrapped_format,
		{1U, 0U},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET};
	D3D12_CLEAR_VALUE const optimized_clear_value = {
		.Format = unwrapped_format,
		.Color = {
			0.0F,
			0.0F,
			0.0F,
			0.0F}};
	assert(NULL == this->m_resource);
	assert(NULL == this->m_allocation);
	HRESULT hr_create_resource = memory_allocator->CreateResource(&allocation_desc, &resource_desc, (!allow_sampled_image) ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE), &optimized_clear_value, &this->m_allocation, IID_PPV_ARGS(&this->m_resource));
	assert(SUCCEEDED(hr_create_resource));

	D3D12_DESCRIPTOR_HEAP_DESC const descriptor_heap_desc = {
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		1U,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0U};
	assert(NULL == this->m_render_target_view_descriptor_heap);
	HRESULT const hr_create_descriptor_heap = device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&this->m_render_target_view_descriptor_heap));
	assert(SUCCEEDED(hr_create_descriptor_heap));

	UINT const new_dsv_descriptor_heap_descriptor_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	D3D12_CPU_DESCRIPTOR_HANDLE const new_dsv_descriptor_heap_cpu_descriptor_handle_start = this->m_render_target_view_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	assert(0U == this->m_render_target_view_descriptor.ptr);
	this->m_render_target_view_descriptor = D3D12_CPU_DESCRIPTOR_HANDLE{new_dsv_descriptor_heap_cpu_descriptor_handle_start.ptr + new_dsv_descriptor_heap_descriptor_increment_size * 0U};

	D3D12_RENDER_TARGET_VIEW_DESC const depth_stencil_view_desc{
		.Format = unwrapped_format,
		.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		.Texture2D = {
			0U,
			0U}};
	device->CreateRenderTargetView(this->m_resource, &depth_stencil_view_desc, this->m_render_target_view_descriptor);

	if (allow_sampled_image)
	{
		this->m_shader_resource_view_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format = unwrapped_format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D = {
				0U,
				1U,
				0U,
				0.0F}};
	}
	else
	{
		this->m_shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
	}
}

void brx_pal_d3d12_color_attachment_intermediate_image::uninit()
{
	assert(NULL != this->m_render_target_view_descriptor_heap);
	this->m_render_target_view_descriptor_heap->Release();
	this->m_render_target_view_descriptor_heap = NULL;

	assert(NULL != this->m_resource);
	this->m_resource->Release();
	this->m_resource = NULL;

	assert(NULL != this->m_allocation);
	this->m_allocation->Release();
	this->m_allocation = NULL;
}

brx_pal_d3d12_color_attachment_intermediate_image::~brx_pal_d3d12_color_attachment_intermediate_image()
{
	assert(NULL == this->m_render_target_view_descriptor_heap);
	assert(NULL == this->m_resource);
	assert(NULL == this->m_allocation);
}

ID3D12Resource *brx_pal_d3d12_color_attachment_intermediate_image::get_resource() const
{
	return this->m_resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE brx_pal_d3d12_color_attachment_intermediate_image::get_render_target_view_descriptor() const
{
	return this->m_render_target_view_descriptor;
}

D3D12_SHADER_RESOURCE_VIEW_DESC const *brx_pal_d3d12_color_attachment_intermediate_image::get_shader_resource_view_desc() const
{
	return &this->m_shader_resource_view_desc;
}

brx_pal_sampled_image const *brx_pal_d3d12_color_attachment_intermediate_image::get_sampled_image() const
{
	assert(D3D12_SRV_DIMENSION_TEXTURE2D == this->m_shader_resource_view_desc.ViewDimension);
	return static_cast<brx_pal_d3d12_sampled_image const *>(this);
}

brx_pal_d3d12_depth_stencil_attachment_intermediate_image::brx_pal_d3d12_depth_stencil_attachment_intermediate_image() : m_resource(NULL), m_allocation(NULL), m_depth_stencil_view_descriptor_heap(NULL), m_depth_stencil_view_descriptor{0U}
{
}

void brx_pal_d3d12_depth_stencil_attachment_intermediate_image::init(ID3D12Device *device, D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *depth_stencil_attachment_intermediate_image_memory_pool, BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT wrapped_depth_stencil_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image)
{
	DXGI_FORMAT unwrapped_resource_format;
	DXGI_FORMAT unwrapped_depth_stencil_view_format;
	DXGI_FORMAT unwrapped_shader_resource_view_format;
	switch (wrapped_depth_stencil_attachment_image_format)
	{
	case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT:
		unwrapped_resource_format = DXGI_FORMAT_R32_TYPELESS;
		unwrapped_depth_stencil_view_format = DXGI_FORMAT_D32_FLOAT;
		unwrapped_shader_resource_view_format = DXGI_FORMAT_R32_FLOAT;
		break;
	case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT_S8_UINT:
		unwrapped_resource_format = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
		unwrapped_depth_stencil_view_format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		unwrapped_shader_resource_view_format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		break;
	case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D24_UNORM_S8_UINT:
		unwrapped_resource_format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		unwrapped_depth_stencil_view_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		unwrapped_shader_resource_view_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		break;
	default:
		assert(false);
		unwrapped_resource_format = static_cast<DXGI_FORMAT>(-1);
		unwrapped_depth_stencil_view_format = static_cast<DXGI_FORMAT>(-1);
		unwrapped_shader_resource_view_format = static_cast<DXGI_FORMAT>(-1);
	}

	D3D12MA::ALLOCATION_DESC allocation_desc = {
		D3D12MA::ALLOCATION_FLAG_NONE,
		D3D12_HEAP_TYPE_CUSTOM,
		D3D12_HEAP_FLAG_NONE,
		depth_stencil_attachment_intermediate_image_memory_pool,
		NULL};
	D3D12_RESOURCE_DESC const resource_desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		width,
		height,
		1U,
		1U,
		unwrapped_resource_format,
		{1U, 0U},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		(!allow_sampled_image) ? (D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) : D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL};
	D3D12_CLEAR_VALUE const optimized_clear_value = {
		.Format = unwrapped_depth_stencil_view_format,
		.DepthStencil = {
			0.0F,
			0U}};
	assert(NULL == this->m_resource);
	assert(NULL == this->m_allocation);
	HRESULT hr_create_resource = memory_allocator->CreateResource(&allocation_desc, &resource_desc, (!allow_sampled_image) ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE), &optimized_clear_value, &this->m_allocation, IID_PPV_ARGS(&this->m_resource));
	assert(SUCCEEDED(hr_create_resource));

	D3D12_DESCRIPTOR_HEAP_DESC const descriptor_heap_desc = {
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		1U,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0U};
	assert(NULL == this->m_depth_stencil_view_descriptor_heap);
	HRESULT const hr_create_descriptor_heap = device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&this->m_depth_stencil_view_descriptor_heap));
	assert(SUCCEEDED(hr_create_descriptor_heap));

	UINT const new_dsv_descriptor_heap_descriptor_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	D3D12_CPU_DESCRIPTOR_HANDLE const new_dsv_descriptor_heap_cpu_descriptor_handle_start = this->m_depth_stencil_view_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	assert(0U == this->m_depth_stencil_view_descriptor.ptr);
	this->m_depth_stencil_view_descriptor = D3D12_CPU_DESCRIPTOR_HANDLE{new_dsv_descriptor_heap_cpu_descriptor_handle_start.ptr + new_dsv_descriptor_heap_descriptor_increment_size * 0U};

	D3D12_DEPTH_STENCIL_VIEW_DESC const depth_stencil_view_desc{
		.Format = unwrapped_depth_stencil_view_format,
		.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		.Flags = D3D12_DSV_FLAG_NONE,
		.Texture2D = {
			0U}};
	device->CreateDepthStencilView(this->m_resource, &depth_stencil_view_desc, this->m_depth_stencil_view_descriptor);

	if (allow_sampled_image)
	{
		this->m_shader_resource_view_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format = unwrapped_shader_resource_view_format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D = {
				0U,
				1U,
				0U,
				0.0F}};
	}
	else
	{
		this->m_shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
	}
}

void brx_pal_d3d12_depth_stencil_attachment_intermediate_image::uninit()
{
	assert(NULL != this->m_depth_stencil_view_descriptor_heap);
	this->m_depth_stencil_view_descriptor_heap->Release();
	this->m_depth_stencil_view_descriptor_heap = NULL;

	assert(NULL != this->m_resource);
	this->m_resource->Release();
	this->m_resource = NULL;

	assert(NULL != this->m_allocation);
	this->m_allocation->Release();
	this->m_allocation = NULL;
}

brx_pal_d3d12_depth_stencil_attachment_intermediate_image::~brx_pal_d3d12_depth_stencil_attachment_intermediate_image()
{
	assert(NULL == this->m_depth_stencil_view_descriptor_heap);
	assert(NULL == this->m_resource);
	assert(NULL == this->m_allocation);
}

ID3D12Resource *brx_pal_d3d12_depth_stencil_attachment_intermediate_image::get_resource() const
{
	return this->m_resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE brx_pal_d3d12_depth_stencil_attachment_intermediate_image::get_depth_stencil_view_descriptor() const
{
	return this->m_depth_stencil_view_descriptor;
}

D3D12_SHADER_RESOURCE_VIEW_DESC const *brx_pal_d3d12_depth_stencil_attachment_intermediate_image::get_shader_resource_view_desc() const
{
	return &this->m_shader_resource_view_desc;
}

brx_pal_sampled_image const *brx_pal_d3d12_depth_stencil_attachment_intermediate_image::get_sampled_image() const
{
	assert(D3D12_SRV_DIMENSION_TEXTURE2D == this->m_shader_resource_view_desc.ViewDimension);
	return static_cast<brx_pal_d3d12_sampled_image const *>(this);
}

brx_pal_d3d12_storage_intermediate_image::brx_pal_d3d12_storage_intermediate_image() : m_resource(NULL), m_allocation(NULL)
{
}

void brx_pal_d3d12_storage_intermediate_image::init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *storage_intermediate_image_memory_pool, DXGI_FORMAT unwrapped_storage_image_format, uint32_t width, uint32_t height, bool allow_sampled_image)
{
	D3D12MA::ALLOCATION_DESC allocation_desc = {
		D3D12MA::ALLOCATION_FLAG_NONE,
		D3D12_HEAP_TYPE_CUSTOM,
		D3D12_HEAP_FLAG_NONE,
		storage_intermediate_image_memory_pool,
		NULL};

	D3D12_RESOURCE_DESC const resource_desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		width,
		height,
		1U,
		1U,
		unwrapped_storage_image_format,
		{1U, 0U},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS};

	assert(NULL == this->m_resource);
	assert(NULL == this->m_allocation);
	HRESULT hr_create_resource = memory_allocator->CreateResource(&allocation_desc, &resource_desc, (!allow_sampled_image) ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE), NULL, &this->m_allocation, IID_PPV_ARGS(&this->m_resource));
	assert(SUCCEEDED(hr_create_resource));

	this->m_unordered_access_view_desc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
		.Format = unwrapped_storage_image_format,
		.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
		.Texture2D = {
			0U,
			0U}};

	if (allow_sampled_image)
	{
		this->m_shader_resource_view_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format = unwrapped_storage_image_format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D = {
				0U,
				1U,
				0U,
				0.0F}};
	}
	else
	{
		this->m_shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
	}
}

void brx_pal_d3d12_storage_intermediate_image::uninit()
{
	assert(NULL != this->m_resource);
	this->m_resource->Release();
	this->m_resource = NULL;

	assert(NULL != this->m_allocation);
	this->m_allocation->Release();
	this->m_allocation = NULL;
}

brx_pal_d3d12_storage_intermediate_image::~brx_pal_d3d12_storage_intermediate_image()
{
	assert(NULL == this->m_resource);
	assert(NULL == this->m_allocation);
}

ID3D12Resource *brx_pal_d3d12_storage_intermediate_image::get_resource() const
{
	return this->m_resource;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC const *brx_pal_d3d12_storage_intermediate_image::get_unordered_access_view_desc() const
{
	return &this->m_unordered_access_view_desc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC const *brx_pal_d3d12_storage_intermediate_image::get_shader_resource_view_desc() const
{
	return &this->m_shader_resource_view_desc;
}

brx_pal_sampled_image const *brx_pal_d3d12_storage_intermediate_image::get_sampled_image() const
{
	assert(D3D12_SRV_DIMENSION_TEXTURE2D == this->m_shader_resource_view_desc.ViewDimension);
	return static_cast<brx_pal_d3d12_sampled_image const *>(this);
}

brx_pal_d3d12_sampled_asset_image::brx_pal_d3d12_sampled_asset_image() : m_resource(NULL), m_allocation(NULL), m_mip_levels(static_cast<uint32_t>(-1))
{
}

void brx_pal_d3d12_sampled_asset_image::init(bool uma, D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *sampled_asset_image_memory_pool, DXGI_FORMAT unwrapped_sampled_asset_image_format, uint32_t width, uint32_t height, uint32_t unwrapped_mip_levels)
{
	uint16_t const wrapped_mip_levels = static_cast<uint16_t>(unwrapped_mip_levels);

	D3D12MA::ALLOCATION_DESC allocation_desc = {
		D3D12MA::ALLOCATION_FLAG_NONE,
		D3D12_HEAP_TYPE_CUSTOM,
		D3D12_HEAP_FLAG_NONE,
		sampled_asset_image_memory_pool,
		NULL};

	D3D12_RESOURCE_DESC resource_desc = {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		width,
		height,
		1U,
		wrapped_mip_levels,
		unwrapped_sampled_asset_image_format,
		{1U, 0U},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_NONE};

	HRESULT hr_create_resource = memory_allocator->CreateResource(&allocation_desc, &resource_desc, (!uma) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_COMMON, NULL, &this->m_allocation, IID_PPV_ARGS(&this->m_resource));
	assert(SUCCEEDED(hr_create_resource));

	this->m_shader_resource_view_desc = D3D12_SHADER_RESOURCE_VIEW_DESC{
		.Format = unwrapped_sampled_asset_image_format,
		.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		.Texture2D = {
			0U,
			wrapped_mip_levels,
			0U,
			0.0F}};

	assert(static_cast<uint32_t>(-1) == this->m_mip_levels);
	this->m_mip_levels = wrapped_mip_levels;
}

void brx_pal_d3d12_sampled_asset_image::uninit()
{
	assert(NULL != this->m_resource);
	this->m_resource->Release();
	this->m_resource = NULL;

	assert(NULL != this->m_allocation);
	this->m_allocation->Release();
	this->m_allocation = NULL;
}

brx_pal_d3d12_sampled_asset_image::~brx_pal_d3d12_sampled_asset_image()
{
	assert(NULL == this->m_resource);
	assert(NULL == this->m_allocation);
}

ID3D12Resource *brx_pal_d3d12_sampled_asset_image::get_resource() const
{
	return this->m_resource;
}

D3D12_SHADER_RESOURCE_VIEW_DESC const *brx_pal_d3d12_sampled_asset_image::get_shader_resource_view_desc() const
{
	return &this->m_shader_resource_view_desc;
}

brx_pal_sampled_image const *brx_pal_d3d12_sampled_asset_image::get_sampled_image() const
{
	return static_cast<brx_pal_d3d12_sampled_image const *>(this);
}

uint32_t brx_pal_d3d12_sampled_asset_image::get_mip_levels() const
{
	return this->m_mip_levels;
}
