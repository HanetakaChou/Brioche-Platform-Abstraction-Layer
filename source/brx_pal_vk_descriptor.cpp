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

brx_pal_vk_descriptor_set_layout::brx_pal_vk_descriptor_set_layout()
    : m_descriptor_set_layout(VK_NULL_HANDLE),
      m_dynamic_uniform_buffer_descriptor_count(0U),
      m_storage_buffer_descriptor_count(0U),
      m_sampled_image_descriptor_count(0U),
      m_sampler_descriptor_count(0U),
      m_storage_image_descriptor_count(0U),
      m_top_level_acceleration_structure_descriptor_count(0U),
      m_unbounded_descriptor_type(static_cast<VkDescriptorType>(-1))
{
}

void brx_pal_vk_descriptor_set_layout::init(uint32_t support_ray_tracing, uint32_t descriptor_set_binding_count, BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const *wrapped_descriptor_set_bindings, uint32_t max_per_stage_descriptor_storage_buffers, uint32_t max_per_stage_descriptor_sampled_images, uint32_t max_descriptor_set_storage_buffers, uint32_t max_descriptor_set_sampled_images, PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, VkAllocationCallbacks const *allocation_callbacks)
{
    PFN_vkCreateDescriptorSetLayout const pfn_create_descriptor_set_layout = reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(pfn_get_device_proc_addr(device, "vkCreateDescriptorSetLayout"));
    assert(NULL != pfn_create_descriptor_set_layout);

    // we assume the maximum of other bounded descriptors
    constexpr uint32_t const max_other_bounded_storage_buffers = 16U;
    constexpr uint32_t const max_other_bounded_sampled_images = 16U;

    mcrt_vector<VkDescriptorSetLayoutBinding> descriptor_set_bindings(static_cast<size_t>(descriptor_set_binding_count));
    mcrt_vector<VkDescriptorBindingFlagsEXT> binding_flags(static_cast<size_t>(descriptor_set_binding_count));
    assert(0U == this->m_dynamic_uniform_buffer_descriptor_count);
    assert(0U == this->m_storage_buffer_descriptor_count);
    assert(0U == this->m_sampled_image_descriptor_count);
    assert(0U == this->m_sampler_descriptor_count);
    assert(0U == this->m_storage_image_descriptor_count);
    assert(0U == this->m_top_level_acceleration_structure_descriptor_count);
    for (uint32_t binding_index = 0U; binding_index < descriptor_set_binding_count; ++binding_index)
    {
        descriptor_set_bindings[binding_index].binding = wrapped_descriptor_set_bindings[binding_index].binding;
        switch (wrapped_descriptor_set_bindings[binding_index].descriptor_type)
        {
        case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
        {
            descriptor_set_bindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            assert(1U == wrapped_descriptor_set_bindings[binding_index].descriptor_count);
            descriptor_set_bindings[binding_index].descriptorCount = 1U;
            ++this->m_dynamic_uniform_buffer_descriptor_count;
            binding_flags[binding_index] = 0U;
        }
        break;
        case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
        {
            descriptor_set_bindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            if (BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != wrapped_descriptor_set_bindings[binding_index].descriptor_count)
            {
                descriptor_set_bindings[binding_index].descriptorCount = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
                this->m_storage_buffer_descriptor_count += wrapped_descriptor_set_bindings[binding_index].descriptor_count;
                binding_flags[binding_index] = 0U;
            }
            else
            {
                assert(support_ray_tracing);
                assert(descriptor_set_binding_count == (binding_index + 1U));
                assert(max_per_stage_descriptor_storage_buffers > max_other_bounded_storage_buffers);
                assert(max_descriptor_set_storage_buffers > max_other_bounded_storage_buffers);
                descriptor_set_bindings[binding_index].descriptorCount = std::min(max_per_stage_descriptor_storage_buffers, max_descriptor_set_storage_buffers) - max_other_bounded_storage_buffers;
                assert(static_cast<VkDescriptorType>(-1) == this->m_unbounded_descriptor_type);
                this->m_unbounded_descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                binding_flags[binding_index] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
            }
        }
        break;
        case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            descriptor_set_bindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != wrapped_descriptor_set_bindings[binding_index].descriptor_count);
            descriptor_set_bindings[binding_index].descriptorCount = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
            this->m_storage_buffer_descriptor_count += wrapped_descriptor_set_bindings[binding_index].descriptor_count;
            binding_flags[binding_index] = 0U;
        }
        break;
        case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            descriptor_set_bindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            if (BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != wrapped_descriptor_set_bindings[binding_index].descriptor_count)
            {
                descriptor_set_bindings[binding_index].descriptorCount = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
                this->m_sampled_image_descriptor_count += wrapped_descriptor_set_bindings[binding_index].descriptor_count;
                binding_flags[binding_index] = 0U;
            }
            else
            {
                assert(support_ray_tracing);
                assert(descriptor_set_binding_count == (binding_index + 1U));
                assert(max_per_stage_descriptor_sampled_images > max_other_bounded_sampled_images);
                assert(max_descriptor_set_sampled_images > max_other_bounded_sampled_images);
                descriptor_set_bindings[binding_index].descriptorCount = std::min(max_per_stage_descriptor_sampled_images, max_descriptor_set_sampled_images) - max_other_bounded_sampled_images;
                assert(static_cast<VkDescriptorType>(-1) == this->m_unbounded_descriptor_type);
                this->m_unbounded_descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                binding_flags[binding_index] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
            }
        }
        break;
        case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
        {
            descriptor_set_bindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != wrapped_descriptor_set_bindings[binding_index].descriptor_count);
            descriptor_set_bindings[binding_index].descriptorCount = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
            this->m_sampler_descriptor_count += wrapped_descriptor_set_bindings[binding_index].descriptor_count;
            binding_flags[binding_index] = 0U;
        }
        break;
        case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            descriptor_set_bindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != wrapped_descriptor_set_bindings[binding_index].descriptor_count);
            descriptor_set_bindings[binding_index].descriptorCount = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
            this->m_storage_image_descriptor_count += wrapped_descriptor_set_bindings[binding_index].descriptor_count;
            binding_flags[binding_index] = 0U;
        }
        break;
        case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
        {
            assert(support_ray_tracing);
            descriptor_set_bindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != wrapped_descriptor_set_bindings[binding_index].descriptor_count);
            descriptor_set_bindings[binding_index].descriptorCount = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
            this->m_top_level_acceleration_structure_descriptor_count += wrapped_descriptor_set_bindings[binding_index].descriptor_count;
            binding_flags[binding_index] = 0U;
        }
        break;
        default:
        {
            assert(false);
            descriptor_set_bindings[binding_index].descriptorType = static_cast<VkDescriptorType>(-1);
            descriptor_set_bindings[binding_index].descriptorCount = static_cast<uint32_t>(-1);
            binding_flags[binding_index] = static_cast<uint32_t>(-1);
        }
        }
        // TODO:
        descriptor_set_bindings[binding_index].stageFlags = VK_SHADER_STAGE_ALL;
        descriptor_set_bindings[binding_index].pImmutableSamplers = NULL;
    }

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT const descriptor_set_layout_binding_flags_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
        NULL,
        descriptor_set_binding_count,
        &binding_flags[0]};

    VkDescriptorSetLayoutCreateInfo const descriptor_set_layout_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        support_ray_tracing ? &descriptor_set_layout_binding_flags_create_info : NULL,
        0U,
        descriptor_set_binding_count,
        &descriptor_set_bindings[0]};

    assert(VK_NULL_HANDLE == this->m_descriptor_set_layout);
    VkResult const res_create_global_descriptor_set_layout = pfn_create_descriptor_set_layout(device, &descriptor_set_layout_create_info, allocation_callbacks, &this->m_descriptor_set_layout);
    assert(VK_SUCCESS == res_create_global_descriptor_set_layout);
}

