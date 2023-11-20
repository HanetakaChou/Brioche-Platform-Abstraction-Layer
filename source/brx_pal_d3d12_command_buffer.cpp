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
#include "brx_pal_d3d12_descriptor_allocator.h"
#include <assert.h>
#include <cstring>
#ifndef NDEBUG
#include <pix.h>
#endif

brx_pal_d3d12_graphics_command_buffer::brx_pal_d3d12_graphics_command_buffer()
    : m_command_allocator(NULL),
      m_command_list(NULL),
      m_descriptor_allocator(NULL),
      m_current_render_pass(NULL),
      m_current_frame_buffer(NULL)
{
}

void brx_pal_d3d12_graphics_command_buffer::init(ID3D12Device *device, bool uma, bool support_ray_tracing, brx_pal_d3d12_descriptor_allocator *descriptor_allocator)
{
    assert(NULL == this->m_command_allocator);
    HRESULT const hr_create_command_allocator = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->m_command_allocator));
    assert(SUCCEEDED(hr_create_command_allocator));

    assert(NULL == this->m_command_list);
    HRESULT const hr_create_command_list = device->CreateCommandList(0U, D3D12_COMMAND_LIST_TYPE_DIRECT, this->m_command_allocator, NULL, IID_PPV_ARGS(&this->m_command_list));
    assert(SUCCEEDED(hr_create_command_list));

    HRESULT const hr_close = this->m_command_list->Close();
    assert(SUCCEEDED(hr_close));

    this->m_uma = uma;

    this->m_support_ray_tracing = support_ray_tracing;

    assert(NULL == this->m_descriptor_allocator);
    this->m_descriptor_allocator = descriptor_allocator;

    assert(NULL == this->m_current_render_pass);

    assert(NULL == this->m_current_frame_buffer);

    assert(0U == this->m_current_vertex_buffer_strides.size());
}

void brx_pal_d3d12_graphics_command_buffer::uninit()
{
    assert(NULL != this->m_command_list);
    this->m_command_list->Release();
    this->m_command_list = NULL;

    assert(NULL != this->m_command_allocator);
    this->m_command_allocator->Release();
    this->m_command_allocator = NULL;
}

brx_pal_d3d12_graphics_command_buffer::~brx_pal_d3d12_graphics_command_buffer()
{
    assert(NULL == this->m_command_allocator);
    assert(NULL == this->m_command_list);
}

ID3D12CommandAllocator *brx_pal_d3d12_graphics_command_buffer::get_command_allocator() const
{
    return this->m_command_allocator;
}

ID3D12GraphicsCommandList4 *brx_pal_d3d12_graphics_command_buffer::get_command_list() const
{
    return this->m_command_list;
}

void brx_pal_d3d12_graphics_command_buffer::begin()
{
    HRESULT hr_reset = this->m_command_list->Reset(this->m_command_allocator, NULL);
    assert(SUCCEEDED(hr_reset));

    this->m_current_vertex_buffer_strides.clear();
}

void brx_pal_d3d12_graphics_command_buffer::acquire(uint32_t storage_asset_buffer_count, brx_pal_storage_asset_buffer const *const *wrapped_storage_asset_buffers, uint32_t sampled_asset_image_subresource_count, BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE const *wrapped_sampled_asset_image_subresources, uint32_t compacted_bottom_level_acceleration_structure_count, brx_pal_compacted_bottom_level_acceleration_structure const *const *wrapped_compacted_bottom_level_acceleration_structures)
{
    mcrt_vector<D3D12_RESOURCE_BARRIER> acquire_barriers(static_cast<size_t>(storage_asset_buffer_count + sampled_asset_image_subresource_count + compacted_bottom_level_acceleration_structure_count));

    for (uint32_t storage_asset_buffer_index = 0U; storage_asset_buffer_index < storage_asset_buffer_count; ++storage_asset_buffer_index)
    {
        ID3D12Resource *const storage_asset_buffer_resource = static_cast<brx_pal_d3d12_storage_asset_buffer const *>(wrapped_storage_asset_buffers[storage_asset_buffer_index])->get_resource();

        acquire_barriers[storage_asset_buffer_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .Transition = {
                storage_asset_buffer_resource,
                0U,
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}};
    }

    for (uint32_t sampled_asset_image_subresource_index = 0U; sampled_asset_image_subresource_index < sampled_asset_image_subresource_count; ++sampled_asset_image_subresource_index)
    {
        ID3D12Resource *const sampled_asset_image_resource = static_cast<brx_pal_d3d12_sampled_asset_image const *>(wrapped_sampled_asset_image_subresources[sampled_asset_image_subresource_index].m_sampled_asset_images)->get_resource();

        acquire_barriers[storage_asset_buffer_count + sampled_asset_image_subresource_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .Transition = {
                sampled_asset_image_resource,
                wrapped_sampled_asset_image_subresources[sampled_asset_image_subresource_index].m_mip_level,
                D3D12_RESOURCE_STATE_COMMON,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}};
    }

    for (uint32_t compacted_bottom_level_acceleration_structure_index = 0U; compacted_bottom_level_acceleration_structure_index < compacted_bottom_level_acceleration_structure_count; ++compacted_bottom_level_acceleration_structure_index)
    {
        ID3D12Resource *asset_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_compacted_bottom_level_acceleration_structure const *>(wrapped_compacted_bottom_level_acceleration_structures[compacted_bottom_level_acceleration_structure_index])->get_resource();

        // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#synchronizing-acceleration-structure-memory-writesreads
        acquire_barriers[storage_asset_buffer_count + sampled_asset_image_subresource_count + compacted_bottom_level_acceleration_structure_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .UAV = {
                asset_acceleration_structure_buffer_resource}};
    }

    if (storage_asset_buffer_count > 0U || sampled_asset_image_subresource_count > 0U || compacted_bottom_level_acceleration_structure_count > 0U)
    {
        this->m_command_list->ResourceBarrier(static_cast<UINT>(acquire_barriers.size()), &acquire_barriers[0]);
    }
}

void brx_pal_d3d12_graphics_command_buffer::begin_debug_utils_label(char const *label_name)
{
#ifndef NDEBUG
    PIXBeginEvent(this->m_command_list, 0, label_name);
#endif
}

void brx_pal_d3d12_graphics_command_buffer::end_debug_utils_label()
{
#ifndef NDEBUG
    PIXEndEvent(this->m_command_list);
#endif
}

