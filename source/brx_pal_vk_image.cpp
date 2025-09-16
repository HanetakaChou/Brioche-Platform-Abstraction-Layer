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

brx_pal_vk_color_attachment_intermediate_image::brx_pal_vk_color_attachment_intermediate_image() : m_image(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_attachment_image_view(VK_NULL_HANDLE), m_sampled_image_view(VK_NULL_HANDLE), m_allow_sampled_image(false)
{
}

void brx_pal_vk_color_attachment_intermediate_image::init(VkDevice device, PFN_vkCreateImageView pfn_create_image_view, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator, VmaPool color_transient_attachment_image_memory_pool, VmaPool color_attachment_sampled_image_memory_pool, BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT wrapped_color_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image)
{
	VkFormat format;
	switch (wrapped_color_attachment_image_format)
	{
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8_UNORM:
		format = VK_FORMAT_R8G8_UNORM;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_UNORM:
		format = VK_FORMAT_B8G8R8A8_UNORM;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_SRGB:
		format = VK_FORMAT_B8G8R8A8_SRGB;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_UNORM:
		format = VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_SRGB:
		format = VK_FORMAT_R8G8B8A8_SRGB;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2B10G10R10_UNORM_PACK32:
		format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32:
		format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
		break;
	case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16_SNORM:
		format = VK_FORMAT_R16G16_SNORM;
		break;
	default:
		assert(false);
		format = VK_FORMAT_UNDEFINED;
	}

	assert(!this->m_allow_sampled_image);
	this->m_allow_sampled_image = allow_sampled_image;

	VmaPool const memory_pool = allow_sampled_image ? color_attachment_sampled_image_memory_pool : color_transient_attachment_image_memory_pool;

	VkImageAspectFlags const aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageUsageFlags const usage = allow_sampled_image ? (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) : (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);

	VkImageCreateInfo const image_create_info = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		NULL,
		0U,
		VK_IMAGE_TYPE_2D,
		format,
		{width, height, 1U},
		1U,
		1U,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0U,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED};

	VmaAllocationCreateInfo const allocation_create_info = {
		0U,
		VMA_MEMORY_USAGE_UNKNOWN,
		0U,
		0U,
		0U,
		memory_pool,
		NULL,
		1.0F};

	assert(VK_NULL_HANDLE == this->m_image);
	assert(VK_NULL_HANDLE == this->m_allocation);
	VkResult res_vma_create_buffer = vmaCreateImage(memory_allocator, &image_create_info, &allocation_create_info, &this->m_image, &this->m_allocation, NULL);
	assert(VK_SUCCESS == res_vma_create_buffer);

	{
		VkImageViewCreateInfo const image_view_create_info = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0U,
			this->m_image,
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{aspect_mask, 0U, 1U, 0U, 1U}};

		assert(VK_NULL_HANDLE == this->m_attachment_image_view);
		VkResult const res_create_image_view = pfn_create_image_view(device, &image_view_create_info, allocation_callbacks, &this->m_attachment_image_view);
		assert(VK_SUCCESS == res_create_image_view);
	}

	if (allow_sampled_image)
	{
		VkImageViewCreateInfo const image_view_create_info = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0U,
			this->m_image,
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{aspect_mask, 0U, 1U, 0U, 1U}};

		assert(VK_NULL_HANDLE == this->m_sampled_image_view);
		VkResult const res_create_image_view = pfn_create_image_view(device, &image_view_create_info, allocation_callbacks, &this->m_sampled_image_view);
		assert(VK_SUCCESS == res_create_image_view);
	}
	else
	{
		assert(VK_NULL_HANDLE == this->m_sampled_image_view);
	}
}