void brx_pal_vk_descriptor_set_layout::uninit(PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, VkAllocationCallbacks const *allocation_callbacks)
{
    PFN_vkDestroyDescriptorSetLayout const pfn_destroy_descriptor_set_layout = reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(pfn_get_device_proc_addr(device, "vkDestroyDescriptorSetLayout"));
    assert(NULL != pfn_destroy_descriptor_set_layout);

    assert(VK_NULL_HANDLE != this->m_descriptor_set_layout);

    pfn_destroy_descriptor_set_layout(device, this->m_descriptor_set_layout, allocation_callbacks);

    this->m_descriptor_set_layout = VK_NULL_HANDLE;
}

brx_pal_vk_descriptor_set_layout::~brx_pal_vk_descriptor_set_layout()
{
    assert(VK_NULL_HANDLE == this->m_descriptor_set_layout);
}

VkDescriptorSetLayout brx_pal_vk_descriptor_set_layout::get_descriptor_set_layout() const
{
    return this->m_descriptor_set_layout;
}

uint32_t brx_pal_vk_descriptor_set_layout::get_dynamic_uniform_buffer_descriptor_count() const
{
    return this->m_dynamic_uniform_buffer_descriptor_count;
}

uint32_t brx_pal_vk_descriptor_set_layout::get_storage_buffer_descriptor_count() const
{
    return this->m_storage_buffer_descriptor_count;
}

