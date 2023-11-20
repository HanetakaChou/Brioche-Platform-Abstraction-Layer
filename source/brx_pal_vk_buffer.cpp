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

brx_pal_vk_uniform_upload_buffer::brx_pal_vk_uniform_upload_buffer() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_host_memory_range_base(NULL)
{
}

void brx_pal_vk_uniform_upload_buffer::init(VmaAllocator memory_allocator, VmaPool uniform_upload_buffer_memory_pool, uint32_t size)
{
    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        uniform_upload_buffer_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VmaAllocationInfo allocation_info;
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, &allocation_info);
    assert(VK_SUCCESS == res_vma_create_buffer);

    assert(NULL != allocation_info.pMappedData);
    assert(NULL == this->m_host_memory_range_base);
    this->m_host_memory_range_base = allocation_info.pMappedData;
}

void brx_pal_vk_uniform_upload_buffer::uninit(VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

VkBuffer brx_pal_vk_uniform_upload_buffer::get_buffer() const
{
    return this->m_buffer;
}

brx_pal_vk_uniform_upload_buffer::~brx_pal_vk_uniform_upload_buffer()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
}

void *brx_pal_vk_uniform_upload_buffer::get_host_memory_range_base() const
{
    return this->m_host_memory_range_base;
}

brx_pal_vk_staging_upload_buffer::brx_pal_vk_staging_upload_buffer() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_host_memory_range_base(NULL)
{
}

void brx_pal_vk_staging_upload_buffer::init(VmaAllocator memory_allocator, VmaPool staging_upload_buffer_memory_pool, uint32_t size)
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        staging_upload_buffer_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VmaAllocationInfo allocation_info;
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, &allocation_info);
    assert(VK_SUCCESS == res_vma_create_buffer);

    assert(NULL != allocation_info.pMappedData);
    assert(NULL == this->m_host_memory_range_base);
    this->m_host_memory_range_base = allocation_info.pMappedData;
}

void brx_pal_vk_staging_upload_buffer::uninit(VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_staging_upload_buffer::~brx_pal_vk_staging_upload_buffer()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
}

VkBuffer brx_pal_vk_staging_upload_buffer::get_buffer() const
{
    return this->m_buffer;
}

void *brx_pal_vk_staging_upload_buffer::get_host_memory_range_base() const
{
    return this->m_host_memory_range_base;
}

brx_pal_vk_storage_intermediate_buffer::brx_pal_vk_storage_intermediate_buffer() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_device_memory_range_base(0U), m_size(static_cast<VkDeviceSize>(-1))
{
}

void brx_pal_vk_storage_intermediate_buffer::init(bool support_ray_tracing, VkDevice device, PFN_vkGetBufferDeviceAddressKHR pfn_get_buffer_device_address, VmaAllocator memory_allocator, VmaPool storage_intermediate_buffer_memory_pool, uint32_t size)
{
    VkBufferUsageFlags const usage = (!support_ray_tracing) ? (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) : (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR);

    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        0U,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        storage_intermediate_buffer_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, NULL);
    assert(VK_SUCCESS == res_vma_create_buffer);

    assert(0U == this->m_device_memory_range_base);
    if (support_ray_tracing)
    {
        VkBufferDeviceAddressInfo const buffer_device_address_info = {
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            NULL,
            this->m_buffer};
        this->m_device_memory_range_base = pfn_get_buffer_device_address(device, &buffer_device_address_info);
    }

    assert(static_cast<VkDeviceSize>(-1) == this->m_size);
    this->m_size = size;
}

void brx_pal_vk_storage_intermediate_buffer::uninit(VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_storage_intermediate_buffer::~brx_pal_vk_storage_intermediate_buffer()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
}

VkBuffer brx_pal_vk_storage_intermediate_buffer::get_buffer() const
{
    return this->m_buffer;
}

VkDeviceSize brx_pal_vk_storage_intermediate_buffer::get_size() const
{
    return this->m_size;
}

VkDeviceAddress brx_pal_vk_storage_intermediate_buffer::get_device_memory_range_base() const
{
    return this->m_device_memory_range_base;
}

brx_pal_read_only_storage_buffer const *brx_pal_vk_storage_intermediate_buffer::get_read_only_storage_buffer() const
{
    return static_cast<brx_pal_vk_read_only_storage_buffer const *>(this);
}