void brx_pal_d3d12_graphics_command_buffer::begin_render_pass(brx_pal_render_pass const *wrapped_render_pass, brx_pal_frame_buffer const *wrapped_frame_buffer, uint32_t width, uint32_t height, uint32_t color_clear_value_count, float const (*color_clear_values)[4], float const *depth_clear_value, uint8_t const *stencil_clear_value)
{
    assert(NULL != wrapped_render_pass);
    assert(NULL != wrapped_frame_buffer);
    assert(NULL == this->m_current_render_pass);
    assert(NULL == this->m_current_frame_buffer);
    this->m_current_render_pass = static_cast<brx_pal_d3d12_render_pass const *>(wrapped_render_pass);
    this->m_current_frame_buffer = static_cast<brx_pal_d3d12_frame_buffer const *>(wrapped_frame_buffer);

    uint32_t const color_attachment_count = this->m_current_render_pass->get_color_attachment_count();
    bool const has_depth_stencil_attachment = (BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED != this->m_current_render_pass->get_depth_stencil_attachment_format());

    brx_pal_d3d12_color_attachment_image const *const *const color_attachment_images = this->m_current_frame_buffer->get_color_attachment_images();
    brx_pal_d3d12_depth_stencil_attachment_image const *const depth_stencil_attachment_image = this->m_current_frame_buffer->get_depth_stencil_attachment_image();
    uint32_t const color_attachment_load_clear_count = this->m_current_render_pass->get_color_attachment_load_clear_count();
    uint32_t const *color_attachment_load_clear_indices = this->m_current_render_pass->get_color_attachment_load_clear_indices();
    assert(color_attachment_load_clear_count <= color_attachment_count);

    assert(this->m_current_frame_buffer->get_color_attachment_image_count() == color_attachment_count);
    assert((NULL != depth_stencil_attachment_image) == has_depth_stencil_attachment);

    assert(color_attachment_load_clear_count == color_clear_value_count);
    assert(has_depth_stencil_attachment || (NULL == depth_clear_value));
    assert(has_depth_stencil_attachment || (NULL == stencil_clear_value));

    // Load Barrier
    {
        // According to the specification, the resource should be in the "D3D12_RESOURCE_STATE_RENDER_TARGET" or "D3D12_RESOURCE_STATE_DEPTH_WRITE" state before clear.
        // But the resource barrier is intrinsically the synchronization of the data between different types of the caches in GPU.
        // Since the the content will be cleared later, no synchronization is required to preserve the content.

        // there is no such "VK_IMAGE_LAYOUT_UNDEFINED" state in D3D12
        // we do not really distinguish between "load dont care" and "load load", nor between "store dont care" and "store flush for sampled image"

        // TODO: Enhanced Barriers
        // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html
        // D3D12_BARRIER_LAYOUT_UNDEFINED

        mcrt_vector<D3D12_RESOURCE_BARRIER> load_barriers;
        load_barriers.reserve(color_attachment_count + (has_depth_stencil_attachment ? 1U : 0U));

        for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
        {
            bool const is_color_attachment_store_flush_for_present = this->m_current_render_pass->is_color_attachment_store_flush_for_present(color_attachment_index);

            brx_pal_d3d12_color_attachment_image const *const color_attachment_image = color_attachment_images[color_attachment_index];
            assert(NULL != color_attachment_image);

            bool const allow_sampled_image = color_attachment_image->allow_sampled_image();

            ID3D12Resource *const render_target_resource = color_attachment_image->get_resource();
            assert(NULL != render_target_resource);

            if (!is_color_attachment_store_flush_for_present)
            {
                if (allow_sampled_image)
                {
                    load_barriers.push_back(D3D12_RESOURCE_BARRIER{
                        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                        .Transition = {
                            render_target_resource,
                            0U,
                            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                            D3D12_RESOURCE_STATE_RENDER_TARGET}});
                }
                else
                {
                    // Do Nothing
                }
            }
            else
            {
                assert(!allow_sampled_image);
                load_barriers.push_back(D3D12_RESOURCE_BARRIER{
                    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                    .Transition = {
                        render_target_resource,
                        0U,
                        D3D12_RESOURCE_STATE_PRESENT,
                        D3D12_RESOURCE_STATE_RENDER_TARGET}});
            }
        }

        assert(load_barriers.size() <= color_attachment_count);

        if (has_depth_stencil_attachment)
        {
            assert(NULL != depth_stencil_attachment_image);

            bool const allow_sampled_image = depth_stencil_attachment_image->allow_sampled_image();

            ID3D12Resource *const depth_stencil_resource = depth_stencil_attachment_image->get_resource();
            assert(NULL != depth_stencil_resource);

            if (allow_sampled_image)
            {
                load_barriers.push_back(D3D12_RESOURCE_BARRIER{
                    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                    .Transition = {
                        depth_stencil_resource,
                        0U,
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                        D3D12_RESOURCE_STATE_DEPTH_WRITE}});
            }
            else
            {
                // Do Nothing
            }
        }

        assert(load_barriers.size() <= (color_attachment_count + (has_depth_stencil_attachment ? 1U : 0U)));

        if (!load_barriers.empty())
        {
            this->m_command_list->ResourceBarrier(static_cast<UINT>(load_barriers.size()), &load_barriers[0]);
        }
    }

    // clear
    {
        for (uint32_t clear_index = 0U; clear_index < color_clear_value_count; ++clear_index)
        {
            uint32_t const color_attachment_index = color_attachment_load_clear_indices[clear_index];

            brx_pal_d3d12_color_attachment_image const *const color_attachment_image = color_attachment_images[color_attachment_index];
            assert(NULL != color_attachment_image);

            D3D12_CPU_DESCRIPTOR_HANDLE const render_target_view_descriptor = color_attachment_image->get_render_target_view_descriptor();

            FLOAT const(&color_rgba)[4] = color_clear_values[clear_index];

            this->m_command_list->ClearRenderTargetView(render_target_view_descriptor, color_rgba, 0U, NULL);
        }

        if ((NULL != depth_clear_value) || (NULL != stencil_clear_value))
        {
            D3D12_CLEAR_FLAGS clear_flags;
            FLOAT depth;
            UINT8 stencil;
            if ((NULL != depth_clear_value) && (NULL != stencil_clear_value))
            {
                clear_flags = D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL;
                depth = (*depth_clear_value);
                stencil = (*stencil_clear_value);
            }
            else if (NULL != depth_clear_value)
            {
                clear_flags = D3D12_CLEAR_FLAG_DEPTH;
                depth = (*depth_clear_value);
                stencil = 0U;
            }
            else
            {
                assert(NULL != stencil_clear_value);
                clear_flags = D3D12_CLEAR_FLAG_STENCIL;
                depth = 0.0F;
                stencil = (*stencil_clear_value);
            }

            D3D12_CPU_DESCRIPTOR_HANDLE const depth_stencil_view_descriptor = depth_stencil_attachment_image->get_depth_stencil_view_descriptor();

            this->m_command_list->ClearDepthStencilView(depth_stencil_view_descriptor, clear_flags, depth, stencil, 0U, NULL);
        }
    }

    {
        mcrt_vector<D3D12_CPU_DESCRIPTOR_HANDLE> render_target_view_descriptors(static_cast<size_t>(color_attachment_count));
        for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
        {
            render_target_view_descriptors[color_attachment_index] = color_attachment_images[color_attachment_index]->get_render_target_view_descriptor();
        }

        D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_view_descriptor = (has_depth_stencil_attachment ? depth_stencil_attachment_image->get_depth_stencil_view_descriptor() : D3D12_CPU_DESCRIPTOR_HANDLE{});

        this->m_command_list->OMSetRenderTargets(color_attachment_count, render_target_view_descriptors.data(), FALSE, (has_depth_stencil_attachment ? &depth_stencil_view_descriptor : NULL));
    }
}

void brx_pal_d3d12_graphics_command_buffer::bind_graphics_pipeline(brx_pal_graphics_pipeline const *wrapped_graphics_pipeline)
{
    assert(NULL != wrapped_graphics_pipeline);
    brx_pal_d3d12_graphics_pipeline const *const unwrapped_graphics_pipeline = static_cast<brx_pal_d3d12_graphics_pipeline const *>(wrapped_graphics_pipeline);

    D3D12_PRIMITIVE_TOPOLOGY const primitive_topology = unwrapped_graphics_pipeline->get_primitive_topology();
    ID3D12PipelineState *graphics_pipeline = unwrapped_graphics_pipeline->get_pipeline();

    this->m_command_list->IASetPrimitiveTopology(primitive_topology);

    this->m_command_list->SetPipelineState(graphics_pipeline);
}

void brx_pal_d3d12_graphics_command_buffer::set_view_port(uint32_t width, uint32_t height)
{
    D3D12_VIEWPORT const view_port = {0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height), 0.0F, 1.0F};
    this->m_command_list->RSSetViewports(1U, &view_port);
}

void brx_pal_d3d12_graphics_command_buffer::set_scissor(int32_t offset_width, int32_t offset_height, uint32_t width, uint32_t height)
{
    D3D12_RECT const rect = {static_cast<LONG>(offset_width), static_cast<LONG>(offset_height), static_cast<LONG>(offset_width + width), static_cast<LONG>(offset_height + height)};
    this->m_command_list->RSSetScissorRects(1U, &rect);
}

void brx_pal_d3d12_graphics_command_buffer::bind_graphics_descriptor_sets(brx_pal_pipeline_layout const *wrapped_pipeline_layout, uint32_t descriptor_set_count, brx_pal_descriptor_set const *const *wrapped_descriptor_sets, uint32_t dynamic_offet_count, uint32_t const *dynamic_offsets)
{
    assert(NULL != wrapped_pipeline_layout);
    assert(NULL != wrapped_descriptor_sets);

    brx_pal_d3d12_pipeline_set_descriptor_heap const *const pipeline_set_descriptor_heap = this->m_descriptor_allocator->create_or_get_pipeline_set_descriptor_heap(descriptor_set_count, wrapped_descriptor_sets);
    assert(descriptor_set_count == pipeline_set_descriptor_heap->m_descriptor_sets_base_descriptor_handle.size());

    if (NULL != pipeline_set_descriptor_heap->m_cbv_srv_uav_descriptor_heap && NULL != pipeline_set_descriptor_heap->m_sampler_descriptor_heap)
    {
        ID3D12DescriptorHeap *const descriptor_heaps[] = {pipeline_set_descriptor_heap->m_cbv_srv_uav_descriptor_heap, pipeline_set_descriptor_heap->m_sampler_descriptor_heap};
        this->m_command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
    }
    else if (NULL != pipeline_set_descriptor_heap->m_cbv_srv_uav_descriptor_heap)
    {
        ID3D12DescriptorHeap *const descriptor_heaps[] = {pipeline_set_descriptor_heap->m_cbv_srv_uav_descriptor_heap};
        this->m_command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
    }
    else if (NULL != pipeline_set_descriptor_heap->m_sampler_descriptor_heap)
    {
        ID3D12DescriptorHeap *const descriptor_heaps[] = {pipeline_set_descriptor_heap->m_sampler_descriptor_heap};
        this->m_command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
    }
    else
    {
        // Do Nothing
    }

    ID3D12RootSignature *const root_signature = static_cast<brx_pal_d3d12_pipeline_layout const *>(wrapped_pipeline_layout)->get_root_signature();
    this->m_command_list->SetGraphicsRootSignature(root_signature);

    uint32_t root_parameter_index = 0U;
    uint32_t dynamic_offset_index = 0U;
    for (uint32_t descriptor_set_index = 0U; descriptor_set_index < descriptor_set_count; ++descriptor_set_index)
    {
        brx_pal_d3d12_descriptor_set const *const descriptor_set = static_cast<brx_pal_d3d12_descriptor_set const *>(wrapped_descriptor_sets[descriptor_set_index]);
        assert(NULL != descriptor_set);

        uint32_t const binding_count = descriptor_set->get_descriptor_binding_count();
        assert(binding_count == pipeline_set_descriptor_heap->m_descriptor_sets_base_descriptor_handle[descriptor_set_index].size());
        brx_pal_d3d12_descriptor_binding const *const descriptor_bindings = descriptor_set->get_descriptor_bindings();

        for (uint32_t binding_index = 0U; binding_index < binding_count; ++binding_index)
        {
            brx_pal_d3d12_descriptor_binding const &descriptor_binding = descriptor_bindings[binding_index];

            D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor_handle = pipeline_set_descriptor_heap->m_descriptor_sets_base_descriptor_handle[descriptor_set_index][binding_index];

            switch (descriptor_binding.m_root_parameter_type)
            {
            case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
            {
                assert(INVALID_BASE_DESCRIPTOR_HANDLE.ptr == base_descriptor_handle.ptr);

                D3D12_GPU_VIRTUAL_ADDRESS const buffer_location = {descriptor_binding.m_root_constant_buffer_view_address_base + dynamic_offsets[dynamic_offset_index]};
                this->m_command_list->SetGraphicsRootConstantBufferView(root_parameter_index, buffer_location);
                ++root_parameter_index;
                ++dynamic_offset_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetGraphicsRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            default:
            {
                assert(false);
            }
            }
        }
    }
    assert(dynamic_offset_index == dynamic_offet_count);
}

void brx_pal_d3d12_graphics_command_buffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    this->m_command_list->DrawInstanced(vertex_count, instance_count, first_vertex, first_instance);
}