void brx_pal_vk_color_attachment_intermediate_image::uninit(VkDevice device, PFN_vkDestroyImageView pfn_destroy_image_view, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator)
{
	assert(VK_NULL_HANDLE != this->m_attachment_image_view);
	pfn_destroy_image_view(device, this->m_attachment_image_view, allocation_callbacks);
	this->m_attachment_image_view = VK_NULL_HANDLE;

	if (VK_NULL_HANDLE != this->m_sampled_image_view)
	{
		assert(this->m_allow_sampled_image);
		pfn_destroy_image_view(device, this->m_sampled_image_view, allocation_callbacks);
		this->m_sampled_image_view = VK_NULL_HANDLE;
	}
	else
	{
		assert(!this->m_allow_sampled_image);
	}

	assert(VK_NULL_HANDLE != this->m_image);
	assert(VK_NULL_HANDLE != this->m_allocation);

	vmaDestroyImage(memory_allocator, this->m_image, this->m_allocation);

	this->m_image = VK_NULL_HANDLE;
	this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_color_attachment_intermediate_image::~brx_pal_vk_color_attachment_intermediate_image()
{
	assert(VK_NULL_HANDLE == this->m_image);
	assert(VK_NULL_HANDLE == this->m_allocation);
	assert(VK_NULL_HANDLE == this->m_attachment_image_view);
	assert(VK_NULL_HANDLE == this->m_sampled_image_view);
}

VkImageView brx_pal_vk_color_attachment_intermediate_image::get_attachment_image_view() const
{
	assert(VK_NULL_HANDLE != this->m_attachment_image_view);
	return this->m_attachment_image_view;
}

VkImageView brx_pal_vk_color_attachment_intermediate_image::get_sampled_image_view() const
{
	assert(this->m_allow_sampled_image);
	assert(VK_NULL_HANDLE != this->m_sampled_image_view);
	return this->m_sampled_image_view;
}

brx_pal_sampled_image const *brx_pal_vk_color_attachment_intermediate_image::get_sampled_image() const
{
	assert(this->m_allow_sampled_image);
	assert(VK_NULL_HANDLE != this->m_sampled_image_view);
	return static_cast<brx_pal_vk_sampled_image const *>(this);
}

brx_pal_vk_depth_stencil_attachment_intermediate_image::brx_pal_vk_depth_stencil_attachment_intermediate_image() : m_image(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_attachment_image_view(VK_NULL_HANDLE), m_sampled_image_view(VK_NULL_HANDLE), m_allow_sampled_image(false)
{
}

void brx_pal_vk_depth_stencil_attachment_intermediate_image::init(VkDevice device, PFN_vkCreateImageView pfn_create_image_view, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator, VmaPool depth_transient_attachment_image_memory_pool, VmaPool depth_attachment_sampled_image_memory_pool, VmaPool depth_stencil_transient_attachment_image_memory_pool, VmaPool depth_stencil_attachment_sampled_image_memory_pool, BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT wrapped_depth_stencil_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image)
{
	VkFormat format;
	VmaPool memory_pool;
	VkImageAspectFlags aspect_mask;
	switch (wrapped_depth_stencil_attachment_image_format)
	{
	case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT:
		format = VK_FORMAT_D32_SFLOAT;
		memory_pool = allow_sampled_image ? depth_attachment_sampled_image_memory_pool : depth_transient_attachment_image_memory_pool;
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
		break;
	case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_X8_D24_UNORM_PACK32:
		format = VK_FORMAT_X8_D24_UNORM_PACK32;
		memory_pool = allow_sampled_image ? depth_attachment_sampled_image_memory_pool : depth_transient_attachment_image_memory_pool;
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
		break;
	case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT_S8_UINT:
		format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		memory_pool = allow_sampled_image ? depth_stencil_attachment_sampled_image_memory_pool : depth_stencil_transient_attachment_image_memory_pool;
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D24_UNORM_S8_UINT:
		format = VK_FORMAT_D24_UNORM_S8_UINT;
		memory_pool = allow_sampled_image ? depth_stencil_attachment_sampled_image_memory_pool : depth_stencil_transient_attachment_image_memory_pool;
		aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		break;
	default:
		assert(false);
		format = VK_FORMAT_UNDEFINED;
		memory_pool = VK_NULL_HANDLE;
		aspect_mask = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
	}

	assert(!this->m_allow_sampled_image);
	this->m_allow_sampled_image = allow_sampled_image;

	VkImageUsageFlags const usage = allow_sampled_image ? (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) : (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);

	VkImageCreateInfo const image_create_info = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		NULL,
		0U,
		VK_IMAGE_TYPE_2D,
		format,
		{width, height, 1U},
		1U,
		1U,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0U,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED};

	VmaAllocationCreateInfo const allocation_create_info = {
		0U,
		VMA_MEMORY_USAGE_UNKNOWN,
		0U,
		0U,
		0U,
		memory_pool,
		NULL,
		1.0F};

	assert(VK_NULL_HANDLE == this->m_image);
	assert(VK_NULL_HANDLE == this->m_allocation);
	VkResult res_vma_create_buffer = vmaCreateImage(memory_allocator, &image_create_info, &allocation_create_info, &this->m_image, &this->m_allocation, NULL);
	assert(VK_SUCCESS == res_vma_create_buffer);

	{
		VkImageViewCreateInfo const image_view_create_info = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0U,
			this->m_image,
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{aspect_mask, 0U, 1U, 0U, 1U}};
		assert(VK_NULL_HANDLE == this->m_attachment_image_view);
		VkResult const res_create_image_view = pfn_create_image_view(device, &image_view_create_info, allocation_callbacks, &this->m_attachment_image_view);
		assert(VK_SUCCESS == res_create_image_view);
	}

	if (allow_sampled_image)
	{
		VkImageViewCreateInfo const image_view_create_info = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0U,
			this->m_image,
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{aspect_mask, 0U, 1U, 0U, 1U}};
		assert(VK_NULL_HANDLE == this->m_sampled_image_view);
		VkResult const res_create_image_view = pfn_create_image_view(device, &image_view_create_info, allocation_callbacks, &this->m_sampled_image_view);
		assert(VK_SUCCESS == res_create_image_view);
	}
	else
	{
		assert(VK_NULL_HANDLE == this->m_sampled_image_view);
	}
}