brx_pal_storage_buffer const *brx_pal_vk_storage_intermediate_buffer::get_storage_buffer() const
{
    return static_cast<brx_pal_vk_storage_buffer const *>(this);
}

brx_pal_acceleration_structure_build_input_read_only_buffer const *brx_pal_vk_storage_intermediate_buffer::get_acceleration_structure_build_input_read_only_buffer() const
{
    return static_cast<brx_pal_vk_acceleration_structure_build_input_read_only_buffer const *>(this);
}

brx_pal_vk_storage_asset_buffer::brx_pal_vk_storage_asset_buffer() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_device_memory_range_base(0U), m_size(static_cast<VkDeviceSize>(-1))
{
}

void brx_pal_vk_storage_asset_buffer::init(bool support_ray_tracing, VkDevice device, PFN_vkGetBufferDeviceAddressKHR pfn_get_buffer_device_address, VmaAllocator memory_allocator, VmaPool storage_asset_buffer_memory_pool, uint32_t size)
{
    VkBufferUsageFlags const usage = (!support_ray_tracing) ? (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) : (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR);

    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        0U,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        storage_asset_buffer_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, NULL);
    assert(VK_SUCCESS == res_vma_create_buffer);

    assert(0U == this->m_device_memory_range_base);
    if (support_ray_tracing)
    {
        VkBufferDeviceAddressInfo const buffer_device_address_info = {
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            NULL,
            this->m_buffer};
        this->m_device_memory_range_base = pfn_get_buffer_device_address(device, &buffer_device_address_info);
    }

    assert(static_cast<VkDeviceSize>(-1) == this->m_size);
    this->m_size = size;
}

void brx_pal_vk_storage_asset_buffer::uninit(VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_storage_asset_buffer::~brx_pal_vk_storage_asset_buffer()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
}

VkBuffer brx_pal_vk_storage_asset_buffer::get_buffer() const
{
    return this->m_buffer;
}

VkDeviceSize brx_pal_vk_storage_asset_buffer::get_size() const
{
    return this->m_size;
}

VkDeviceAddress brx_pal_vk_storage_asset_buffer::get_device_memory_range_base() const
{
    return this->m_device_memory_range_base;
}

brx_pal_read_only_storage_buffer const *brx_pal_vk_storage_asset_buffer::get_read_only_storage_buffer() const
{
    return static_cast<brx_pal_vk_read_only_storage_buffer const *>(this);
}

brx_pal_acceleration_structure_build_input_read_only_buffer const *brx_pal_vk_storage_asset_buffer::get_acceleration_structure_build_input_read_only_buffer() const
{
    return static_cast<brx_pal_vk_acceleration_structure_build_input_read_only_buffer const *>(this);
}

brx_pal_vk_scratch_buffer::brx_pal_vk_scratch_buffer() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_device_memory_range_base(0U)
{
}

void brx_pal_vk_scratch_buffer::init(VkDevice device, PFN_vkGetBufferDeviceAddressKHR pfn_get_buffer_device_address, VmaAllocator memory_allocator, VmaPool scratch_buffer_memory_pool, uint32_t size)
{
    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        0U,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        scratch_buffer_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, NULL);
    assert(VK_SUCCESS == res_vma_create_buffer);

    assert(0U == this->m_device_memory_range_base);
    VkBufferDeviceAddressInfo const buffer_device_address_info = {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        NULL,
        this->m_buffer};
    this->m_device_memory_range_base = pfn_get_buffer_device_address(device, &buffer_device_address_info);
}

void brx_pal_vk_scratch_buffer::uninit(VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_scratch_buffer::~brx_pal_vk_scratch_buffer()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
}

VkBuffer brx_pal_vk_scratch_buffer::get_buffer() const
{
    return this->m_buffer;
}

VkDeviceAddress brx_pal_vk_scratch_buffer::get_device_memory_range_base() const
{
    return this->m_device_memory_range_base;
}

brx_pal_vk_intermediate_bottom_level_acceleration_structure::brx_pal_vk_intermediate_bottom_level_acceleration_structure() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_acceleration_structure(VK_NULL_HANDLE), m_device_memory_range_base(0U)
{
}