void brx_pal_d3d12_graphics_command_buffer::end_render_pass()
{
    assert(NULL != this->m_current_render_pass);
    assert(NULL != this->m_current_frame_buffer);

    uint32_t const color_attachment_count = this->m_current_render_pass->get_color_attachment_count();
    bool const has_depth_stencil_attachment = (BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED != this->m_current_render_pass->get_depth_stencil_attachment_format());

    brx_pal_d3d12_color_attachment_image const *const *const color_attachment_images = this->m_current_frame_buffer->get_color_attachment_images();
    brx_pal_d3d12_depth_stencil_attachment_image const *const depth_stencil_attachment_image = this->m_current_frame_buffer->get_depth_stencil_attachment_image();

    assert(this->m_current_frame_buffer->get_color_attachment_image_count() == color_attachment_count);
    assert((NULL != depth_stencil_attachment_image) == has_depth_stencil_attachment);

    // store barrier
    {
        mcrt_vector<D3D12_RESOURCE_BARRIER> store_barriers;
        store_barriers.reserve(color_attachment_count + (has_depth_stencil_attachment ? 1U : 0U));

        for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
        {
            bool const is_color_attachment_store_flush_for_present = this->m_current_render_pass->is_color_attachment_store_flush_for_present(color_attachment_index);

            brx_pal_d3d12_color_attachment_image const *const color_attachment_image = color_attachment_images[color_attachment_index];
            assert(NULL != color_attachment_image);

            bool const allow_sampled_image = color_attachment_image->allow_sampled_image();

            ID3D12Resource *const render_target_resource = color_attachment_image->get_resource();
            assert(NULL != render_target_resource);

            if (!is_color_attachment_store_flush_for_present)
            {
                if (allow_sampled_image)
                {
                    store_barriers.push_back(D3D12_RESOURCE_BARRIER{
                        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                        .Transition = {
                            render_target_resource,
                            0U,
                            D3D12_RESOURCE_STATE_RENDER_TARGET,
                            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}});
                }
                else
                {
                    // Do Nothing
                }
            }
            else
            {

                assert(!allow_sampled_image);
                store_barriers.push_back(D3D12_RESOURCE_BARRIER{
                    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                    .Transition = {
                        render_target_resource,
                        0U,
                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                        D3D12_RESOURCE_STATE_PRESENT}});
            }
        }

        assert(store_barriers.size() <= color_attachment_count);

        if (has_depth_stencil_attachment)
        {
            assert(NULL != depth_stencil_attachment_image);

            bool const allow_sampled_image = depth_stencil_attachment_image->allow_sampled_image();

            ID3D12Resource *const depth_stencil_resource = depth_stencil_attachment_image->get_resource();
            assert(NULL != depth_stencil_resource);

            if (allow_sampled_image)
            {
                store_barriers.push_back(D3D12_RESOURCE_BARRIER{
                    .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                    .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                    .Transition = {
                        depth_stencil_resource,
                        0U,
                        D3D12_RESOURCE_STATE_DEPTH_WRITE,
                        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}});
            }
            else
            {
                // Do Nothing
            }
        }

        assert(store_barriers.size() <= (color_attachment_count + (has_depth_stencil_attachment ? 1U : 0U)));

        if (!store_barriers.empty())
        {
            this->m_command_list->ResourceBarrier(static_cast<UINT>(store_barriers.size()), &store_barriers[0]);
        }
    }

    this->m_current_render_pass = NULL;
    this->m_current_frame_buffer = NULL;
}

void brx_pal_d3d12_graphics_command_buffer::bind_compute_pipeline(brx_pal_compute_pipeline const *wrapped_compute_pipeline)
{
    assert(NULL != wrapped_compute_pipeline);
    brx_pal_d3d12_compute_pipeline const *const unwrapped_compute_pipeline = static_cast<brx_pal_d3d12_compute_pipeline const *>(wrapped_compute_pipeline);

    ID3D12PipelineState *compute_pipeline = unwrapped_compute_pipeline->get_pipeline();

    this->m_command_list->SetPipelineState(compute_pipeline);
}

void brx_pal_d3d12_graphics_command_buffer::bind_compute_descriptor_sets(brx_pal_pipeline_layout const *wrapped_pipeline_layout, uint32_t descriptor_set_count, brx_pal_descriptor_set const *const *wrapped_descriptor_sets, uint32_t dynamic_offet_count, uint32_t const *dynamic_offsets)
{
    assert(NULL != wrapped_pipeline_layout);
    assert(NULL != wrapped_descriptor_sets);

    brx_pal_d3d12_pipeline_set_descriptor_heap const *const pipeline_set_descriptor_heap = this->m_descriptor_allocator->create_or_get_pipeline_set_descriptor_heap(descriptor_set_count, wrapped_descriptor_sets);
    assert(descriptor_set_count == pipeline_set_descriptor_heap->m_descriptor_sets_base_descriptor_handle.size());

    if (NULL != pipeline_set_descriptor_heap->m_cbv_srv_uav_descriptor_heap && NULL != pipeline_set_descriptor_heap->m_sampler_descriptor_heap)
    {
        ID3D12DescriptorHeap *const descriptor_heaps[] = {pipeline_set_descriptor_heap->m_cbv_srv_uav_descriptor_heap, pipeline_set_descriptor_heap->m_sampler_descriptor_heap};
        this->m_command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
    }
    else if (NULL != pipeline_set_descriptor_heap->m_cbv_srv_uav_descriptor_heap)
    {
        ID3D12DescriptorHeap *const descriptor_heaps[] = {pipeline_set_descriptor_heap->m_cbv_srv_uav_descriptor_heap};
        this->m_command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
    }
    else if (NULL != pipeline_set_descriptor_heap->m_sampler_descriptor_heap)
    {
        ID3D12DescriptorHeap *const descriptor_heaps[] = {pipeline_set_descriptor_heap->m_sampler_descriptor_heap};
        this->m_command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
    }
    else
    {
        // Do Nothing
    }

    ID3D12RootSignature *const root_signature = static_cast<brx_pal_d3d12_pipeline_layout const *>(wrapped_pipeline_layout)->get_root_signature();
    this->m_command_list->SetComputeRootSignature(root_signature);

    uint32_t root_parameter_index = 0U;
    uint32_t dynamic_offset_index = 0U;
    for (uint32_t descriptor_set_index = 0U; descriptor_set_index < descriptor_set_count; ++descriptor_set_index)
    {
        brx_pal_d3d12_descriptor_set const *const descriptor_set = static_cast<brx_pal_d3d12_descriptor_set const *>(wrapped_descriptor_sets[descriptor_set_index]);
        assert(NULL != descriptor_set);

        uint32_t const binding_count = descriptor_set->get_descriptor_binding_count();
        assert(binding_count == pipeline_set_descriptor_heap->m_descriptor_sets_base_descriptor_handle[descriptor_set_index].size());
        brx_pal_d3d12_descriptor_binding const *const descriptor_bindings = descriptor_set->get_descriptor_bindings();

        for (uint32_t binding_index = 0U; binding_index < binding_count; ++binding_index)
        {
            brx_pal_d3d12_descriptor_binding const &descriptor_binding = descriptor_bindings[binding_index];

            D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor_handle = pipeline_set_descriptor_heap->m_descriptor_sets_base_descriptor_handle[descriptor_set_index][binding_index];

            switch (descriptor_binding.m_root_parameter_type)
            {
            case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
            {
                assert(INVALID_BASE_DESCRIPTOR_HANDLE.ptr == base_descriptor_handle.ptr);

                D3D12_GPU_VIRTUAL_ADDRESS const buffer_location = {descriptor_binding.m_root_constant_buffer_view_address_base + dynamic_offsets[dynamic_offset_index]};
                this->m_command_list->SetComputeRootConstantBufferView(root_parameter_index, buffer_location);
                ++root_parameter_index;
                ++dynamic_offset_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetComputeRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetComputeRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetComputeRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetComputeRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetComputeRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
            {
                D3D12_GPU_DESCRIPTOR_HANDLE const base_descriptor = base_descriptor_handle;
                this->m_command_list->SetComputeRootDescriptorTable(root_parameter_index, base_descriptor);
                ++root_parameter_index;
            }
            break;
            default:
            {
                assert(false);
            }
            }
        }
    }
    assert(dynamic_offset_index == dynamic_offet_count);
}

void brx_pal_d3d12_graphics_command_buffer::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    this->m_command_list->Dispatch(group_count_x, group_count_y, group_count_z);
}