void brx_pal_vk_depth_stencil_attachment_intermediate_image::uninit(VkDevice device, PFN_vkDestroyImageView pfn_destroy_image_view, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator)
{
	assert(VK_NULL_HANDLE != this->m_attachment_image_view);
	pfn_destroy_image_view(device, this->m_attachment_image_view, allocation_callbacks);
	this->m_attachment_image_view = VK_NULL_HANDLE;

	if (VK_NULL_HANDLE != this->m_sampled_image_view)
	{
		assert(this->m_allow_sampled_image);
		pfn_destroy_image_view(device, this->m_sampled_image_view, allocation_callbacks);
		this->m_sampled_image_view = VK_NULL_HANDLE;
	}
	else
	{
		assert(!this->m_allow_sampled_image);
	}

	assert(VK_NULL_HANDLE != this->m_image);
	assert(VK_NULL_HANDLE != this->m_allocation);

	vmaDestroyImage(memory_allocator, this->m_image, this->m_allocation);

	this->m_image = VK_NULL_HANDLE;
	this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_depth_stencil_attachment_intermediate_image::~brx_pal_vk_depth_stencil_attachment_intermediate_image()
{
	assert(VK_NULL_HANDLE == this->m_image);
	assert(VK_NULL_HANDLE == this->m_allocation);
	assert(VK_NULL_HANDLE == this->m_attachment_image_view);
	assert(VK_NULL_HANDLE == this->m_sampled_image_view);
}

VkImageView brx_pal_vk_depth_stencil_attachment_intermediate_image::get_attachment_image_view() const
{
	assert(VK_NULL_HANDLE != this->m_attachment_image_view);
	return this->m_attachment_image_view;
}

VkImageView brx_pal_vk_depth_stencil_attachment_intermediate_image::get_sampled_image_view() const
{
	assert(this->m_allow_sampled_image);
	assert(VK_NULL_HANDLE != this->m_sampled_image_view);
	return this->m_sampled_image_view;
}

brx_pal_sampled_image const *brx_pal_vk_depth_stencil_attachment_intermediate_image::get_sampled_image() const
{
	assert(this->m_allow_sampled_image);
	assert(VK_NULL_HANDLE != this->m_sampled_image_view);
	return static_cast<brx_pal_vk_sampled_image const *>(this);
}

brx_pal_vk_storage_intermediate_image::brx_pal_vk_storage_intermediate_image() : m_image(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_storage_image_view(VK_NULL_HANDLE), m_sampled_image_view(VK_NULL_HANDLE), m_allow_sampled_image(false)
{
}

void brx_pal_vk_storage_intermediate_image::init(VkDevice device, PFN_vkCreateImageView pfn_create_image_view, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator, VmaPool storage_intermediate_image_memory_pool, BRX_PAL_STORAGE_IMAGE_FORMAT wrapped_storage_image_format, uint32_t width, uint32_t height, bool volume, uint32_t wrapped_depth, bool allow_sampled_image)
{
	VkFormat unwrapped_storage_image_format;
	VkFormat unwrapped_sampled_image_format;
	VkImageCreateFlags image_create_flags;
	switch (wrapped_storage_image_format)
	{
	case BRX_PAL_STORAGE_IMAGE_FORMAT_R32_UINT:
		unwrapped_storage_image_format = VK_FORMAT_R32_UINT;
		unwrapped_sampled_image_format = VK_FORMAT_R32_UINT;
		image_create_flags = 0U;
		break;
	case BRX_PAL_STORAGE_IMAGE_FORMAT_R32_WRITE_UINT_READ_SFLOAT:
		assert(allow_sampled_image);
		unwrapped_storage_image_format = VK_FORMAT_R32_UINT;
		unwrapped_sampled_image_format = VK_FORMAT_R32_SFLOAT;
		image_create_flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		break;
	case BRX_PAL_STORAGE_IMAGE_FORMAT_R16_SFLOAT:
		unwrapped_storage_image_format = VK_FORMAT_R16_SFLOAT;
		unwrapped_sampled_image_format = VK_FORMAT_R16_SFLOAT;
		image_create_flags = 0U;
		break;
	case BRX_PAL_STORAGE_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
		unwrapped_storage_image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
		unwrapped_sampled_image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
		image_create_flags = 0U;
		break;
	default:
		assert(false);
		unwrapped_storage_image_format = VK_FORMAT_UNDEFINED;
		unwrapped_sampled_image_format = VK_FORMAT_UNDEFINED;
		image_create_flags = -1;
	}

	assert(!this->m_allow_sampled_image);
	this->m_allow_sampled_image = allow_sampled_image;

	VkImageType const image_type = (!volume) ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;

	assert((volume) || (1U == wrapped_depth));
	uint32_t const depth = (!volume) ? 1U : wrapped_depth;

	VkImageUsageFlags const usage = allow_sampled_image ? (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT) : VK_IMAGE_USAGE_STORAGE_BIT;

	VkImageViewType const image_view_type = (!volume) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;

	VkImageAspectFlags const aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageCreateInfo const image_create_info = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		NULL,
		image_create_flags,
		image_type,
		unwrapped_storage_image_format,
		{width, height, depth},
		1U,
		1U,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		usage,
		VK_SHARING_MODE_EXCLUSIVE,
		0U,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED};

	VmaAllocationCreateInfo const allocation_create_info = {
		0U,
		VMA_MEMORY_USAGE_UNKNOWN,
		0U,
		0U,
		0U,
		storage_intermediate_image_memory_pool,
		NULL,
		1.0F};

	assert(VK_NULL_HANDLE == this->m_image);
	assert(VK_NULL_HANDLE == this->m_allocation);
	VkResult res_vma_create_buffer = vmaCreateImage(memory_allocator, &image_create_info, &allocation_create_info, &this->m_image, &this->m_allocation, NULL);
	assert(VK_SUCCESS == res_vma_create_buffer);

	{
		VkImageViewCreateInfo const image_view_create_info = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0U,
			this->m_image,
			image_view_type,
			unwrapped_storage_image_format,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{aspect_mask, 0U, 1U, 0U, 1U}};

		assert(VK_NULL_HANDLE == this->m_storage_image_view);
		VkResult const res_create_image_view = pfn_create_image_view(device, &image_view_create_info, allocation_callbacks, &this->m_storage_image_view);
		assert(VK_SUCCESS == res_create_image_view);
	}

	if (allow_sampled_image)
	{
		VkImageViewCreateInfo const image_view_create_info = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			NULL,
			0U,
			this->m_image,
			image_view_type,
			unwrapped_sampled_image_format,
			{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
			{aspect_mask, 0U, 1U, 0U, 1U}};

		assert(VK_NULL_HANDLE == this->m_sampled_image_view);
		VkResult const res_create_image_view = pfn_create_image_view(device, &image_view_create_info, allocation_callbacks, &this->m_sampled_image_view);
		assert(VK_SUCCESS == res_create_image_view);
	}
	else
	{
		assert(VK_NULL_HANDLE == this->m_sampled_image_view);
	}
}