uint32_t brx_pal_vk_descriptor_set_layout::get_sampled_image_descriptor_count() const
{
    return this->m_sampled_image_descriptor_count;
}

uint32_t brx_pal_vk_descriptor_set_layout::get_sampler_descriptor_count() const
{
    return this->m_sampler_descriptor_count;
}

uint32_t brx_pal_vk_descriptor_set_layout::get_storage_image_descriptor_count() const
{
    return this->m_storage_image_descriptor_count;
}

uint32_t brx_pal_vk_descriptor_set_layout::get_top_level_acceleration_structure_descriptor_count() const
{
    return this->m_top_level_acceleration_structure_descriptor_count;
}

VkDescriptorType brx_pal_vk_descriptor_set_layout::get_unbounded_descriptor_type() const
{
    return this->m_unbounded_descriptor_type;
}

brx_pal_vk_pipeline_layout::brx_pal_vk_pipeline_layout(VkPipelineLayout pipeline_layout) : m_pipeline_layout(pipeline_layout)
{
}

VkPipelineLayout brx_pal_vk_pipeline_layout::get_pipeline_layout() const
{
    return this->m_pipeline_layout;
}

void brx_pal_vk_pipeline_layout::steal(VkPipelineLayout *out_pipeline_layout)
{
    assert(NULL != out_pipeline_layout);

    (*out_pipeline_layout) = this->m_pipeline_layout;

    this->m_pipeline_layout = VK_NULL_HANDLE;
}

brx_pal_vk_pipeline_layout::~brx_pal_vk_pipeline_layout()
{
    assert(VK_NULL_HANDLE == this->m_pipeline_layout);
}

brx_pal_vk_descriptor_set::brx_pal_vk_descriptor_set() : m_descriptor_pool(VK_NULL_HANDLE), m_descriptor_set(VK_NULL_HANDLE)
{
}