void brx_pal_d3d12_graphics_command_buffer::storage_resource_load_dont_care(uint32_t storage_buffer_count, brx_pal_storage_buffer const *const *storage_buffers, uint32_t storage_image_count, brx_pal_storage_image const *const *storage_images)
{
    // there is no such "VK_IMAGE_LAYOUT_UNDEFINED" state in D3D12
    // we do not really distinguish between "load dont care" and "load load"

    // TODO: Enhanced Barriers
    // https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html
    // D3D12_BARRIER_LAYOUT_UNDEFINED

    return this->storage_resource_load_load(storage_buffer_count, storage_buffers, storage_image_count, storage_images);
}

void brx_pal_d3d12_graphics_command_buffer::storage_resource_load_load(uint32_t storage_buffer_count, brx_pal_storage_buffer const *const *wrapped_storage_buffers, uint32_t storage_image_count, brx_pal_storage_image const *const *wrapped_storage_images)
{
    mcrt_vector<D3D12_RESOURCE_BARRIER> load_barriers(static_cast<size_t>(storage_buffer_count + storage_image_count));

    for (uint32_t storage_buffer_index = 0U; storage_buffer_index < storage_buffer_count; ++storage_buffer_index)
    {
        ID3D12Resource *const storage_buffer_resource = static_cast<brx_pal_d3d12_storage_buffer const *>(wrapped_storage_buffers[storage_buffer_index])->get_resource();

        load_barriers[storage_buffer_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .Transition = {
                storage_buffer_resource,
                0U,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS}};
    }

    for (uint32_t storage_image_index = 0U; storage_image_index < storage_image_count; ++storage_image_index)
    {
        bool const allow_sampled_image = static_cast<brx_pal_d3d12_storage_image const *>(wrapped_storage_images[storage_image_index])->allow_sampled_image();
        ID3D12Resource *const storage_image_resource = static_cast<brx_pal_d3d12_storage_image const *>(wrapped_storage_images[storage_image_index])->get_resource();

        if (allow_sampled_image)
        {
            load_barriers[storage_buffer_count + storage_image_index] = D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .Transition = {
                    storage_image_resource,
                    0U,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS}};
        }
        else
        {
            load_barriers[storage_buffer_count + storage_image_index] = D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .UAV = {
                    storage_image_resource}};
        }
    }

    this->m_command_list->ResourceBarrier(static_cast<UINT>(load_barriers.size()), load_barriers.data());
}

void brx_pal_d3d12_graphics_command_buffer::storage_resource_barrier(uint32_t storage_buffer_count, brx_pal_storage_buffer const *const *wrapped_storage_buffers, uint32_t storage_image_count, brx_pal_storage_image const *const *wrapped_storage_images)
{
    mcrt_vector<D3D12_RESOURCE_BARRIER> intermediate_barriers(static_cast<size_t>(storage_buffer_count + storage_image_count));

    for (uint32_t storage_buffer_index = 0U; storage_buffer_index < storage_buffer_count; ++storage_buffer_index)
    {
        ID3D12Resource *const storage_buffer_resource = static_cast<brx_pal_d3d12_storage_buffer const *>(wrapped_storage_buffers[storage_buffer_index])->get_resource();

        intermediate_barriers[storage_buffer_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .UAV = {
                storage_buffer_resource}};
    }

    for (uint32_t storage_image_index = 0U; storage_image_index < storage_image_count; ++storage_image_index)
    {
        ID3D12Resource *const storage_image_resource = static_cast<brx_pal_d3d12_storage_image const *>(wrapped_storage_images[storage_image_index])->get_resource();

        intermediate_barriers[storage_buffer_count + storage_image_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .UAV = {
                storage_image_resource}};
    }

    this->m_command_list->ResourceBarrier(static_cast<UINT>(intermediate_barriers.size()), intermediate_barriers.data());
}

void brx_pal_d3d12_graphics_command_buffer::storage_resource_store(uint32_t storage_buffer_count, brx_pal_storage_buffer const *const *wrapped_storage_buffers, uint32_t storage_image_count, brx_pal_storage_image const *const *wrapped_storage_images)
{
    mcrt_vector<D3D12_RESOURCE_BARRIER> store_barriers(static_cast<size_t>(storage_buffer_count + storage_image_count));

    for (uint32_t storage_buffer_index = 0U; storage_buffer_index < storage_buffer_count; ++storage_buffer_index)
    {
        ID3D12Resource *const storage_buffer_resource = static_cast<brx_pal_d3d12_storage_buffer const *>(wrapped_storage_buffers[storage_buffer_index])->get_resource();

        store_barriers[storage_buffer_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .Transition = {
                storage_buffer_resource,
                0U,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}};
    }

    for (uint32_t storage_image_index = 0U; storage_image_index < storage_image_count; ++storage_image_index)
    {
        bool const allow_sampled_image = static_cast<brx_pal_d3d12_storage_image const *>(wrapped_storage_images[storage_image_index])->allow_sampled_image();
        ID3D12Resource *const storage_image_resource = static_cast<brx_pal_d3d12_storage_image const *>(wrapped_storage_images[storage_image_index])->get_resource();

        if (allow_sampled_image)
        {
            store_barriers[storage_buffer_count + storage_image_index] = D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .Transition = {
                    storage_image_resource,
                    0U,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}};
        }
        else
        {
            store_barriers[storage_buffer_count + storage_image_index] = D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .UAV = {
                    storage_image_resource}};
        }
    }

    this->m_command_list->ResourceBarrier(static_cast<UINT>(store_barriers.size()), store_barriers.data());
}

void brx_pal_d3d12_graphics_command_buffer::build_intermediate_bottom_level_acceleration_structure(brx_pal_intermediate_bottom_level_acceleration_structure *wrapped_intermediate_bottom_level_acceleration_structure, uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *wrapped_bottom_level_acceleration_structure_geometries, brx_pal_scratch_buffer *wrapped_scratch_buffer)
{
    assert(NULL != wrapped_intermediate_bottom_level_acceleration_structure);
    D3D12_GPU_VIRTUAL_ADDRESS const destination_acceleration_structure_device_memory_range_base = static_cast<brx_pal_d3d12_intermediate_bottom_level_acceleration_structure *>(wrapped_intermediate_bottom_level_acceleration_structure)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (destination_acceleration_structure_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    assert(NULL != wrapped_bottom_level_acceleration_structure_geometries);

    mcrt_vector<D3D12_RAYTRACING_GEOMETRY_DESC> ray_tracing_geometry_descs(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));

    for (uint32_t bottom_level_acceleration_structure_geometry_index = 0U; bottom_level_acceleration_structure_geometry_index < bottom_level_acceleration_structure_geometry_count; ++bottom_level_acceleration_structure_geometry_index)
    {
        BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const &wrapped_bottom_level_acceleration_structure_geometry = wrapped_bottom_level_acceleration_structure_geometries[bottom_level_acceleration_structure_geometry_index];

        ID3D12Resource *const unwrapped_vertex_position_buffer_resource = static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.vertex_position_buffer)->get_resource();

        ID3D12Resource *const unwrapped_index_buffer_resource = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.index_buffer)->get_resource() : NULL;

        DXGI_FORMAT vertex_position_attribute_format;
        switch (wrapped_bottom_level_acceleration_structure_geometry.vertex_position_attribute_format)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_VERTEX_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT:
            vertex_position_attribute_format = DXGI_FORMAT_R32G32B32_FLOAT;
            break;
        default:
            // VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR
            assert(false);
            vertex_position_attribute_format = static_cast<DXGI_FORMAT>(-1);
            break;
        }

        D3D12_GPU_VIRTUAL_ADDRESS const vertex_position_buffer_device_memory_range_base = unwrapped_vertex_position_buffer_resource->GetGPUVirtualAddress();

        DXGI_FORMAT index_format;
        switch (wrapped_bottom_level_acceleration_structure_geometry.index_type)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT32:
            index_format = DXGI_FORMAT_R32_UINT;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT16:
            index_format = DXGI_FORMAT_R16_UINT;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE:
            index_format = DXGI_FORMAT_UNKNOWN;
            break;
        default:
            assert(false);
            index_format = static_cast<DXGI_FORMAT>(-1);
        }

        D3D12_GPU_VIRTUAL_ADDRESS const index_buffer_device_memory_range_base = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? unwrapped_index_buffer_resource->GetGPUVirtualAddress() : NULL;

        ray_tracing_geometry_descs[bottom_level_acceleration_structure_geometry_index] = D3D12_RAYTRACING_GEOMETRY_DESC{
            D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
            wrapped_bottom_level_acceleration_structure_geometry.force_closest_hit ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE,
            {.Triangles = {
                 0U,
                 index_format,
                 vertex_position_attribute_format,
                 (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? wrapped_bottom_level_acceleration_structure_geometry.index_count : 0U,
                 wrapped_bottom_level_acceleration_structure_geometry.vertex_count,
                 index_buffer_device_memory_range_base,
                 {vertex_position_buffer_device_memory_range_base, wrapped_bottom_level_acceleration_structure_geometry.vertex_position_binding_stride}

             }}};
    }

    assert(NULL != wrapped_scratch_buffer);
    D3D12_GPU_VIRTUAL_ADDRESS const scratch_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_scratch_buffer *>(wrapped_scratch_buffer)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (scratch_buffer_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    assert(bottom_level_acceleration_structure_geometry_count == ray_tracing_geometry_descs.size());
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC const ray_tracing_acceleration_structure_desc = {
        destination_acceleration_structure_device_memory_range_base,
        {D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
         static_cast<UINT>(ray_tracing_geometry_descs.size()),
         D3D12_ELEMENTS_LAYOUT_ARRAY,
         {.pGeometryDescs = &ray_tracing_geometry_descs[0]}},
        0U,
        scratch_buffer_device_memory_range_base};

    this->m_command_list->BuildRaytracingAccelerationStructure(&ray_tracing_acceleration_structure_desc, 0U, NULL);

    static_cast<brx_pal_d3d12_intermediate_bottom_level_acceleration_structure *>(wrapped_intermediate_bottom_level_acceleration_structure)->set_bottom_level_acceleration_structure_geometries(bottom_level_acceleration_structure_geometry_count, wrapped_bottom_level_acceleration_structure_geometries);
}