void brx_pal_vk_intermediate_bottom_level_acceleration_structure::init(VkDevice device, PFN_vkCreateAccelerationStructureKHR pfn_create_acceleration_structure, PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_get_acceleration_structure_device_address, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator, VmaPool intermediate_bottom_level_acceleration_structure_memory_pool, uint32_t size)
{
    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        0U,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        intermediate_bottom_level_acceleration_structure_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, NULL);
    assert(VK_SUCCESS == res_vma_create_buffer);

    VkAccelerationStructureCreateInfoKHR const acceleration_structure_create_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        NULL,
        0U,
        this->m_buffer,
        0U,
        size,
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        0U};

    assert(VK_NULL_HANDLE == this->m_acceleration_structure);
    pfn_create_acceleration_structure(device, &acceleration_structure_create_info, allocation_callbacks, &this->m_acceleration_structure);

    VkAccelerationStructureDeviceAddressInfoKHR const acceleration_structure_device_address_info =
        {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            NULL,
            this->m_acceleration_structure};
    assert(0U == this->m_device_memory_range_base);
    this->m_device_memory_range_base = pfn_get_acceleration_structure_device_address(device, &acceleration_structure_device_address_info);
}

void brx_pal_vk_intermediate_bottom_level_acceleration_structure::uninit(VkDevice device, PFN_vkDestroyAccelerationStructureKHR pfn_destroy_acceleration_structure, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_acceleration_structure);

    pfn_destroy_acceleration_structure(device, this->m_acceleration_structure, allocation_callbacks);

    this->m_acceleration_structure = VK_NULL_HANDLE;

    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_intermediate_bottom_level_acceleration_structure::~brx_pal_vk_intermediate_bottom_level_acceleration_structure()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    assert(VK_NULL_HANDLE == this->m_acceleration_structure);
}

VkBuffer brx_pal_vk_intermediate_bottom_level_acceleration_structure::get_buffer() const
{
    return this->m_buffer;
}

VkAccelerationStructureKHR brx_pal_vk_intermediate_bottom_level_acceleration_structure::get_acceleration_structure() const
{
    return this->m_acceleration_structure;
}

void brx_pal_vk_intermediate_bottom_level_acceleration_structure::set_bottom_level_acceleration_structure_geometries(uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *wrapped_bottom_level_acceleration_structure_geometries)
{
    this->m_bottom_level_acceleration_structure_geometries.resize(bottom_level_acceleration_structure_geometry_count);

    for (uint32_t bottom_level_acceleration_structure_geometry_index = 0U; bottom_level_acceleration_structure_geometry_index < bottom_level_acceleration_structure_geometry_count; ++bottom_level_acceleration_structure_geometry_index)
    {
        this->m_bottom_level_acceleration_structure_geometries[bottom_level_acceleration_structure_geometry_index] = wrapped_bottom_level_acceleration_structure_geometries[bottom_level_acceleration_structure_geometry_index];

        // [Acceleration structure update constraints](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#acceleration-structure-update-constraints)
        this->m_bottom_level_acceleration_structure_geometries[bottom_level_acceleration_structure_geometry_index].vertex_position_buffer = NULL;
    }
}

mcrt_vector<BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY> const &brx_pal_vk_intermediate_bottom_level_acceleration_structure::get_bottom_level_acceleration_structure_geometries() const
{
    return this->m_bottom_level_acceleration_structure_geometries;
}

VkDeviceAddress brx_pal_vk_intermediate_bottom_level_acceleration_structure::get_device_memory_range_base() const
{
    return this->m_device_memory_range_base;
}

brx_pal_bottom_level_acceleration_structure const *brx_pal_vk_intermediate_bottom_level_acceleration_structure::get_bottom_level_acceleration_structure() const
{
    return static_cast<brx_pal_bottom_level_acceleration_structure const *>(this);
}

brx_pal_vk_non_compacted_bottom_level_acceleration_structure::brx_pal_vk_non_compacted_bottom_level_acceleration_structure() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_acceleration_structure(VK_NULL_HANDLE)
{
}

void brx_pal_vk_non_compacted_bottom_level_acceleration_structure::init(VkDevice device, PFN_vkCreateAccelerationStructureKHR pfn_create_acceleration_structure, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator, VmaPool non_compacted_bottom_level_acceleration_structure_memory_pool, uint32_t size)
{
    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        0U,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        non_compacted_bottom_level_acceleration_structure_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, NULL);
    assert(VK_SUCCESS == res_vma_create_buffer);

    VkAccelerationStructureCreateInfoKHR const acceleration_structure_create_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        NULL,
        0U,
        this->m_buffer,
        0U,
        size,
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        0U};

    assert(VK_NULL_HANDLE == this->m_acceleration_structure);
    pfn_create_acceleration_structure(device, &acceleration_structure_create_info, allocation_callbacks, &this->m_acceleration_structure);
}