void brx_pal_vk_descriptor_set::init(bool support_ray_tracing, brx_pal_descriptor_set_layout const *wrapped_descriptor_set_layout, uint32_t unbounded_descriptor_count, PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, VkAllocationCallbacks const *allocation_callbacks)
{
    // According to SRT(Shader Resource Table) in PS5, "descriptor set" is essentially a block of GPU-readable memory.

    // In Vulkan, there is no limit for the descriptor pool, and we can use one descriptor pool for each descriptor set.

    assert(NULL != wrapped_descriptor_set_layout);
    brx_pal_vk_descriptor_set_layout const *unwrapped_descriptor_set_layout = static_cast<brx_pal_vk_descriptor_set_layout const *>(wrapped_descriptor_set_layout);
    VkDescriptorSetLayout const descriptor_set_layout = unwrapped_descriptor_set_layout->get_descriptor_set_layout();
    uint32_t const dynamic_uniform_buffer_descriptor_count = unwrapped_descriptor_set_layout->get_dynamic_uniform_buffer_descriptor_count();
    uint32_t storage_buffer_descriptor_count = unwrapped_descriptor_set_layout->get_storage_buffer_descriptor_count();
    uint32_t sampled_image_descriptor_count = unwrapped_descriptor_set_layout->get_sampled_image_descriptor_count();
    uint32_t const sampler_descriptor_count = unwrapped_descriptor_set_layout->get_sampler_descriptor_count();
    uint32_t const storage_image_descriptor_count = unwrapped_descriptor_set_layout->get_storage_image_descriptor_count();
    uint32_t const top_level_acceleration_structure_descriptor_count = unwrapped_descriptor_set_layout->get_top_level_acceleration_structure_descriptor_count();

    bool has_unbounded_descriptor;
    if (support_ray_tracing)
    {
        VkDescriptorType const unbounded_descriptor_type = unwrapped_descriptor_set_layout->get_unbounded_descriptor_type();
        switch (unbounded_descriptor_type)
        {
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            storage_buffer_descriptor_count += unbounded_descriptor_count;
            has_unbounded_descriptor = true;
        }
        break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        {
            sampled_image_descriptor_count += unbounded_descriptor_count;
            has_unbounded_descriptor = true;
        }
        break;
        case (static_cast<VkDescriptorType>(-1)):
        {
            assert(0U == unbounded_descriptor_count);
            has_unbounded_descriptor = false;
        }
        break;
        default:
        {
            assert(false);
            has_unbounded_descriptor = false;
        }
        }
    }
    else
    {
        assert(static_cast<VkDescriptorType>(-1) == unwrapped_descriptor_set_layout->get_unbounded_descriptor_type());
        has_unbounded_descriptor = false;
    }

    PFN_vkCreateDescriptorPool const pfn_create_descriptor_pool = reinterpret_cast<PFN_vkCreateDescriptorPool>(pfn_get_device_proc_addr(device, "vkCreateDescriptorPool"));
    assert(NULL != pfn_create_descriptor_pool);

    mcrt_vector<VkDescriptorPoolSize> descriptor_pool_sizes;
    if (0U < dynamic_uniform_buffer_descriptor_count)
    {
        descriptor_pool_sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, dynamic_uniform_buffer_descriptor_count});
    }
    if (0U < storage_buffer_descriptor_count)
    {
        descriptor_pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storage_buffer_descriptor_count});
    }
    if (0U < sampled_image_descriptor_count)
    {
        descriptor_pool_sizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, sampled_image_descriptor_count});
    }
    if (0U < sampler_descriptor_count)
    {
        descriptor_pool_sizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER, sampler_descriptor_count});
    }
    if (0U < storage_image_descriptor_count)
    {
        descriptor_pool_sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, storage_image_descriptor_count});
    }
    if (0U < top_level_acceleration_structure_descriptor_count)
    {
        descriptor_pool_sizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, top_level_acceleration_structure_descriptor_count});
    }

    VkDescriptorPoolCreateInfo const descriptor_pool_create_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        NULL,
        0U,
        1U,
        static_cast<uint32_t>(descriptor_pool_sizes.size()),
        &descriptor_pool_sizes[0]};

    assert(VK_NULL_HANDLE == this->m_descriptor_pool);
    VkResult const res_create_descriptor_pool = pfn_create_descriptor_pool(device, &descriptor_pool_create_info, allocation_callbacks, &this->m_descriptor_pool);
    assert(VK_SUCCESS == res_create_descriptor_pool);

    PFN_vkAllocateDescriptorSets const pfn_allocate_descriptor_sets = reinterpret_cast<PFN_vkAllocateDescriptorSets>(pfn_get_device_proc_addr(device, "vkAllocateDescriptorSets"));
    assert(NULL != pfn_create_descriptor_pool);

    VkDescriptorSetLayout const descriptor_set_layouts[1] = {descriptor_set_layout};

    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT const descriptor_set_variable_descriptor_count_allocate_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        NULL,
        1U,
        &unbounded_descriptor_count};

    VkDescriptorSetAllocateInfo const descriptor_set_allocate_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        (support_ray_tracing && has_unbounded_descriptor) ? &descriptor_set_variable_descriptor_count_allocate_info : NULL,
        this->m_descriptor_pool,
        sizeof(descriptor_set_layouts) / sizeof(descriptor_set_layouts[0]),
        descriptor_set_layouts};

    assert(VK_NULL_HANDLE == this->m_descriptor_set);
    VkResult const res_allocate_descriptor_sets = pfn_allocate_descriptor_sets(device, &descriptor_set_allocate_info, &this->m_descriptor_set);
    assert(VK_SUCCESS == res_allocate_descriptor_sets);
}