void brx_pal_d3d12_graphics_command_buffer::build_intermediate_bottom_level_acceleration_structure_store(uint32_t intermediate_bottom_level_acceleration_structure_count, brx_pal_intermediate_bottom_level_acceleration_structure const *const *wrapped_intermediate_bottom_level_acceleration_structures)
{
    mcrt_vector<D3D12_RESOURCE_BARRIER> store_barriers(static_cast<size_t>(intermediate_bottom_level_acceleration_structure_count));

    for (uint32_t intermediate_bottom_level_acceleration_structure_index = 0U; intermediate_bottom_level_acceleration_structure_index < intermediate_bottom_level_acceleration_structure_count; ++intermediate_bottom_level_acceleration_structure_index)
    {
        ID3D12Resource *const unwrapped_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_intermediate_bottom_level_acceleration_structure const *>(wrapped_intermediate_bottom_level_acceleration_structures[intermediate_bottom_level_acceleration_structure_index])->get_resource();

        // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#synchronizing-acceleration-structure-memory-writesreads
        store_barriers[intermediate_bottom_level_acceleration_structure_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .UAV = {
                unwrapped_acceleration_structure_buffer_resource}};
    }

    this->m_command_list->ResourceBarrier(static_cast<UINT>(store_barriers.size()), store_barriers.data());
}

void brx_pal_d3d12_graphics_command_buffer::update_intermediate_bottom_level_acceleration_structure(brx_pal_intermediate_bottom_level_acceleration_structure *wrapped_intermediate_bottom_level_acceleration_structure, brx_pal_acceleration_structure_build_input_read_only_buffer const *const *wrapped_bottom_level_acceleration_structure_geometry_vertex_position_buffers, brx_pal_scratch_buffer *wrapped_scratch_buffer)
{
    assert(NULL != wrapped_intermediate_bottom_level_acceleration_structure);
    D3D12_GPU_VIRTUAL_ADDRESS const destination_acceleration_structure_device_memory_range_base = static_cast<brx_pal_d3d12_intermediate_bottom_level_acceleration_structure *>(wrapped_intermediate_bottom_level_acceleration_structure)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (destination_acceleration_structure_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    mcrt_vector<BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY> const &wrapped_bottom_level_acceleration_structure_geometries = static_cast<brx_pal_d3d12_intermediate_bottom_level_acceleration_structure *>(wrapped_intermediate_bottom_level_acceleration_structure)->get_bottom_level_acceleration_structure_geometries();
    uint32_t const bottom_level_acceleration_structure_geometry_count = static_cast<uint32_t>(wrapped_bottom_level_acceleration_structure_geometries.size());
    assert(NULL != wrapped_bottom_level_acceleration_structure_geometry_vertex_position_buffers);

    mcrt_vector<D3D12_RAYTRACING_GEOMETRY_DESC> ray_tracing_geometry_descs(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));
    for (uint32_t bottom_level_acceleration_structure_geometry_index = 0U; bottom_level_acceleration_structure_geometry_index < bottom_level_acceleration_structure_geometry_count; ++bottom_level_acceleration_structure_geometry_index)
    {
        BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const &wrapped_bottom_level_acceleration_structure_geometry = wrapped_bottom_level_acceleration_structure_geometries[bottom_level_acceleration_structure_geometry_index];

        // [Acceleration structure update constraints](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#acceleration-structure-update-constraints)
        assert(NULL == wrapped_bottom_level_acceleration_structure_geometry.vertex_position_buffer);
        ID3D12Resource *const unwrapped_vertex_position_buffer_resource = static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry_vertex_position_buffers[bottom_level_acceleration_structure_geometry_index])->get_resource();

        ID3D12Resource *const unwrapped_index_buffer_resource = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.index_buffer)->get_resource() : NULL;

        DXGI_FORMAT vertex_position_attribute_format;
        switch (wrapped_bottom_level_acceleration_structure_geometry.vertex_position_attribute_format)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_VERTEX_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT:
            vertex_position_attribute_format = DXGI_FORMAT_R32G32B32_FLOAT;
            break;
        default:
            // VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR
            assert(false);
            vertex_position_attribute_format = static_cast<DXGI_FORMAT>(-1);
            break;
        }

        D3D12_GPU_VIRTUAL_ADDRESS const vertex_position_buffer_device_memory_range_base = unwrapped_vertex_position_buffer_resource->GetGPUVirtualAddress();

        DXGI_FORMAT index_format;
        switch (wrapped_bottom_level_acceleration_structure_geometry.index_type)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT32:
            index_format = DXGI_FORMAT_R32_UINT;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT16:
            index_format = DXGI_FORMAT_R16_UINT;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE:
            index_format = DXGI_FORMAT_UNKNOWN;
            break;
        default:
            assert(false);
            index_format = static_cast<DXGI_FORMAT>(-1);
        }

        D3D12_GPU_VIRTUAL_ADDRESS const index_buffer_device_memory_range_base = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? unwrapped_index_buffer_resource->GetGPUVirtualAddress() : NULL;

        ray_tracing_geometry_descs[bottom_level_acceleration_structure_geometry_index] = D3D12_RAYTRACING_GEOMETRY_DESC{
            D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
            wrapped_bottom_level_acceleration_structure_geometry.force_closest_hit ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE,
            {.Triangles = {
                 0U,
                 index_format,
                 vertex_position_attribute_format,
                 (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? wrapped_bottom_level_acceleration_structure_geometry.index_count : 0U,
                 wrapped_bottom_level_acceleration_structure_geometry.vertex_count,
                 index_buffer_device_memory_range_base,
                 {vertex_position_buffer_device_memory_range_base, wrapped_bottom_level_acceleration_structure_geometry.vertex_position_binding_stride}

             }}};
    }

    assert(NULL != wrapped_scratch_buffer);
    D3D12_GPU_VIRTUAL_ADDRESS const scratch_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_scratch_buffer *>(wrapped_scratch_buffer)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (scratch_buffer_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    assert(bottom_level_acceleration_structure_geometry_count == ray_tracing_geometry_descs.size());
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC const ray_tracing_acceleration_structure_desc = {
        destination_acceleration_structure_device_memory_range_base,
        {D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
         static_cast<UINT>(ray_tracing_geometry_descs.size()),
         D3D12_ELEMENTS_LAYOUT_ARRAY,
         {.pGeometryDescs = &ray_tracing_geometry_descs[0]}},
        destination_acceleration_structure_device_memory_range_base,
        scratch_buffer_device_memory_range_base};

    this->m_command_list->BuildRaytracingAccelerationStructure(&ray_tracing_acceleration_structure_desc, 0U, NULL);
}

void brx_pal_d3d12_graphics_command_buffer::update_intermediate_bottom_level_acceleration_structure_store(uint32_t intermediate_bottom_level_acceleration_structure_count, brx_pal_intermediate_bottom_level_acceleration_structure const *const *wrapped_intermediate_bottom_level_acceleration_structures)
{
    mcrt_vector<D3D12_RESOURCE_BARRIER> store_barriers(static_cast<size_t>(intermediate_bottom_level_acceleration_structure_count));

    for (uint32_t intermediate_bottom_level_acceleration_structure_index = 0U; intermediate_bottom_level_acceleration_structure_index < intermediate_bottom_level_acceleration_structure_count; ++intermediate_bottom_level_acceleration_structure_index)
    {
        ID3D12Resource *const unwrapped_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_intermediate_bottom_level_acceleration_structure const *>(wrapped_intermediate_bottom_level_acceleration_structures[intermediate_bottom_level_acceleration_structure_index])->get_resource();

        // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#synchronizing-acceleration-structure-memory-writesreads
        store_barriers[intermediate_bottom_level_acceleration_structure_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .UAV = {
                unwrapped_acceleration_structure_buffer_resource}};
    }

    this->m_command_list->ResourceBarrier(static_cast<UINT>(store_barriers.size()), store_barriers.data());
}