void brx_pal_vk_storage_intermediate_image::uninit(VkDevice device, PFN_vkDestroyImageView pfn_destroy_image_view, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator)
{
	assert(VK_NULL_HANDLE != this->m_storage_image_view);
	pfn_destroy_image_view(device, this->m_storage_image_view, allocation_callbacks);
	this->m_storage_image_view = VK_NULL_HANDLE;

	if (VK_NULL_HANDLE != this->m_sampled_image_view)
	{
		assert(this->m_allow_sampled_image);
		pfn_destroy_image_view(device, this->m_sampled_image_view, allocation_callbacks);
		this->m_sampled_image_view = VK_NULL_HANDLE;
	}
	else
	{
		assert(!this->m_allow_sampled_image);
	}

	assert(VK_NULL_HANDLE != this->m_image);
	assert(VK_NULL_HANDLE != this->m_allocation);

	vmaDestroyImage(memory_allocator, this->m_image, this->m_allocation);

	this->m_image = VK_NULL_HANDLE;
	this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_storage_intermediate_image::~brx_pal_vk_storage_intermediate_image()
{
	assert(VK_NULL_HANDLE == this->m_image);
	assert(VK_NULL_HANDLE == this->m_allocation);
	assert(VK_NULL_HANDLE == this->m_storage_image_view);
	assert(VK_NULL_HANDLE == this->m_sampled_image_view);
}
VkImage brx_pal_vk_storage_intermediate_image::get_image() const
{
	return this->m_image;
}

VkImageView brx_pal_vk_storage_intermediate_image::get_storage_image_view() const
{
	return this->m_storage_image_view;
}

VkImageView brx_pal_vk_storage_intermediate_image::get_sampled_image_view() const
{
	assert(this->m_allow_sampled_image);
	assert(VK_NULL_HANDLE != this->m_sampled_image_view);
	return this->m_sampled_image_view;
}

bool brx_pal_vk_storage_intermediate_image::allow_sampled_image() const
{
	return this->m_allow_sampled_image;
}

brx_pal_sampled_image const *brx_pal_vk_storage_intermediate_image::get_sampled_image() const
{
	assert(this->m_allow_sampled_image);
	assert(VK_NULL_HANDLE != this->m_sampled_image_view);
	return static_cast<brx_pal_vk_sampled_image const *>(this);
}

brx_pal_vk_sampled_asset_image::brx_pal_vk_sampled_asset_image() : m_image(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_image_view(VK_NULL_HANDLE), m_mip_levels(static_cast<uint32_t>(-1))
{
}

void brx_pal_vk_sampled_asset_image::init(VkDevice device, PFN_vkCreateImageView pfn_create_image_view, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator, VmaPool sampled_asset_image_memory_pool, VkFormat format, uint32_t width, uint32_t height, bool array, uint32_t wrapped_array_layers, uint32_t mip_levels)
{
	assert((array) || (1U == wrapped_array_layers));
	uint32_t const array_layers = (!array) ? 1U : wrapped_array_layers;

	VkImageViewType const image_view_type = (!array) ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;

	VkImageCreateInfo const image_create_info = {
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		NULL,
		0U,
		VK_IMAGE_TYPE_2D,
		format,
		{width, height, 1U},
		mip_levels,
		array_layers,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_SHARING_MODE_EXCLUSIVE,
		0U,
		NULL,
		VK_IMAGE_LAYOUT_UNDEFINED};

	VmaAllocationCreateInfo const allocation_create_info = {
		0U,
		VMA_MEMORY_USAGE_UNKNOWN,
		0U,
		0U,
		0U,
		sampled_asset_image_memory_pool,
		NULL,
		1.0F};

	assert(VK_NULL_HANDLE == this->m_image);
	assert(VK_NULL_HANDLE == this->m_allocation);
	VkResult const res_vma_create_image = vmaCreateImage(memory_allocator, &image_create_info, &allocation_create_info, &this->m_image, &this->m_allocation, NULL);
	assert(VK_SUCCESS == res_vma_create_image);

	VkImageViewCreateInfo const image_view_create_info = {
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		NULL,
		0U,
		this->m_image,
		image_view_type,
		format,
		{VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
		{VK_IMAGE_ASPECT_COLOR_BIT, 0U, mip_levels, 0U, array_layers}};

	assert(VK_NULL_HANDLE == this->m_image_view);
	VkResult res_create_image_view = pfn_create_image_view(device, &image_view_create_info, allocation_callbacks, &this->m_image_view);
	assert(VK_SUCCESS == res_create_image_view);

	assert(static_cast<uint32_t>(-1) == this->m_mip_levels);
	this->m_mip_levels = mip_levels;
}

void brx_pal_vk_sampled_asset_image::uninit(VkDevice device, PFN_vkDestroyImageView pfn_destroy_image_view, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator)
{
	if (VK_NULL_HANDLE != this->m_image_view)
	{
		pfn_destroy_image_view(device, this->m_image_view, allocation_callbacks);
		this->m_image_view = VK_NULL_HANDLE;
	}

	assert(VK_NULL_HANDLE != this->m_image);
	assert(VK_NULL_HANDLE != this->m_allocation);
	vmaDestroyImage(memory_allocator, this->m_image, this->m_allocation);
	this->m_image = VK_NULL_HANDLE;
	this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_sampled_asset_image::~brx_pal_vk_sampled_asset_image()
{
	assert(VK_NULL_HANDLE == this->m_image);
	assert(VK_NULL_HANDLE == this->m_allocation);
	assert(VK_NULL_HANDLE == this->m_image_view);
}

VkImage brx_pal_vk_sampled_asset_image::get_image() const
{
	return this->m_image;
}

VkImageView brx_pal_vk_sampled_asset_image::get_sampled_image_view() const
{
	assert(VK_NULL_HANDLE != this->m_image_view);
	return this->m_image_view;
}

brx_pal_sampled_image const *brx_pal_vk_sampled_asset_image::get_sampled_image() const
{
	return static_cast<brx_pal_vk_sampled_image const *>(this);
}