void brx_pal_vk_non_compacted_bottom_level_acceleration_structure::uninit(VkDevice device, PFN_vkDestroyAccelerationStructureKHR pfn_destroy_acceleration_structure, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_acceleration_structure);

    pfn_destroy_acceleration_structure(device, this->m_acceleration_structure, allocation_callbacks);

    this->m_acceleration_structure = VK_NULL_HANDLE;

    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_non_compacted_bottom_level_acceleration_structure::~brx_pal_vk_non_compacted_bottom_level_acceleration_structure()
{
    assert(VK_NULL_HANDLE == this->m_acceleration_structure);
}

VkBuffer brx_pal_vk_non_compacted_bottom_level_acceleration_structure::get_buffer() const
{
    return this->m_buffer;
}

VkAccelerationStructureKHR brx_pal_vk_non_compacted_bottom_level_acceleration_structure::get_acceleration_structure() const
{
    return this->m_acceleration_structure;
}

brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool::brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool() : m_query_pool(VK_NULL_HANDLE)
{
}

void brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool::init(VkDevice device, PFN_vkCreateQueryPool pfn_create_query_pool, VkAllocationCallbacks const *allocation_callbacks, uint32_t query_count)
{

    VkQueryPoolCreateInfo const query_pool_create_info =
        {
            VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            NULL,
            0U,
            VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
            query_count};

    assert(VK_NULL_HANDLE == this->m_query_pool);
    VkResult const res_create_query_pool = pfn_create_query_pool(device, &query_pool_create_info, allocation_callbacks, &this->m_query_pool);
    assert(VK_SUCCESS == res_create_query_pool);
}

void brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool::uninit(VkDevice device, PFN_vkDestroyQueryPool pfn_destroy_query_pool, VkAllocationCallbacks const *allocation_callbacks)
{
    assert(VK_NULL_HANDLE != this->m_query_pool);
    pfn_destroy_query_pool(device, this->m_query_pool, allocation_callbacks);
    this->m_query_pool = VK_NULL_HANDLE;
}

brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool::~brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool()
{
    assert(VK_NULL_HANDLE == this->m_query_pool);
}

VkQueryPool brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool::get_query_pool() const
{
    return this->m_query_pool;
}

brx_pal_vk_compacted_bottom_level_acceleration_structure::brx_pal_vk_compacted_bottom_level_acceleration_structure() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_acceleration_structure(VK_NULL_HANDLE), m_device_memory_range_base(0U)
{
}

void brx_pal_vk_compacted_bottom_level_acceleration_structure::init(VkDevice device, PFN_vkCreateAccelerationStructureKHR pfn_create_acceleration_structure, PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_get_acceleration_structure_device_address, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator, VmaPool compacted_bottom_level_acceleration_structure_memory_pool, uint32_t size)
{
    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        0U,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        compacted_bottom_level_acceleration_structure_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, NULL);
    assert(VK_SUCCESS == res_vma_create_buffer);

    VkAccelerationStructureCreateInfoKHR const acceleration_structure_create_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        NULL,
        0U,
        this->m_buffer,
        0U,
        size,
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        0U};

    assert(VK_NULL_HANDLE == this->m_acceleration_structure);
    pfn_create_acceleration_structure(device, &acceleration_structure_create_info, allocation_callbacks, &this->m_acceleration_structure);

    VkAccelerationStructureDeviceAddressInfoKHR const acceleration_structure_device_address_info =
        {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            NULL,
            this->m_acceleration_structure};
    assert(0U == this->m_device_memory_range_base);
    this->m_device_memory_range_base = pfn_get_acceleration_structure_device_address(device, &acceleration_structure_device_address_info);
}

void brx_pal_vk_compacted_bottom_level_acceleration_structure::uninit(VkDevice device, PFN_vkDestroyAccelerationStructureKHR pfn_destroy_acceleration_structure, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_acceleration_structure);

    pfn_destroy_acceleration_structure(device, this->m_acceleration_structure, allocation_callbacks);

    this->m_acceleration_structure = VK_NULL_HANDLE;

    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_compacted_bottom_level_acceleration_structure::~brx_pal_vk_compacted_bottom_level_acceleration_structure()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    assert(VK_NULL_HANDLE == this->m_acceleration_structure);
}