void brx_pal_d3d12_graphics_command_buffer::build_top_level_acceleration_structure(brx_pal_top_level_acceleration_structure *wrapped_top_level_acceleration_structure, uint32_t top_level_acceleration_structure_instance_count, brx_pal_top_level_acceleration_structure_instance_upload_buffer *wrapped_top_level_acceleration_structure_instance_upload_buffer, brx_pal_scratch_buffer *wrapped_scratch_buffer)
{
    assert(NULL != wrapped_top_level_acceleration_structure);
    ID3D12Resource *const destination_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_top_level_acceleration_structure *>(wrapped_top_level_acceleration_structure)->get_resource();
    D3D12_GPU_VIRTUAL_ADDRESS const destination_acceleration_structure_device_memory_range_base = destination_acceleration_structure_buffer_resource->GetGPUVirtualAddress();
    assert(0U == (destination_acceleration_structure_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    assert(NULL != wrapped_top_level_acceleration_structure_instance_upload_buffer);
    D3D12_GPU_VIRTUAL_ADDRESS const top_level_acceleration_structure_instance_upload_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer *>(wrapped_top_level_acceleration_structure_instance_upload_buffer)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (top_level_acceleration_structure_instance_upload_buffer_device_memory_range_base % D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT));

    assert(NULL != wrapped_scratch_buffer);
    D3D12_GPU_VIRTUAL_ADDRESS const scratch_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_scratch_buffer *>(wrapped_scratch_buffer)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (scratch_buffer_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC const ray_tracing_acceleration_structure_desc = {
        destination_acceleration_structure_device_memory_range_base,
        {D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
         static_cast<UINT>(top_level_acceleration_structure_instance_count),
         D3D12_ELEMENTS_LAYOUT_ARRAY,
         {.InstanceDescs = top_level_acceleration_structure_instance_upload_buffer_device_memory_range_base}},
        0U,
        scratch_buffer_device_memory_range_base};

    this->m_command_list->BuildRaytracingAccelerationStructure(&ray_tracing_acceleration_structure_desc, 0U, NULL);

    static_cast<brx_pal_d3d12_top_level_acceleration_structure *>(wrapped_top_level_acceleration_structure)->set_instance_count(top_level_acceleration_structure_instance_count);
}

void brx_pal_d3d12_graphics_command_buffer::build_top_level_acceleration_structure_store(brx_pal_top_level_acceleration_structure *wrapped_top_level_acceleration_structure)
{
    assert(NULL != wrapped_top_level_acceleration_structure);
    ID3D12Resource *const unwrapped_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_top_level_acceleration_structure *>(wrapped_top_level_acceleration_structure)->get_resource();

    // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#synchronizing-acceleration-structure-memory-writesreads
    D3D12_RESOURCE_BARRIER const store_barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .UAV = {
            unwrapped_acceleration_structure_buffer_resource}};
    this->m_command_list->ResourceBarrier(1U, &store_barrier);
}

void brx_pal_d3d12_graphics_command_buffer::update_top_level_acceleration_structure(brx_pal_top_level_acceleration_structure *wrapped_top_level_acceleration_structure, brx_pal_top_level_acceleration_structure_instance_upload_buffer *wrapped_top_level_acceleration_structure_instance_upload_buffer, brx_pal_scratch_buffer *wrapped_scratch_buffer)
{
    assert(NULL != wrapped_top_level_acceleration_structure);
    ID3D12Resource *const destination_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_top_level_acceleration_structure *>(wrapped_top_level_acceleration_structure)->get_resource();
    D3D12_GPU_VIRTUAL_ADDRESS const destination_acceleration_structure_device_memory_range_base = destination_acceleration_structure_buffer_resource->GetGPUVirtualAddress();
    assert(0U == (destination_acceleration_structure_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    // [Acceleration structure update constraints](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#acceleration-structure-update-constraints)
    uint32_t const top_level_acceleration_structure_instance_count = static_cast<brx_pal_d3d12_top_level_acceleration_structure *>(wrapped_top_level_acceleration_structure)->get_instance_count();

    assert(NULL != wrapped_top_level_acceleration_structure_instance_upload_buffer);
    D3D12_GPU_VIRTUAL_ADDRESS const top_level_acceleration_structure_instance_upload_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer *>(wrapped_top_level_acceleration_structure_instance_upload_buffer)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (top_level_acceleration_structure_instance_upload_buffer_device_memory_range_base % D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT));

    assert(NULL != wrapped_scratch_buffer);
    D3D12_GPU_VIRTUAL_ADDRESS const scratch_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_scratch_buffer *>(wrapped_scratch_buffer)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (scratch_buffer_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC const ray_tracing_acceleration_structure_desc = {
        destination_acceleration_structure_device_memory_range_base,
        {D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
         static_cast<UINT>(top_level_acceleration_structure_instance_count),
         D3D12_ELEMENTS_LAYOUT_ARRAY,
         {.InstanceDescs = top_level_acceleration_structure_instance_upload_buffer_device_memory_range_base}},
        destination_acceleration_structure_device_memory_range_base,
        scratch_buffer_device_memory_range_base};

    this->m_command_list->BuildRaytracingAccelerationStructure(&ray_tracing_acceleration_structure_desc, 0U, NULL);
}

void brx_pal_d3d12_graphics_command_buffer::update_top_level_acceleration_structure_store(brx_pal_top_level_acceleration_structure *wrapped_top_level_acceleration_structure)
{
    assert(NULL != wrapped_top_level_acceleration_structure);
    ID3D12Resource *const destination_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_top_level_acceleration_structure *>(wrapped_top_level_acceleration_structure)->get_resource();

    // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#synchronizing-acceleration-structure-memory-writesreads
    D3D12_RESOURCE_BARRIER const store_barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .UAV = {
            destination_acceleration_structure_buffer_resource}};
    this->m_command_list->ResourceBarrier(1U, &store_barrier);
}

void brx_pal_d3d12_graphics_command_buffer::end()
{
    HRESULT const hr_close = this->m_command_list->Close();
    assert(SUCCEEDED(hr_close));
}

brx_pal_d3d12_upload_command_buffer::brx_pal_d3d12_upload_command_buffer() : m_command_allocator(NULL), m_command_list(NULL), m_upload_queue_submit_fence(NULL)
{
}

void brx_pal_d3d12_upload_command_buffer::init(ID3D12Device *device, bool uma, bool support_ray_tracing)
{
    this->m_uma = uma;

    this->m_support_ray_tracing = support_ray_tracing;

    if ((!this->m_uma) || this->m_support_ray_tracing)
    {
        assert(NULL == this->m_command_allocator);
        HRESULT const hr_create_command_allocator = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&this->m_command_allocator));
        assert(SUCCEEDED(hr_create_command_allocator));

        assert(NULL == this->m_command_list);
        HRESULT const hr_create_command_list = device->CreateCommandList(0U, D3D12_COMMAND_LIST_TYPE_COMPUTE, this->m_command_allocator, NULL, IID_PPV_ARGS(&this->m_command_list));
        assert(SUCCEEDED(hr_create_command_list));

        HRESULT const hr_close = this->m_command_list->Close();
        assert(SUCCEEDED(hr_close));

        assert(NULL == this->m_upload_queue_submit_fence);
        HRESULT const hr_create_fence = device->CreateFence(0U, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_upload_queue_submit_fence));
        assert(SUCCEEDED(hr_create_fence));
    }
    else
    {
        assert(NULL == this->m_command_allocator);
        assert(NULL == this->m_command_list);
        assert(NULL == this->m_upload_queue_submit_fence);
    }
}

void brx_pal_d3d12_upload_command_buffer::uninit()
{
    if ((!this->m_uma) || this->m_support_ray_tracing)
    {
        assert(NULL != this->m_upload_queue_submit_fence);
        this->m_upload_queue_submit_fence->Release();
        this->m_upload_queue_submit_fence = NULL;

        assert(NULL != this->m_command_list);
        this->m_command_list->Release();
        this->m_command_list = NULL;

        assert(NULL != this->m_command_allocator);
        this->m_command_allocator->Release();
        this->m_command_allocator = NULL;
    }
    else
    {
        assert(NULL == this->m_command_allocator);
        assert(NULL == this->m_command_list);
        assert(NULL == this->m_upload_queue_submit_fence);
    }
}

brx_pal_d3d12_upload_command_buffer::~brx_pal_d3d12_upload_command_buffer()
{
    assert(NULL == this->m_command_allocator);
    assert(NULL == this->m_command_list);
    assert(NULL == this->m_upload_queue_submit_fence);
}

ID3D12CommandAllocator *brx_pal_d3d12_upload_command_buffer::get_command_allocator() const
{
    return this->m_command_allocator;
}

ID3D12GraphicsCommandList4 *brx_pal_d3d12_upload_command_buffer::get_command_list() const
{
    return this->m_command_list;
}

ID3D12Fence *brx_pal_d3d12_upload_command_buffer::get_upload_queue_submit_fence() const
{
    return this->m_upload_queue_submit_fence;
}

void brx_pal_d3d12_upload_command_buffer::begin()
{
    if ((!this->m_uma) || this->m_support_ray_tracing)
    {
        HRESULT hr_reset = this->m_command_list->Reset(this->m_command_allocator, NULL);
        assert(SUCCEEDED(hr_reset));
    }
    else
    {
        assert(NULL == this->m_command_allocator);
        assert(NULL == this->m_command_list);
    }
}

void brx_pal_d3d12_upload_command_buffer::upload_from_staging_upload_buffer_to_storage_asset_buffer(brx_pal_storage_asset_buffer *wrapped_storage_asset_buffer, uint64_t dst_offset, brx_pal_staging_upload_buffer *wrapped_staging_upload_buffer, uint64_t src_offset, uint32_t src_size)
{
    ID3D12Resource *asset_buffer_resource = static_cast<brx_pal_d3d12_storage_asset_buffer *>(wrapped_storage_asset_buffer)->get_resource();
    ID3D12Resource *staging_upload_buffer_resource = static_cast<brx_pal_d3d12_staging_upload_buffer *>(wrapped_staging_upload_buffer)->get_resource();

    if (!this->m_uma)
    {
        this->m_command_list->CopyBufferRegion(asset_buffer_resource, dst_offset, staging_upload_buffer_resource, src_offset, src_size);
    }
    else
    {
        assert(this->m_support_ray_tracing || NULL == this->m_command_allocator);
        assert(this->m_support_ray_tracing || NULL == this->m_command_list);

        void *asset_buffer_memory_range_base = NULL;
        {
            D3D12_RANGE const read_range = {0U, 0U};
            HRESULT const hr_map = asset_buffer_resource->Map(0U, &read_range, &asset_buffer_memory_range_base);
            assert(SUCCEEDED(hr_map));
        }

        void *staging_upload_buffer_memory_range_base = static_cast<brx_pal_d3d12_staging_upload_buffer *>(wrapped_staging_upload_buffer)->get_host_memory_range_base();

        std::memcpy(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(asset_buffer_memory_range_base) + dst_offset), reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(staging_upload_buffer_memory_range_base) + src_offset), src_size);

        {
            D3D12_RANGE const written_range = {static_cast<SIZE_T>(dst_offset), static_cast<SIZE_T>(src_size)};
            asset_buffer_resource->Unmap(0U, &written_range);
        }
    }
}