void brx_pal_vk_descriptor_set::uninit(PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, VkAllocationCallbacks const *allocation_callbacks)
{
    assert(VK_NULL_HANDLE != this->m_descriptor_set);
    this->m_descriptor_set = VK_NULL_HANDLE;

    assert(VK_NULL_HANDLE != this->m_descriptor_pool);

    PFN_vkDestroyDescriptorPool const pfn_destroy_descriptor_pool = reinterpret_cast<PFN_vkDestroyDescriptorPool>(pfn_get_device_proc_addr(device, "vkDestroyDescriptorPool"));
    assert(NULL != pfn_destroy_descriptor_pool);

    pfn_destroy_descriptor_pool(device, this->m_descriptor_pool, allocation_callbacks);

    this->m_descriptor_pool = VK_NULL_HANDLE;
}

brx_pal_vk_descriptor_set::~brx_pal_vk_descriptor_set()
{
    assert(VK_NULL_HANDLE == this->m_descriptor_set);
}

void brx_pal_vk_descriptor_set::write_descriptor(PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr, VkDevice device, uint32_t dst_binding, BRX_PAL_DESCRIPTOR_TYPE wrapped_descriptor_type, uint32_t dst_descriptor_start_index, uint32_t src_descriptor_count, brx_pal_uniform_upload_buffer const *const *src_dynamic_uniform_buffers, uint32_t const *src_dynamic_uniform_buffer_ranges, brx_pal_read_only_storage_buffer const *const *src_read_only_storage_buffers, brx_pal_storage_buffer const *const *src_storage_buffers, brx_pal_sampled_image const *const *src_sampled_images, brx_pal_storage_image const *const *src_storage_images, brx_pal_sampler const *const *src_samplers, brx_pal_top_level_acceleration_structure const *const *src_top_level_acceleration_structures)
{
    PFN_vkUpdateDescriptorSets const pfn_update_descriptor_sets = reinterpret_cast<PFN_vkUpdateDescriptorSets>(pfn_get_device_proc_addr(device, "vkUpdateDescriptorSets"));
    assert(NULL != pfn_update_descriptor_sets);

    VkWriteDescriptorSet descriptor_write;
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = this->m_descriptor_set;
    descriptor_write.dstBinding = dst_binding;
    descriptor_write.dstArrayElement = dst_descriptor_start_index;
    descriptor_write.descriptorCount = src_descriptor_count;

    mcrt_vector<VkDescriptorBufferInfo> buffer_info(static_cast<size_t>(src_descriptor_count));
    mcrt_vector<VkDescriptorImageInfo> image_info(static_cast<size_t>(src_descriptor_count));
    mcrt_vector<VkAccelerationStructureKHR> acceleration_structure_info(static_cast<size_t>(src_descriptor_count));
    switch (wrapped_descriptor_type)
    {
    case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
    {
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

        assert(NULL != src_dynamic_uniform_buffers);
        assert(NULL != src_dynamic_uniform_buffer_ranges);
        assert(NULL == src_read_only_storage_buffers);
        assert(NULL == src_storage_buffers);
        assert(NULL == src_sampled_images);
        assert(NULL == src_storage_images);
        assert(NULL == src_samplers);
        assert(NULL == src_top_level_acceleration_structures);
        descriptor_write.pNext = NULL;
        descriptor_write.pImageInfo = NULL;
        descriptor_write.pBufferInfo = &buffer_info[0];
        descriptor_write.pTexelBufferView = NULL;

        for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
        {
            assert(NULL != src_dynamic_uniform_buffers[descriptor_index]);
            buffer_info[descriptor_index].buffer = static_cast<brx_pal_vk_uniform_upload_buffer const *>(src_dynamic_uniform_buffers[descriptor_index])->get_buffer();
            buffer_info[descriptor_index].offset = 0U;
            buffer_info[descriptor_index].range = src_dynamic_uniform_buffer_ranges[descriptor_index];
        }
    }
    break;
    case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
    {
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        assert(NULL == src_dynamic_uniform_buffers);
        assert(NULL == src_dynamic_uniform_buffer_ranges);
        assert(NULL != src_read_only_storage_buffers);
        assert(NULL == src_storage_buffers);
        assert(NULL == src_sampled_images);
        assert(NULL == src_storage_images);
        assert(NULL == src_samplers);
        assert(NULL == src_top_level_acceleration_structures);
        descriptor_write.pNext = NULL;
        descriptor_write.pImageInfo = NULL;
        descriptor_write.pBufferInfo = &buffer_info[0];
        descriptor_write.pTexelBufferView = NULL;

        for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
        {
            assert(NULL != src_read_only_storage_buffers[descriptor_index]);
            buffer_info[descriptor_index].buffer = static_cast<brx_pal_vk_read_only_storage_buffer const *>(src_read_only_storage_buffers[descriptor_index])->get_buffer();
            buffer_info[descriptor_index].offset = 0U;
            buffer_info[descriptor_index].range = static_cast<brx_pal_vk_read_only_storage_buffer const *>(src_read_only_storage_buffers[descriptor_index])->get_size();
        }
    }
    break;
    case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    {
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

        assert(NULL == src_dynamic_uniform_buffers);
        assert(NULL == src_dynamic_uniform_buffer_ranges);
        assert(NULL == src_read_only_storage_buffers);
        assert(NULL != src_storage_buffers);
        assert(NULL == src_sampled_images);
        assert(NULL == src_samplers);
        assert(NULL == src_storage_images);
        assert(NULL == src_top_level_acceleration_structures);
        descriptor_write.pNext = NULL;
        descriptor_write.pImageInfo = NULL;
        descriptor_write.pBufferInfo = &buffer_info[0];
        descriptor_write.pTexelBufferView = NULL;

        for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
        {
            assert(NULL != src_storage_buffers[descriptor_index]);
            buffer_info[descriptor_index].buffer = static_cast<brx_pal_vk_storage_buffer const *>(src_storage_buffers[descriptor_index])->get_buffer();
            buffer_info[descriptor_index].offset = 0U;
            buffer_info[descriptor_index].range = static_cast<brx_pal_vk_storage_buffer const *>(src_storage_buffers[descriptor_index])->get_size();
        }
    }
    break;
    case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    {
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

        assert(NULL == src_dynamic_uniform_buffers);
        assert(NULL == src_dynamic_uniform_buffer_ranges);
        assert(NULL == src_read_only_storage_buffers);
        assert(NULL == src_storage_buffers);
        assert(NULL != src_sampled_images);
        assert(NULL == src_storage_images);
        assert(NULL == src_samplers);
        assert(NULL == src_top_level_acceleration_structures);
        descriptor_write.pNext = NULL;
        descriptor_write.pImageInfo = &image_info[0];
        descriptor_write.pBufferInfo = NULL;
        descriptor_write.pTexelBufferView = NULL;

        for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
        {
            assert(NULL != src_sampled_images[descriptor_index]);
            image_info[descriptor_index].sampler = VK_NULL_HANDLE;
            image_info[descriptor_index].imageView = static_cast<brx_pal_vk_sampled_image const *>(src_sampled_images[descriptor_index])->get_sampled_image_view();
            image_info[descriptor_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }
    break;
    case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
    {
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

        assert(NULL == src_dynamic_uniform_buffers);
        assert(NULL == src_dynamic_uniform_buffer_ranges);
        assert(NULL == src_read_only_storage_buffers);
        assert(NULL == src_storage_buffers);
        assert(NULL == src_sampled_images);
        assert(NULL != src_storage_images);
        assert(NULL == src_samplers);
        assert(NULL == src_top_level_acceleration_structures);
        descriptor_write.pNext = NULL;
        descriptor_write.pImageInfo = &image_info[0];
        descriptor_write.pBufferInfo = NULL;
        descriptor_write.pTexelBufferView = NULL;

        for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
        {
            assert(NULL != src_storage_images[descriptor_index]);
            image_info[descriptor_index].sampler = VK_NULL_HANDLE;
            image_info[descriptor_index].imageView = static_cast<brx_pal_vk_storage_image const *>(src_storage_images[descriptor_index])->get_storage_image_view();
            image_info[descriptor_index].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        }
    }
    break;
    case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
    {
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

        assert(NULL == src_dynamic_uniform_buffers);
        assert(NULL == src_dynamic_uniform_buffer_ranges);
        assert(NULL == src_read_only_storage_buffers);
        assert(NULL == src_storage_buffers);
        assert(NULL == src_sampled_images);
        assert(NULL == src_storage_images);
        assert(NULL != src_samplers);
        assert(NULL == src_top_level_acceleration_structures);
        descriptor_write.pNext = NULL;
        descriptor_write.pImageInfo = &image_info[0];
        descriptor_write.pBufferInfo = NULL;
        descriptor_write.pTexelBufferView = NULL;

        for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
        {
            assert(NULL != src_samplers[descriptor_index]);
            image_info[descriptor_index].sampler = static_cast<brx_pal_vk_sampler const *>(src_samplers[descriptor_index])->get_sampler();
            image_info[descriptor_index].imageView = VK_NULL_HANDLE;
            image_info[descriptor_index].imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }
    break;
    case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
    {
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

        VkWriteDescriptorSetAccelerationStructureKHR const descriptor_write_acceleration_structure = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
            NULL,
            src_descriptor_count,
            &acceleration_structure_info[0]};

        assert(NULL == src_dynamic_uniform_buffers);
        assert(NULL == src_dynamic_uniform_buffer_ranges);
        assert(NULL == src_read_only_storage_buffers);
        assert(NULL == src_storage_buffers);
        assert(NULL == src_sampled_images);
        assert(NULL == src_samplers);
        assert(NULL != src_top_level_acceleration_structures);
        descriptor_write.pNext = &descriptor_write_acceleration_structure;
        descriptor_write.pImageInfo = NULL;
        descriptor_write.pBufferInfo = NULL;
        descriptor_write.pTexelBufferView = NULL;

        for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
        {
            assert(NULL != src_top_level_acceleration_structures[descriptor_index]);
            acceleration_structure_info[descriptor_index] = static_cast<brx_pal_vk_top_level_acceleration_structure const *>(src_top_level_acceleration_structures[descriptor_index])->get_acceleration_structure();
        }
    }
    break;
    default:
    {
        assert(false);
        descriptor_write.descriptorType = static_cast<VkDescriptorType>(-1);
        descriptor_write.pImageInfo = NULL;
        descriptor_write.pBufferInfo = NULL;
        descriptor_write.pTexelBufferView = NULL;
    }
    }

    pfn_update_descriptor_sets(device, 1U, &descriptor_write, 0U, NULL);
}

VkDescriptorSet brx_pal_vk_descriptor_set::get_descriptor_set() const
{
    return this->m_descriptor_set;
}