VkBuffer brx_pal_vk_compacted_bottom_level_acceleration_structure::get_buffer() const
{
    return this->m_buffer;
}

VkAccelerationStructureKHR brx_pal_vk_compacted_bottom_level_acceleration_structure::get_acceleration_structure() const
{
    return this->m_acceleration_structure;
}

VkDeviceAddress brx_pal_vk_compacted_bottom_level_acceleration_structure::get_device_memory_range_base() const
{
    return this->m_device_memory_range_base;
}

brx_pal_bottom_level_acceleration_structure const *brx_pal_vk_compacted_bottom_level_acceleration_structure::get_bottom_level_acceleration_structure() const
{
    return static_cast<brx_pal_bottom_level_acceleration_structure const *>(this);
}

brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer::brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_device_memory_range_base(0U), m_host_memory_range_base(NULL)
{
}

void brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer::init(VkDevice device, PFN_vkGetBufferDeviceAddressKHR pfn_get_buffer_device_address, VmaAllocator memory_allocator, VmaPool top_level_acceleration_structure_instance_upload_buffer_memory_pool, uint32_t instance_count)
{
    uint32_t const size = sizeof(VkAccelerationStructureInstanceKHR) * instance_count;

    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        VMA_ALLOCATION_CREATE_MAPPED_BIT,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        top_level_acceleration_structure_instance_upload_buffer_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VmaAllocationInfo allocation_info;
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, &allocation_info);
    assert(VK_SUCCESS == res_vma_create_buffer);

    assert(0U == this->m_device_memory_range_base);
    VkBufferDeviceAddressInfo const buffer_device_address_info = {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        NULL,
        this->m_buffer};
    this->m_device_memory_range_base = pfn_get_buffer_device_address(device, &buffer_device_address_info);

    assert(NULL != allocation_info.pMappedData);
    assert(NULL == this->m_host_memory_range_base);
    this->m_host_memory_range_base = static_cast<VkAccelerationStructureInstanceKHR *>(allocation_info.pMappedData);
}

void brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer::uninit(VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer::~brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
}

void brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer::write_instance(uint32_t instance_index, BRX_PAL_TOP_LEVEL_ACCELERATION_STRUCTURE_INSTANCE const *wrapped_bottom_top_acceleration_structure_instance)
{
    assert(wrapped_bottom_top_acceleration_structure_instance->instance_id < 0X1000000U);

    brx_pal_vk_bottom_level_acceleration_structure const *const unwrapped_compacted_bottom_level_acceleration_structure = static_cast<brx_pal_vk_bottom_level_acceleration_structure const *>(wrapped_bottom_top_acceleration_structure_instance->bottom_level_acceleration_structure);
    VkDeviceAddress const compacted_bottom_level_acceleration_structure_device_memory_range_base = unwrapped_compacted_bottom_level_acceleration_structure->get_device_memory_range_base();

    this->m_host_memory_range_base[instance_index].transform.matrix[0][0] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[0][0];
    this->m_host_memory_range_base[instance_index].transform.matrix[0][1] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[0][1];
    this->m_host_memory_range_base[instance_index].transform.matrix[0][2] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[0][2];
    this->m_host_memory_range_base[instance_index].transform.matrix[0][3] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[0][3];
    this->m_host_memory_range_base[instance_index].transform.matrix[1][0] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[1][0];
    this->m_host_memory_range_base[instance_index].transform.matrix[1][1] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[1][1];
    this->m_host_memory_range_base[instance_index].transform.matrix[1][2] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[1][2];
    this->m_host_memory_range_base[instance_index].transform.matrix[1][3] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[1][3];
    this->m_host_memory_range_base[instance_index].transform.matrix[2][0] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[2][0];
    this->m_host_memory_range_base[instance_index].transform.matrix[2][1] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[2][1];
    this->m_host_memory_range_base[instance_index].transform.matrix[2][2] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[2][2];
    this->m_host_memory_range_base[instance_index].transform.matrix[2][3] = wrapped_bottom_top_acceleration_structure_instance->transform_matrix[2][3];
    this->m_host_memory_range_base[instance_index].instanceCustomIndex = wrapped_bottom_top_acceleration_structure_instance->instance_id;
    this->m_host_memory_range_base[instance_index].mask = wrapped_bottom_top_acceleration_structure_instance->instance_mask;
    this->m_host_memory_range_base[instance_index].instanceShaderBindingTableRecordOffset = 0U;
    this->m_host_memory_range_base[instance_index].flags = (wrapped_bottom_top_acceleration_structure_instance->force_closest_hit ? VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR : 0U) | (wrapped_bottom_top_acceleration_structure_instance->force_any_hit ? VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR : 0U) | (wrapped_bottom_top_acceleration_structure_instance->disable_back_face_cull ? VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR : 0U) | (wrapped_bottom_top_acceleration_structure_instance->front_ccw ? VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR : 0U);
    this->m_host_memory_range_base[instance_index].accelerationStructureReference = compacted_bottom_level_acceleration_structure_device_memory_range_base;
}