void brx_pal_d3d12_upload_command_buffer::upload_from_staging_upload_buffer_to_sampled_asset_image(brx_pal_sampled_asset_image *wrapped_sampled_asset_image, BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT wrapped_sampled_asset_image_format, uint32_t sampled_asset_image_width, uint32_t sampled_asset_image_height, uint32_t dst_mip_level, brx_pal_staging_upload_buffer *wrapped_staging_upload_buffer, uint64_t src_offset, uint32_t src_row_pitch, uint32_t src_row_count)
{
    assert(NULL != wrapped_sampled_asset_image);
    ID3D12Resource *const sampled_asset_image = static_cast<brx_pal_d3d12_sampled_asset_image *>(wrapped_sampled_asset_image)->get_resource();

    assert(NULL != wrapped_staging_upload_buffer);
    ID3D12Resource *const staging_upload_buffer = static_cast<brx_pal_d3d12_staging_upload_buffer *>(wrapped_staging_upload_buffer)->get_resource();

    DXGI_FORMAT unwrapped_sampled_asset_image_format;
    switch (wrapped_sampled_asset_image_format)
    {
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM:
        unwrapped_sampled_asset_image_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_SRGB:
        unwrapped_sampled_asset_image_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
        unwrapped_sampled_asset_image_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_UNORM_BLOCK:
        unwrapped_sampled_asset_image_format = DXGI_FORMAT_BC7_UNORM;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_SRGB_BLOCK:
        unwrapped_sampled_asset_image_format = DXGI_FORMAT_BC7_UNORM_SRGB;
        break;
    default:
        assert(false);
        unwrapped_sampled_asset_image_format = DXGI_FORMAT_UNKNOWN;
    }

    uint32_t const block_width = brx_pal_sampled_asset_image_format_get_block_width(wrapped_sampled_asset_image_format);
    uint32_t const block_height = brx_pal_sampled_asset_image_format_get_block_height(wrapped_sampled_asset_image_format);

    uint32_t image_width = (sampled_asset_image_width >> dst_mip_level);
    uint32_t image_height = (sampled_asset_image_height >> dst_mip_level);
    if (0U == image_width)
    {
        image_width = 1U;
    }
    if (0U == image_height)
    {
        image_height = 1U;
    }

    uint32_t const width = ((image_width + (block_width - 1U)) / block_width) * block_width;
    uint32_t const height = ((image_height + (block_height - 1U)) / block_height) * block_height;

#ifndef NDEBUG
    {
        ID3D12Device *device = NULL;
        HRESULT const hr_get_device = sampled_asset_image->GetDevice(IID_PPV_ARGS(&device));
        assert(SUCCEEDED(hr_get_device));

        D3D12_RESOURCE_DESC const sampled_asset_image_resource_desc = sampled_asset_image->GetDesc();

        // The resulting structures are GPU adapter-agnostic, meaning that the values will not vary from one GPU adapter to the next.
        // This means that we can implement this function by ourselves according to the specification.
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[1];
        UINT num_rows[1];
        device->GetCopyableFootprints(&sampled_asset_image_resource_desc, dst_mip_level, 1U, src_offset, layouts, num_rows, NULL, NULL);

        device->Release();

        assert(layouts[0].Offset == src_offset);
        assert(layouts[0].Footprint.Format == unwrapped_sampled_asset_image_format);
        assert(layouts[0].Footprint.Width == width);
        assert(layouts[0].Footprint.Height == height);
        assert(layouts[0].Footprint.Depth == 1U);
        assert(layouts[0].Footprint.RowPitch == src_row_pitch);
        assert(num_rows[0] == src_row_count);
    }
#endif

    if (!this->m_uma)
    {
        {
            D3D12_TEXTURE_COPY_LOCATION const destination = {
                .pResource = sampled_asset_image,
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = dst_mip_level};

            D3D12_TEXTURE_COPY_LOCATION const source = {
                .pResource = staging_upload_buffer,
                .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                .PlacedFootprint = {
                    src_offset,
                    {unwrapped_sampled_asset_image_format,
                     static_cast<UINT>(width),
                     height,
                     1U,
                     src_row_pitch}}};

            this->m_command_list->CopyTextureRegion(&destination, 0U, 0U, 0U, &source, NULL);
        }
    }
    else
    {
        assert(this->m_support_ray_tracing || NULL == this->m_command_allocator);
        assert(this->m_support_ray_tracing || NULL == this->m_command_list);

        {
            D3D12_RANGE const read_range = {0U, 0U};
            HRESULT const hr_map = sampled_asset_image->Map(0U, &read_range, NULL);
            assert(SUCCEEDED(hr_map));
        }

        void *staging_upload_buffer_memory_range_base = static_cast<brx_pal_d3d12_staging_upload_buffer *>(wrapped_staging_upload_buffer)->get_host_memory_range_base();

        // the tiling mode is vendor specific
        // for example,  the AMD addrlib [ac_surface_addr_from_coord](https://gitlab.freedesktop.org/mesa/mesa/-/blob/22.3/src/amd/vulkan/radv_meta_bufimage.c#L1372)
        sampled_asset_image->WriteToSubresource(dst_mip_level, NULL, reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(staging_upload_buffer_memory_range_base) + src_offset), src_row_pitch, src_row_pitch * src_row_count);

        sampled_asset_image->Unmap(0U, NULL);
    }
}

void brx_pal_d3d12_upload_command_buffer::build_non_compacted_bottom_level_acceleration_structure_pass_load(uint32_t acceleration_structure_build_input_read_only_buffer_count, brx_pal_acceleration_structure_build_input_read_only_buffer const *const *wrapped_acceleration_structure_build_input_read_only_buffers)
{
    if (!this->m_uma)
    {
        mcrt_vector<D3D12_RESOURCE_BARRIER> load_barriers(static_cast<size_t>(acceleration_structure_build_input_read_only_buffer_count));

        for (uint32_t acceleration_structure_build_input_read_only_buffer_index = 0U; acceleration_structure_build_input_read_only_buffer_index < acceleration_structure_build_input_read_only_buffer_count; ++acceleration_structure_build_input_read_only_buffer_index)
        {
            ID3D12Resource *const unwrapped_acceleration_structure_build_input_read_only_buffer_resource = static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_acceleration_structure_build_input_read_only_buffers[acceleration_structure_build_input_read_only_buffer_index])->get_resource();

            load_barriers[acceleration_structure_build_input_read_only_buffer_index] = D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .Transition = {
                    unwrapped_acceleration_structure_build_input_read_only_buffer_resource,
                    0U,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE}};
        }

        this->m_command_list->ResourceBarrier(static_cast<UINT>(load_barriers.size()), load_barriers.data());
    }
}

void brx_pal_d3d12_upload_command_buffer::build_non_compacted_bottom_level_acceleration_structure(brx_pal_non_compacted_bottom_level_acceleration_structure *wrapped_non_compacted_bottom_level_acceleration_structure, uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *wrapped_bottom_level_acceleration_structure_geometries, brx_pal_scratch_buffer *wrapped_scratch_buffer, brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool *wrapped_compacted_bottom_level_acceleration_structure_size_query_pool, uint32_t query_index)
{
    assert(NULL != wrapped_non_compacted_bottom_level_acceleration_structure);
    D3D12_GPU_VIRTUAL_ADDRESS const destination_acceleration_structure_device_memory_range_base = static_cast<brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure *>(wrapped_non_compacted_bottom_level_acceleration_structure)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (destination_acceleration_structure_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    assert(NULL != wrapped_bottom_level_acceleration_structure_geometries);

    mcrt_vector<D3D12_RAYTRACING_GEOMETRY_DESC> ray_tracing_geometry_descs(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));

    for (uint32_t bottom_level_acceleration_structure_geometry_index = 0U; bottom_level_acceleration_structure_geometry_index < bottom_level_acceleration_structure_geometry_count; ++bottom_level_acceleration_structure_geometry_index)
    {
        BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const &wrapped_bottom_level_acceleration_structure_geometry = wrapped_bottom_level_acceleration_structure_geometries[bottom_level_acceleration_structure_geometry_index];

        ID3D12Resource *const unwrapped_vertex_position_buffer_resource = static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.vertex_position_buffer)->get_resource();

        ID3D12Resource *const unwrapped_index_buffer_resource = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.index_buffer)->get_resource() : NULL;

        DXGI_FORMAT vertex_position_attribute_format;
        switch (wrapped_bottom_level_acceleration_structure_geometry.vertex_position_attribute_format)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_VERTEX_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT:
            vertex_position_attribute_format = DXGI_FORMAT_R32G32B32_FLOAT;
            break;
        default:
            // VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR
            assert(false);
            vertex_position_attribute_format = static_cast<DXGI_FORMAT>(-1);
            break;
        }

        D3D12_GPU_VIRTUAL_ADDRESS const vertex_position_buffer_device_memory_range_base = unwrapped_vertex_position_buffer_resource->GetGPUVirtualAddress();

        DXGI_FORMAT index_format;
        switch (wrapped_bottom_level_acceleration_structure_geometry.index_type)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT32:
            index_format = DXGI_FORMAT_R32_UINT;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT16:
            index_format = DXGI_FORMAT_R16_UINT;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE:
            index_format = DXGI_FORMAT_UNKNOWN;
            break;
        default:
            assert(false);
            index_format = static_cast<DXGI_FORMAT>(-1);
        }

        D3D12_GPU_VIRTUAL_ADDRESS const index_buffer_device_memory_range_base = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? unwrapped_index_buffer_resource->GetGPUVirtualAddress() : NULL;

        ray_tracing_geometry_descs[bottom_level_acceleration_structure_geometry_index] = D3D12_RAYTRACING_GEOMETRY_DESC{
            D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES,
            wrapped_bottom_level_acceleration_structure_geometry.force_closest_hit ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE,
            {.Triangles = {
                 0U,
                 index_format,
                 vertex_position_attribute_format,
                 (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? wrapped_bottom_level_acceleration_structure_geometry.index_count : 0U,
                 wrapped_bottom_level_acceleration_structure_geometry.vertex_count,
                 index_buffer_device_memory_range_base,
                 {vertex_position_buffer_device_memory_range_base, wrapped_bottom_level_acceleration_structure_geometry.vertex_position_binding_stride}

             }}};
    }

    assert(NULL != wrapped_scratch_buffer);
    D3D12_GPU_VIRTUAL_ADDRESS const scratch_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_scratch_buffer *>(wrapped_scratch_buffer)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (scratch_buffer_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    assert(NULL != wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);
    D3D12_GPU_VIRTUAL_ADDRESS const query_pool_device_memory_range_base = static_cast<brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool *>(wrapped_compacted_bottom_level_acceleration_structure_size_query_pool)->get_resource()->GetGPUVirtualAddress();

    assert(bottom_level_acceleration_structure_geometry_count == ray_tracing_geometry_descs.size());
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC const ray_tracing_acceleration_structure_desc = {
        destination_acceleration_structure_device_memory_range_base,
        {D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
         D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
         static_cast<UINT>(ray_tracing_geometry_descs.size()),
         D3D12_ELEMENTS_LAYOUT_ARRAY,
         {.pGeometryDescs = &ray_tracing_geometry_descs[0]}},
        0U,
        scratch_buffer_device_memory_range_base};

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC const ray_tracing_acceleration_structure_postbuild_info_desc = {
        query_pool_device_memory_range_base + sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC) * query_index,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE,
    };

    this->m_command_list->BuildRaytracingAccelerationStructure(&ray_tracing_acceleration_structure_desc, 1U, &ray_tracing_acceleration_structure_postbuild_info_desc);
}

void brx_pal_d3d12_upload_command_buffer::build_non_compacted_bottom_level_acceleration_structure_pass_store(uint32_t acceleration_structure_build_input_read_only_buffer_count, brx_pal_acceleration_structure_build_input_read_only_buffer const *const *wrapped_acceleration_structure_build_input_read_only_buffers)
{
    if (!this->m_uma)
    {
        mcrt_vector<D3D12_RESOURCE_BARRIER> store_barriers(static_cast<size_t>(acceleration_structure_build_input_read_only_buffer_count));

        for (uint32_t acceleration_structure_build_input_read_only_buffer_index = 0U; acceleration_structure_build_input_read_only_buffer_index < acceleration_structure_build_input_read_only_buffer_count; ++acceleration_structure_build_input_read_only_buffer_index)
        {
            ID3D12Resource *const unwrapped_acceleration_structure_build_input_read_only_buffer_resource = static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_acceleration_structure_build_input_read_only_buffers[acceleration_structure_build_input_read_only_buffer_index])->get_resource();

            store_barriers[acceleration_structure_build_input_read_only_buffer_index] = D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .Transition = {
                    unwrapped_acceleration_structure_build_input_read_only_buffer_resource,
                    0U,
                    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_COPY_DEST}};
        }

        this->m_command_list->ResourceBarrier(static_cast<UINT>(store_barriers.size()), store_barriers.data());
    }
}

void brx_pal_d3d12_upload_command_buffer::compact_bottom_level_acceleration_structure(brx_pal_compacted_bottom_level_acceleration_structure *wrapped_destination_compacted_bottom_level_acceleration_structure, brx_pal_non_compacted_bottom_level_acceleration_structure *wrapped_source_non_compacted_bottom_level_acceleration_structure)
{
    // NOTE: we do NOT need the barrier to synchronize the staging non compacted bottom level acceleration structure, since we already use the fence to wait for the GPU completion to have the size of the compacted acceleration structure.

    assert(NULL != wrapped_destination_compacted_bottom_level_acceleration_structure);
    assert(NULL != wrapped_source_non_compacted_bottom_level_acceleration_structure);
    ID3D12Resource *const destination_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_compacted_bottom_level_acceleration_structure *>(wrapped_destination_compacted_bottom_level_acceleration_structure)->get_resource();
    D3D12_GPU_VIRTUAL_ADDRESS const destination_acceleration_structure_device_memory_range_base = destination_acceleration_structure_buffer_resource->GetGPUVirtualAddress();
    assert(0U == (destination_acceleration_structure_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));
    D3D12_GPU_VIRTUAL_ADDRESS const source_acceleration_structure_device_memory_range_base = static_cast<brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure *>(wrapped_source_non_compacted_bottom_level_acceleration_structure)->get_resource()->GetGPUVirtualAddress();
    assert(0U == (source_acceleration_structure_device_memory_range_base % D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT));

    this->m_command_list->CopyRaytracingAccelerationStructure(destination_acceleration_structure_device_memory_range_base, source_acceleration_structure_device_memory_range_base, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);
}

void brx_pal_d3d12_upload_command_buffer::release(uint32_t storage_asset_buffer_count, brx_pal_storage_asset_buffer const *const *wrapped_storage_asset_buffers, uint32_t sampled_asset_image_subresource_count, BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE const *wrapped_sampled_asset_image_subresources, uint32_t compacted_bottom_level_acceleration_structure_count, brx_pal_compacted_bottom_level_acceleration_structure const *const *wrapped_compacted_bottom_level_acceleration_structures)
{
    mcrt_vector<D3D12_RESOURCE_BARRIER> release_barriers(static_cast<size_t>((!this->m_uma) ? (storage_asset_buffer_count + sampled_asset_image_subresource_count + compacted_bottom_level_acceleration_structure_count) : compacted_bottom_level_acceleration_structure_count));

    if (!this->m_uma)
    {
        for (uint32_t storage_asset_buffer_index = 0U; storage_asset_buffer_index < storage_asset_buffer_count; ++storage_asset_buffer_index)
        {
            ID3D12Resource *const asset_buffer_resource = static_cast<brx_pal_d3d12_storage_asset_buffer const *>(wrapped_storage_asset_buffers[storage_asset_buffer_index])->get_resource();

            release_barriers[storage_asset_buffer_index] = D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .Transition = {
                    asset_buffer_resource,
                    0U,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    D3D12_RESOURCE_STATE_COMMON}};
        }

        for (uint32_t sampled_asset_image_index = 0U; sampled_asset_image_index < sampled_asset_image_subresource_count; ++sampled_asset_image_index)
        {
            ID3D12Resource *const sampled_asset_image = static_cast<brx_pal_d3d12_sampled_asset_image const *>(wrapped_sampled_asset_image_subresources[sampled_asset_image_index].m_sampled_asset_images)->get_resource();

            release_barriers[storage_asset_buffer_count + sampled_asset_image_index] = D3D12_RESOURCE_BARRIER{
                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                .Transition = {
                    sampled_asset_image,
                    wrapped_sampled_asset_image_subresources[sampled_asset_image_index].m_mip_level,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    D3D12_RESOURCE_STATE_COMMON}};
        }
    }

    for (uint32_t compacted_bottom_level_acceleration_structure_index = 0U; compacted_bottom_level_acceleration_structure_index < compacted_bottom_level_acceleration_structure_count; ++compacted_bottom_level_acceleration_structure_index)
    {
        ID3D12Resource *const unwrapped_compacted_bottom_level_acceleration_structure_buffer_resource = static_cast<brx_pal_d3d12_compacted_bottom_level_acceleration_structure const *>(wrapped_compacted_bottom_level_acceleration_structures[compacted_bottom_level_acceleration_structure_index])->get_resource();

        // https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#synchronizing-acceleration-structure-memory-writesreads
        release_barriers[(!this->m_uma) ? (storage_asset_buffer_count + sampled_asset_image_subresource_count + compacted_bottom_level_acceleration_structure_index) : compacted_bottom_level_acceleration_structure_index] = D3D12_RESOURCE_BARRIER{
            .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
            .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
            .UAV = {
                unwrapped_compacted_bottom_level_acceleration_structure_buffer_resource}};
    }

    if (release_barriers.size() > 0U)
    {
        this->m_command_list->ResourceBarrier(static_cast<UINT>(release_barriers.size()), release_barriers.data());
    }
}

void brx_pal_d3d12_upload_command_buffer::end()
{
    if ((!this->m_uma) || this->m_support_ray_tracing)
    {
        HRESULT hr_close = this->m_command_list->Close();
        assert(SUCCEEDED(hr_close));
    }
    else
    {
        assert(NULL == this->m_command_allocator);
        assert(NULL == this->m_command_list);
    }
}