VkBuffer brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer::get_buffer() const
{
    return this->m_buffer;
}

VkDeviceAddress brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer::get_device_memory_range_base() const
{
    return this->m_device_memory_range_base;
}

brx_pal_vk_top_level_acceleration_structure::brx_pal_vk_top_level_acceleration_structure() : m_buffer(VK_NULL_HANDLE), m_allocation(VK_NULL_HANDLE), m_acceleration_structure(VK_NULL_HANDLE), m_instance_count(static_cast<uint32_t>(-1))
{
}

void brx_pal_vk_top_level_acceleration_structure::init(VkDevice device, PFN_vkCreateAccelerationStructureKHR pfn_create_acceleration_structure, PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_get_acceleration_structure_device_address, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator, VmaPool top_level_acceleration_structure_memory_pool, uint32_t size)
{
    VkBufferCreateInfo const buffer_create_info = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        NULL,
        0U,
        size,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VK_SHARING_MODE_EXCLUSIVE,
        0U,
        NULL};

    VmaAllocationCreateInfo const allocation_create_info = {
        0U,
        VMA_MEMORY_USAGE_UNKNOWN,
        0U,
        0U,
        0U,
        top_level_acceleration_structure_memory_pool,
        NULL,
        1.0F};

    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    VkResult const res_vma_create_buffer = vmaCreateBuffer(memory_allocator, &buffer_create_info, &allocation_create_info, &this->m_buffer, &this->m_allocation, NULL);
    assert(VK_SUCCESS == res_vma_create_buffer);

    VkAccelerationStructureCreateInfoKHR const acceleration_structure_create_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        NULL,
        0U,
        this->m_buffer,
        0U,
        size,
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        0U};

    assert(VK_NULL_HANDLE == this->m_acceleration_structure);
    pfn_create_acceleration_structure(device, &acceleration_structure_create_info, allocation_callbacks, &this->m_acceleration_structure);
}

void brx_pal_vk_top_level_acceleration_structure::uninit(VkDevice device, PFN_vkDestroyAccelerationStructureKHR pfn_destroy_acceleration_structure, VkAllocationCallbacks const *allocation_callbacks, VmaAllocator memory_allocator)
{
    assert(VK_NULL_HANDLE != this->m_acceleration_structure);

    pfn_destroy_acceleration_structure(device, this->m_acceleration_structure, allocation_callbacks);

    this->m_acceleration_structure = VK_NULL_HANDLE;

    assert(VK_NULL_HANDLE != this->m_buffer);
    assert(VK_NULL_HANDLE != this->m_allocation);

    vmaDestroyBuffer(memory_allocator, this->m_buffer, this->m_allocation);

    this->m_buffer = VK_NULL_HANDLE;
    this->m_allocation = VK_NULL_HANDLE;
}

brx_pal_vk_top_level_acceleration_structure::~brx_pal_vk_top_level_acceleration_structure()
{
    assert(VK_NULL_HANDLE == this->m_buffer);
    assert(VK_NULL_HANDLE == this->m_allocation);
    assert(VK_NULL_HANDLE == this->m_acceleration_structure);
}

VkBuffer brx_pal_vk_top_level_acceleration_structure::get_buffer() const
{
    return this->m_buffer;
}

VkAccelerationStructureKHR brx_pal_vk_top_level_acceleration_structure::get_acceleration_structure() const
{
    return this->m_acceleration_structure;
}

void brx_pal_vk_top_level_acceleration_structure::set_instance_count(uint32_t instance_count)
{
    assert(static_cast<uint32_t>(-1) == this->m_instance_count);
    this->m_instance_count = instance_count;
}

uint32_t brx_pal_vk_top_level_acceleration_structure::get_instance_count() const
{
    return this->m_instance_count;
}
