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

#ifndef _BRX_PAL_D3D12_DEVICE_H_
#define _BRX_PAL_D3D12_DEVICE_H_ 1

#include "../include/brx_pal_device.h"
#include "../../McRT-Malloc/include/mcrt_vector.h"
#include "../../McRT-Malloc/include/mcrt_unordered_set.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <sdkddkver.h>
#include <windows.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED 1
#include "../thirdparty/D3D12MemoryAllocator/include/D3D12MemAlloc.h"
#include "brx_pal_d3d12_descriptor_allocator.h"

class brx_pal_d3d12_color_attachment_image;
class brx_pal_d3d12_depth_stencil_attachment_image;

class brx_pal_d3d12_device final : public brx_pal_device
{
    decltype(D3D12SerializeRootSignature) *m_pfn_d3d12_serialize_root_signature;

    bool m_support_ray_tracing;

    IDXGIFactory2 *m_factory;
    IDXGIAdapter *m_adapter;

    ID3D12Device5 *m_device;

    bool m_uma;
    bool m_cache_coherent_uma;

    ID3D12CommandQueue *m_graphics_queue;
    ID3D12CommandQueue *m_upload_queue;

    D3D12MA::Allocator *m_memory_allocator;
    D3D12MA::Pool *m_uniform_upload_buffer_memory_pool;
    D3D12MA::Pool *m_staging_upload_buffer_memory_pool;
    D3D12MA::Pool *m_storage_intermediate_buffer_memory_pool;
    D3D12MA::Pool *m_storage_asset_buffer_memory_pool;
    D3D12MA::Pool *m_color_attachment_intermediate_image_memory_pool;
    D3D12MA::Pool *m_depth_stencil_attachment_intermediate_image_memory_pool;
    D3D12MA::Pool *m_storage_intermediate_image_memory_pool;
    D3D12MA::Pool *m_sampled_asset_image_memory_pool;
    D3D12MA::Pool *m_scratch_buffer_memory_pool;
    D3D12MA::Pool *m_intermediate_bottom_level_acceleration_structure_memory_pool;
    D3D12MA::Pool *m_non_compacted_bottom_level_acceleration_structure_memory_pool;
    D3D12MA::Pool *m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool;
    D3D12MA::Pool *m_compacted_bottom_level_acceleration_structure_memory_pool;
    D3D12MA::Pool *m_top_level_acceleration_structure_instance_upload_buffer_memory_pool;
    D3D12MA::Pool *m_top_level_acceleration_structure_memory_pool;

    brx_pal_d3d12_descriptor_allocator m_descriptor_allocator;

public:
    brx_pal_d3d12_device();
    void init(bool support_ray_tracing);
    void uninit();
    ~brx_pal_d3d12_device();

private:
    BRX_PAL_BACKEND_NAME get_backend_name() const override;
    bool is_ray_tracing_supported() const override;
    brx_pal_graphics_queue *create_graphics_queue() const override;
    void destroy_graphics_queue(brx_pal_graphics_queue *graphics_queue) const override;
    brx_pal_upload_queue *create_upload_queue() const override;
    void destroy_upload_queue(brx_pal_upload_queue *upload_queue) const override;
    brx_pal_graphics_command_buffer *create_graphics_command_buffer() const override;
    void reset_graphics_command_buffer(brx_pal_graphics_command_buffer *graphics_command_buffer) const override;
    void destroy_graphics_command_buffer(brx_pal_graphics_command_buffer *graphics_command_buffer) const override;
    brx_pal_upload_command_buffer *create_upload_command_buffer() const override;
    void reset_upload_command_buffer(brx_pal_upload_command_buffer *upload_command_buffer) const override;
    void destroy_upload_command_buffer(brx_pal_upload_command_buffer *upload_command_buffer) const override;
    brx_pal_fence *create_fence(bool signaled) const override;
    void wait_for_fence(brx_pal_fence *fence) const override;
    void reset_fence(brx_pal_fence *fence) const override;
    void destroy_fence(brx_pal_fence *fence) const override;
    brx_pal_descriptor_set_layout *create_descriptor_set_layout(uint32_t descriptor_set_binding_count, BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const *descriptor_set_bindings) const override;
    void destroy_descriptor_set_layout(brx_pal_descriptor_set_layout *descriptor_set_layout) const override;
    brx_pal_pipeline_layout *create_pipeline_layout(uint32_t descriptor_set_layout_count, brx_pal_descriptor_set_layout const *const *descriptor_set_layouts) const override;
    void destroy_pipeline_layout(brx_pal_pipeline_layout *pipeline_layout) const override;
    brx_pal_descriptor_set *create_descriptor_set(brx_pal_descriptor_set_layout const *descriptor_set_layout, uint32_t unbounded_descriptor_count) override;
    void write_descriptor_set(brx_pal_descriptor_set *descriptor_set, uint32_t dst_binding, BRX_PAL_DESCRIPTOR_TYPE descriptor_type, uint32_t dst_descriptor_start_index, uint32_t src_descriptor_count, brx_pal_uniform_upload_buffer const *const *src_dynamic_uniform_buffers, uint32_t const *src_dynamic_uniform_buffer_ranges, brx_pal_read_only_storage_buffer const *const *src_read_only_storage_buffers, brx_pal_storage_buffer const *const *src_storage_buffers, brx_pal_sampled_image const *const *src_sampled_images, brx_pal_storage_image const *const *src_storage_images, brx_pal_sampler const *const *src_samplers, brx_pal_top_level_acceleration_structure const *const *src_top_level_acceleration_structures) override;
    void destroy_descriptor_set(brx_pal_descriptor_set *descriptor_set) override;
    brx_pal_render_pass *create_render_pass(uint32_t color_attachment_count, BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const *color_attachments, BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT const *depth_stencil_attachment) const override;
    void destroy_render_pass(brx_pal_render_pass *render_pass) const override;
    brx_pal_graphics_pipeline *create_graphics_pipeline(brx_pal_render_pass const *render_pass, brx_pal_pipeline_layout const *pipeline_layout, size_t vertex_shader_module_code_size, void const *vertex_shader_module_code, size_t fragment_shader_module_code_size, void const *fragment_shader_module_code, bool enable_back_face_cull, bool front_ccw, bool enable_depth_clip, uint32_t forced_sample_count, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION depth_compare_operation, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION blend_operation) const override;
    void destroy_graphics_pipeline(brx_pal_graphics_pipeline *graphics_pipeline) const override;
    brx_pal_compute_pipeline *create_compute_pipeline(brx_pal_pipeline_layout const *pipeline_layout, size_t compute_shader_module_code_size, void const *compute_shader_module_code) const override;
    void destroy_compute_pipeline(brx_pal_compute_pipeline *compute_pipeline) const override;
    brx_pal_frame_buffer *create_frame_buffer(brx_pal_render_pass const *render_pass, uint32_t width, uint32_t height, uint32_t color_attachment_count, brx_pal_color_attachment_image const *const *color_attachments, brx_pal_depth_stencil_attachment_image const *depth_stencil_attachment) const override;
    void destroy_frame_buffer(brx_pal_frame_buffer *frame_buffer) const override;
    uint32_t get_uniform_upload_buffer_offset_alignment() const override;
    brx_pal_uniform_upload_buffer *create_uniform_upload_buffer(uint32_t size) const override;
    void destroy_uniform_upload_buffer(brx_pal_uniform_upload_buffer *uniform_upload_buffer) const override;
    uint32_t get_staging_upload_buffer_offset_alignment() const override;
    uint32_t get_staging_upload_buffer_row_pitch_alignment() const override;
    brx_pal_staging_upload_buffer *create_staging_upload_buffer(uint32_t size) const override;
    void destroy_staging_upload_buffer(brx_pal_staging_upload_buffer *staging_upload_buffer) const override;
    brx_pal_storage_intermediate_buffer *create_storage_intermediate_buffer(uint32_t size) const override;
    void destroy_storage_intermediate_buffer(brx_pal_storage_intermediate_buffer *storage_intermediate_buffer) const override;
    brx_pal_storage_asset_buffer *create_storage_asset_buffer(uint32_t size) const override;
    void destroy_storage_asset_buffer(brx_pal_storage_asset_buffer *asset_storage_buffer) const override;
    brx_pal_color_attachment_image *create_color_attachment_image(BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT color_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image) const override;
    void destroy_color_attachment_image(brx_pal_color_attachment_image *color_attachment_image) const override;
    BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT get_depth_attachment_image_format() const override;
    BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT get_depth_stencil_attachment_image_format() const override;
    brx_pal_depth_stencil_attachment_image *create_depth_stencil_attachment_image(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT depth_stencil_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image) const override;
    void destroy_depth_stencil_attachment_image(brx_pal_depth_stencil_attachment_image *depth_stencil_attachment_image) const override;
    brx_pal_storage_image *create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT storage_image_format, uint32_t width, uint32_t height, bool volume, uint32_t depth, bool allow_sampled_image) const override;
    void destroy_storage_image(brx_pal_storage_image *storage_image) const override;
    bool is_sampled_asset_image_compression_bc_supported() const override;
    bool is_sampled_asset_image_compression_astc_supported() const override;
    brx_pal_sampled_asset_image *create_sampled_asset_image(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT sampled_asset_image_format, uint32_t width, uint32_t height, bool array, uint32_t array_layers, uint32_t mip_levels) const override;
    void destroy_sampled_asset_image(brx_pal_sampled_asset_image *sampled_asset_image) const override;
    brx_pal_sampler *create_sampler(BRX_PAL_SAMPLER_FILTER filter, BRX_PAL_SAMPLER_ADDRESS_MODE address_mode) const override;
    void destroy_sampler(brx_pal_sampler *sampler) const override;
    brx_pal_surface *create_surface(void *wsi_window) const override;
    void destroy_surface(brx_pal_surface *surface) const override;
    brx_pal_swap_chain *create_swap_chain(brx_pal_surface *surface) const override;
    bool acquire_next_image(brx_pal_graphics_command_buffer *graphics_command_buffer, brx_pal_swap_chain const *swap_chain, uint32_t *out_swap_chain_image_index) const override;
    void destroy_swap_chain(brx_pal_swap_chain *swap_chain) const override;
    brx_pal_scratch_buffer *create_scratch_buffer(uint32_t size) const override;
    void destroy_scratch_buffer(brx_pal_scratch_buffer *scratch_buffer) const override;
    void get_intermediate_bottom_level_acceleration_structure_size(uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *bottom_level_acceleration_structure_geometries, uint32_t *intermediate_bottom_level_acceleration_structure_size, uint32_t *build_scratch_size, uint32_t *update_scratch_size) const override;
    brx_pal_intermediate_bottom_level_acceleration_structure *create_intermediate_bottom_level_acceleration_structure(uint32_t size) const override;
    void destroy_intermediate_bottom_level_acceleration_structure(brx_pal_intermediate_bottom_level_acceleration_structure *intermediate_bottom_level_acceleration_structure) const override;
    void get_non_compacted_bottom_level_acceleration_structure_size(uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *bottom_level_acceleration_structure_geometries, uint32_t *non_compacted_bottom_level_acceleration_structure_size, uint32_t *build_scratch_size) const override;
    brx_pal_non_compacted_bottom_level_acceleration_structure *create_non_compacted_bottom_level_acceleration_structure(uint32_t size) const override;
    void destroy_non_compacted_bottom_level_acceleration_structure(brx_pal_non_compacted_bottom_level_acceleration_structure *non_compacted_bottom_level_acceleration_structure) const override;
    brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool *create_compacted_bottom_level_acceleration_structure_size_query_pool(uint32_t query_count) const override;
    uint32_t get_compacted_bottom_level_acceleration_structure_size_query_pool_result(brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool const *compacted_bottom_level_acceleration_structure_size_query_pool, uint32_t query_index) const override;
    void destroy_compacted_bottom_level_acceleration_structure_size_query_pool(brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool *compacted_bottom_level_acceleration_structure_size_query_pool) const override;
    brx_pal_compacted_bottom_level_acceleration_structure *create_compacted_bottom_level_acceleration_structure(uint32_t size) const override;
    void destroy_compacted_bottom_level_acceleration_structure(brx_pal_compacted_bottom_level_acceleration_structure *compacted_bottom_level_acceleration_structure) const override;
    brx_pal_top_level_acceleration_structure_instance_upload_buffer *create_top_level_acceleration_structure_instance_upload_buffer(uint32_t instance_count) const override;
    void destroy_top_level_acceleration_structure_instance_upload_buffer(brx_pal_top_level_acceleration_structure_instance_upload_buffer *top_level_acceleration_structure_instance_upload_buffer) const override;
    void get_top_level_acceleration_structure_size(uint32_t top_level_acceleration_structure_instance_count, uint32_t *top_level_acceleration_structure_size, uint32_t *build_scratch_size, uint32_t *update_scratch_size) const override;
    brx_pal_top_level_acceleration_structure *create_top_level_acceleration_structure(uint32_t size) const override;
    void destroy_top_level_acceleration_structure(brx_pal_top_level_acceleration_structure *top_level_acceleration_structure) const override;
};

class brx_pal_d3d12_graphics_queue final : public brx_pal_graphics_queue
{
    ID3D12CommandQueue *m_graphics_queue;
    bool m_uma;
    bool m_support_ray_tracing;

public:
    brx_pal_d3d12_graphics_queue();
    void init(ID3D12CommandQueue *graphics_queue, bool uma, bool support_ray_tracing);
    void uninit(ID3D12CommandQueue *graphics_queue);
    ~brx_pal_d3d12_graphics_queue();
    void wait_and_submit(brx_pal_upload_command_buffer const *upload_command_buffer_to_wait, brx_pal_graphics_command_buffer const *graphics_command_buffer_to_submit, brx_pal_fence *fence_to_signal) const override;
    bool submit_and_present(brx_pal_graphics_command_buffer *graphics_command_buffer_to_submit, brx_pal_swap_chain *swap_chain_to_present, uint32_t swap_chain_image_index, brx_pal_fence *fence_to_signal) const override;
};

class brx_pal_d3d12_upload_queue final : public brx_pal_upload_queue
{
    ID3D12CommandQueue *m_upload_queue;
    bool m_uma;
    bool m_support_ray_tracing;

public:
    brx_pal_d3d12_upload_queue();
    void init(ID3D12CommandQueue *upload_queue, bool uma, bool support_ray_tracing);
    void uninit(ID3D12CommandQueue *upload_queue);
    ~brx_pal_d3d12_upload_queue();
    void submit_and_signal(brx_pal_upload_command_buffer const *upload_command_buffer_to_submit_and_signal) const override;
};

class brx_pal_d3d12_graphics_command_buffer final : public brx_pal_graphics_command_buffer
{
    bool m_uma;
    bool m_support_ray_tracing;
    ID3D12CommandAllocator *m_command_allocator;
    ID3D12GraphicsCommandList4 *m_command_list;
    brx_pal_d3d12_descriptor_allocator *m_descriptor_allocator;
    class brx_pal_d3d12_render_pass const *m_current_render_pass;
    class brx_pal_d3d12_frame_buffer const *m_current_frame_buffer;
    mcrt_vector<uint32_t> m_current_vertex_buffer_strides;

public:
    brx_pal_d3d12_graphics_command_buffer();
    void init(ID3D12Device *device, bool uma, bool support_ray_tracing, brx_pal_d3d12_descriptor_allocator *descriptor_allocator);
    void uninit();
    ~brx_pal_d3d12_graphics_command_buffer();
    ID3D12CommandAllocator *get_command_allocator() const;
    ID3D12GraphicsCommandList4 *get_command_list() const;
    void begin() override;
    void end() override;
    void begin_debug_utils_label(char const *label_name) override;
    void end_debug_utils_label() override;
    void acquire(uint32_t storage_asset_buffer_count, brx_pal_storage_asset_buffer const *const *storage_asset_buffers, uint32_t sampled_asset_image_subresource_count, BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE const *sampled_asset_image_subresources, uint32_t compacted_bottom_level_acceleration_structure_count, brx_pal_compacted_bottom_level_acceleration_structure const *const *compacted_bottom_level_acceleration_structures) override;
    void begin_render_pass(brx_pal_render_pass const *render_pass, brx_pal_frame_buffer const *frame_buffer, uint32_t width, uint32_t height, uint32_t color_clear_value_count, float const (*color_clear_values)[4], float const *depth_clear_value, uint8_t const *stencil_clear_value) override;
    void end_render_pass() override;
    void storage_resource_load_dont_care(uint32_t storage_buffer_count, brx_pal_storage_buffer const *const *storage_buffers, uint32_t storage_image_count, brx_pal_storage_image const *const *storage_images) override;
    void storage_resource_load_load(uint32_t storage_buffer_count, brx_pal_storage_buffer const *const *storage_buffers, uint32_t storage_image_count, brx_pal_storage_image const *const *storage_images) override;
    void storage_resource_barrier(uint32_t storage_buffer_count, brx_pal_storage_buffer const *const *storage_buffers, uint32_t storage_image_count, brx_pal_storage_image const *const *storage_images) override;
    void storage_resource_store(uint32_t storage_buffer_count, brx_pal_storage_buffer const *const *storage_buffers, uint32_t storage_image_count, brx_pal_storage_image const *const *storage_images) override;
    void bind_graphics_pipeline(brx_pal_graphics_pipeline const *graphics_pipeline) override;
    void set_view_port(uint32_t width, uint32_t height) override;
    void set_scissor(int32_t offset_width, int32_t offset_height, uint32_t width, uint32_t height) override;
    void bind_graphics_descriptor_sets(brx_pal_pipeline_layout const *pipeline_layout, uint32_t descriptor_set_count, brx_pal_descriptor_set const *const *descriptor_sets, uint32_t dynamic_offet_count, uint32_t const *dynamic_offsets) override;
    void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) override;
    void bind_compute_pipeline(brx_pal_compute_pipeline const *compute_pipeline) override;
    void bind_compute_descriptor_sets(brx_pal_pipeline_layout const *pipeline_layout, uint32_t descriptor_set_count, brx_pal_descriptor_set const *const *descriptor_sets, uint32_t dynamic_offet_count, uint32_t const *dynamic_offsets) override;
    void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) override;
    void build_intermediate_bottom_level_acceleration_structure(brx_pal_intermediate_bottom_level_acceleration_structure *intermediate_bottom_level_acceleration_structure, uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *bottom_level_acceleration_structure_geometries, brx_pal_scratch_buffer *scratch_buffer) override;
    void build_intermediate_bottom_level_acceleration_structure_store(uint32_t intermediate_bottom_level_acceleration_structure_count, brx_pal_intermediate_bottom_level_acceleration_structure const *const *intermediate_bottom_level_acceleration_structures) override;
    void update_intermediate_bottom_level_acceleration_structure(brx_pal_intermediate_bottom_level_acceleration_structure *intermediate_bottom_level_acceleration_structure, brx_pal_acceleration_structure_build_input_read_only_buffer const *const *bottom_level_acceleration_structure_geometry_vertex_position_buffers, brx_pal_scratch_buffer *scratch_buffer) override;
    void update_intermediate_bottom_level_acceleration_structure_store(uint32_t intermediate_bottom_level_acceleration_structure_count, brx_pal_intermediate_bottom_level_acceleration_structure const *const *intermediate_bottom_level_acceleration_structures) override;
    void build_top_level_acceleration_structure(brx_pal_top_level_acceleration_structure *top_level_acceleration_structure, uint32_t top_level_acceleration_structure_instance_count, brx_pal_top_level_acceleration_structure_instance_upload_buffer *top_level_acceleration_structure_instance_upload_buffer, brx_pal_scratch_buffer *scratch_buffer) override;
    void build_top_level_acceleration_structure_store(brx_pal_top_level_acceleration_structure *top_level_acceleration_structure) override;
    void update_top_level_acceleration_structure(brx_pal_top_level_acceleration_structure *top_level_acceleration_structure, brx_pal_top_level_acceleration_structure_instance_upload_buffer *top_level_acceleration_structure_instance_upload_buffer, brx_pal_scratch_buffer *scratch_buffer) override;
    void update_top_level_acceleration_structure_store(brx_pal_top_level_acceleration_structure *top_level_acceleration_structure) override;
};

class brx_pal_d3d12_upload_command_buffer final : public brx_pal_upload_command_buffer
{
    bool m_uma;
    bool m_support_ray_tracing;

    ID3D12CommandAllocator *m_command_allocator;
    ID3D12GraphicsCommandList4 *m_command_list;
    ID3D12Fence *m_upload_queue_submit_fence;

public:
    brx_pal_d3d12_upload_command_buffer();
    void init(ID3D12Device *device, bool uma, bool support_ray_tracing);
    void uninit();
    ~brx_pal_d3d12_upload_command_buffer();
    ID3D12CommandAllocator *get_command_allocator() const;
    ID3D12GraphicsCommandList4 *get_command_list() const;
    ID3D12Fence *get_upload_queue_submit_fence() const;
    void begin() override;
    void end() override;
    void asset_resource_load_dont_care(uint32_t storage_asset_buffer_count, brx_pal_storage_asset_buffer const *const *storage_asset_buffers, uint32_t sampled_asset_image_subresource_count, BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE const *sampled_asset_image_subresources) override;
    void asset_resource_store(uint32_t storage_asset_buffer_count, brx_pal_storage_asset_buffer const *const *storage_asset_buffers, uint32_t sampled_asset_image_subresource_count, BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE const *sampled_asset_image_subresources) override;
    void upload_from_staging_upload_buffer_to_storage_asset_buffer(brx_pal_storage_asset_buffer *storage_asset_buffer, uint64_t dst_offset, brx_pal_staging_upload_buffer *staging_upload_buffer, uint64_t src_offset, uint32_t src_size) override;
    void upload_from_staging_upload_buffer_to_sampled_asset_image(BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE const *sampled_asset_image_subresource, BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT sampled_asset_image_format, uint32_t sampled_asset_image_zeroth_width, uint32_t sampled_asset_image_zeroth_height, brx_pal_staging_upload_buffer *staging_upload_buffer, uint64_t src_offset, uint32_t src_row_pitch, uint32_t src_row_count) override;
    void acceleration_structure_build_input_read_only_buffer_load(uint32_t acceleration_structure_build_input_read_only_buffer_count, brx_pal_acceleration_structure_build_input_read_only_buffer const *const *acceleration_structure_build_input_read_only_buffers) override;
    void acceleration_structure_build_input_read_only_buffer_store(uint32_t acceleration_structure_build_input_read_only_buffer_count, brx_pal_acceleration_structure_build_input_read_only_buffer const *const *acceleration_structure_build_input_read_only_buffers) override;
    void build_non_compacted_bottom_level_acceleration_structure(brx_pal_non_compacted_bottom_level_acceleration_structure *non_compacted_bottom_level_acceleration_structure, uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *bottom_level_acceleration_structure_geometries, brx_pal_scratch_buffer *scratch_buffer, brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool *compacted_bottom_level_acceleration_structure_size_query_pool, uint32_t query_index) override;
    void compact_bottom_level_acceleration_structure(brx_pal_compacted_bottom_level_acceleration_structure *destination_compacted_bottom_level_acceleration_structure, brx_pal_non_compacted_bottom_level_acceleration_structure *source_non_compacted_bottom_level_acceleration_structure) override;
    void release(uint32_t storage_asset_buffer_count, brx_pal_storage_asset_buffer const *const *storage_asset_buffers, uint32_t sampled_asset_image_subresource_count, BRX_PAL_SAMPLED_ASSET_IMAGE_SUBRESOURCE const *sampled_asset_image_subresources, uint32_t compacted_bottom_level_acceleration_structure_count, brx_pal_compacted_bottom_level_acceleration_structure const *const *compacted_bottom_level_acceleration_structures) override;
};

class brx_pal_d3d12_fence : public brx_pal_fence
{
    ID3D12Fence *m_fence;

public:
    brx_pal_d3d12_fence(ID3D12Fence *fence);
    ID3D12Fence *get_fence() const;
    void steal(ID3D12Fence **out_fence);
    ~brx_pal_d3d12_fence();
};

struct brx_pal_d3d12_descriptor_layout
{
    BRX_PAL_DESCRIPTOR_TYPE m_root_parameter_type;
    uint32_t m_root_parameter_shader_register;
    uint32_t m_root_descriptor_table_num_descriptors;
};

class brx_pal_d3d12_descriptor_set_layout final : public brx_pal_descriptor_set_layout
{
    mcrt_vector<brx_pal_d3d12_descriptor_layout> m_descriptor_layouts;

public:
    brx_pal_d3d12_descriptor_set_layout();
    void init(uint32_t descriptor_set_binding_count, BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const *wrapped_descriptor_set_bindingst);
    void uninit();
    uint32_t get_descriptor_layout_count() const;
    brx_pal_d3d12_descriptor_layout const *get_descriptor_layouts() const;
};

class brx_pal_d3d12_pipeline_layout final : public brx_pal_pipeline_layout
{
    ID3D12RootSignature *m_root_signature;

public:
    brx_pal_d3d12_pipeline_layout();
    void init(ID3D12Device *device, decltype(D3D12SerializeRootSignature) *pfn_d3d12_serialize_root_signature, uint32_t descriptor_set_layout_count, brx_pal_descriptor_set_layout const *const *descriptor_set_layouts);
    void uninit();
    ~brx_pal_d3d12_pipeline_layout();
    ID3D12RootSignature *get_root_signature() const;
};

struct brx_pal_d3d12_descriptor_binding
{
    BRX_PAL_DESCRIPTOR_TYPE m_root_parameter_type;
    uint32_t m_root_parameter_shader_register;
    D3D12_GPU_VIRTUAL_ADDRESS m_root_constant_buffer_view_address_base;
    uint32_t m_root_descriptor_table_num_descriptors;
    mcrt_vector<ID3D12Resource *> m_root_descriptor_table_resources;
    mcrt_vector<D3D12_SHADER_RESOURCE_VIEW_DESC> m_root_descriptor_table_shader_resource_views;
    mcrt_vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> m_root_descriptor_table_unordered_access_views;
    mcrt_vector<D3D12_SAMPLER_DESC> m_root_descriptor_table_samplers;
};

class brx_pal_d3d12_descriptor_set final : public brx_pal_descriptor_set
{
    mcrt_vector<brx_pal_d3d12_descriptor_binding> m_descriptor_bindings;

public:
    brx_pal_d3d12_descriptor_set();
    void init(brx_pal_d3d12_descriptor_allocator *descriptor_allocator, brx_pal_descriptor_set_layout const *descriptor_set_layout, uint32_t unbounded_descriptor_count);
    void uninit(brx_pal_d3d12_descriptor_allocator *descriptor_allocator);
    ~brx_pal_d3d12_descriptor_set();
    void write_descriptor(ID3D12Device *device, brx_pal_d3d12_descriptor_allocator *descriptor_allocator, uint32_t dst_binding, BRX_PAL_DESCRIPTOR_TYPE descriptor_type, uint32_t dst_descriptor_start_index, uint32_t src_descriptor_count, brx_pal_uniform_upload_buffer const *const *src_dynamic_uniform_buffers, uint32_t const *src_dynamic_uniform_buffer_ranges, brx_pal_read_only_storage_buffer const *const *src_read_only_storage_buffers, brx_pal_storage_buffer const *const *src_storage_buffers, brx_pal_sampled_image const *const *src_sampled_images, brx_pal_storage_image const *const *src_storage_images, brx_pal_sampler const *const *src_samplers, brx_pal_top_level_acceleration_structure const *const *src_top_level_acceleration_structures);
    uint32_t get_descriptor_binding_count() const;
    brx_pal_d3d12_descriptor_binding const *get_descriptor_bindings() const;
};

class brx_pal_d3d12_render_pass final : public brx_pal_render_pass
{
    mcrt_vector<BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT> m_color_attachment_formats;
    mcrt_vector<uint32_t> m_color_attachment_load_clear_indices;
    mcrt_unordered_set<uint32_t> m_color_attachment_store_flush_for_present_indices;
    BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT m_depth_stencil_attachment_format;

public:
    brx_pal_d3d12_render_pass();
    void init(uint32_t color_attachment_count, BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const *color_attachments, BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT const *depth_stencil_attachment);
    void uninit();
    uint32_t get_color_attachment_count() const;
    BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const *get_color_attachment_formats() const;
    uint32_t get_color_attachment_load_clear_count() const;
    uint32_t const *get_color_attachment_load_clear_indices() const;
    bool is_color_attachment_store_flush_for_present(uint32_t color_attachment_index) const;
    BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT get_depth_stencil_attachment_format() const;
};

class brx_pal_d3d12_graphics_pipeline final : public brx_pal_graphics_pipeline
{
    D3D12_PRIMITIVE_TOPOLOGY m_primitive_topology;
    ID3D12PipelineState *m_pipeline_state;

public:
    brx_pal_d3d12_graphics_pipeline();
    void init(ID3D12Device *device, brx_pal_render_pass const *render_pass, brx_pal_pipeline_layout const *pipeline_layout, size_t vertex_shader_module_code_size, void const *vertex_shader_module_code, size_t fragment_shader_module_code_size, void const *fragment_shader_module_code, bool enable_back_face_cull, bool front_ccw, bool enable_depth_clip, uint32_t forced_sample_count, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION depth_compare_operation, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION blend_operation);
    void uninit();
    ~brx_pal_d3d12_graphics_pipeline();
    D3D12_PRIMITIVE_TOPOLOGY get_primitive_topology() const;
    ID3D12PipelineState *get_pipeline() const;
};

class brx_pal_d3d12_compute_pipeline final : public brx_pal_compute_pipeline
{
    ID3D12PipelineState *m_pipeline_state;

public:
    brx_pal_d3d12_compute_pipeline();
    void init(ID3D12Device *device, brx_pal_pipeline_layout const *pipeline_layout, size_t compute_shader_module_code_size, void const *compute_shader_module_code);
    void uninit();
    ~brx_pal_d3d12_compute_pipeline();
    ID3D12PipelineState *get_pipeline() const;
};

class brx_pal_d3d12_frame_buffer final : public brx_pal_frame_buffer
{
    mcrt_vector<brx_pal_d3d12_color_attachment_image const *> m_color_attachment_images;
    brx_pal_d3d12_depth_stencil_attachment_image const *m_depth_stencil_attachment_image;

public:
    brx_pal_d3d12_frame_buffer();
    void init(brx_pal_render_pass const *wrapped_render_pass, uint32_t width, uint32_t height, uint32_t color_attachment_count, brx_pal_color_attachment_image const *const *color_attachments, brx_pal_depth_stencil_attachment_image const *depth_stencil_attachment);
    void uninit();
    uint32_t get_color_attachment_image_count() const;
    brx_pal_d3d12_color_attachment_image const *const *get_color_attachment_images() const;
    brx_pal_d3d12_depth_stencil_attachment_image const *get_depth_stencil_attachment_image() const;
};

class brx_pal_d3d12_uniform_upload_buffer final : public brx_pal_uniform_upload_buffer
{
    ID3D12Resource *m_resource = NULL;
    D3D12MA::Allocation *m_allocation;
    void *m_host_memory_range_base = NULL;

public:
    brx_pal_d3d12_uniform_upload_buffer();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *uniform_upload_buffer_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_uniform_upload_buffer();
    ID3D12Resource *get_resource() const;
    void *get_host_memory_range_base() const override;
};

class brx_pal_d3d12_staging_upload_buffer final : public brx_pal_staging_upload_buffer
{
    ID3D12Resource *m_resource = NULL;
    D3D12MA::Allocation *m_allocation;
    void *m_host_memory_range_base = NULL;

public:
    brx_pal_d3d12_staging_upload_buffer();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *staging_upload_buffer_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_staging_upload_buffer();
    ID3D12Resource *get_resource() const;
    void *get_host_memory_range_base() const override;
};

class brx_pal_d3d12_read_only_storage_buffer : public brx_pal_read_only_storage_buffer
{
public:
    virtual ID3D12Resource *get_resource() const = 0;
    virtual D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const = 0;
};

class brx_pal_d3d12_storage_buffer : public brx_pal_storage_buffer
{
public:
    virtual ID3D12Resource *get_resource() const = 0;
    virtual D3D12_UNORDERED_ACCESS_VIEW_DESC const *get_unordered_access_view_desc() const = 0;
};

class brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer : public brx_pal_acceleration_structure_build_input_read_only_buffer
{
public:
    virtual ID3D12Resource *get_resource() const = 0;
};

class brx_pal_d3d12_storage_intermediate_buffer final : public brx_pal_storage_intermediate_buffer, brx_pal_d3d12_read_only_storage_buffer, brx_pal_d3d12_storage_buffer, brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_shader_resource_view_desc;
    D3D12_UNORDERED_ACCESS_VIEW_DESC m_unordered_access_view_desc;

public:
    brx_pal_d3d12_storage_intermediate_buffer();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *storage_buffer_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_storage_intermediate_buffer();

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const override;
    D3D12_UNORDERED_ACCESS_VIEW_DESC const *get_unordered_access_view_desc() const override;
    ID3D12Resource *get_resource() const override;
    brx_pal_read_only_storage_buffer const *get_read_only_storage_buffer() const override;
    brx_pal_storage_buffer const *get_storage_buffer() const override;
    brx_pal_acceleration_structure_build_input_read_only_buffer const *get_acceleration_structure_build_input_read_only_buffer() const override;
};

class brx_pal_d3d12_storage_asset_buffer final : public brx_pal_storage_asset_buffer, brx_pal_d3d12_read_only_storage_buffer, brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_shader_resource_view_desc;

public:
    brx_pal_d3d12_storage_asset_buffer();
    void init(bool uma, D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *storage_buffer_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_storage_asset_buffer();

    ID3D12Resource *get_resource() const override;

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const override;
    brx_pal_read_only_storage_buffer const *get_read_only_storage_buffer() const override;
    brx_pal_acceleration_structure_build_input_read_only_buffer const *get_acceleration_structure_build_input_read_only_buffer() const override;
};

class brx_pal_d3d12_sampled_image : public brx_pal_sampled_image
{
public:
    virtual ID3D12Resource *get_resource() const = 0;
    virtual D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const = 0;
};

class brx_pal_d3d12_color_attachment_image : public brx_pal_color_attachment_image
{
public:
    virtual ID3D12Resource *get_resource() const = 0;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE get_render_target_view_descriptor() const = 0;
    virtual bool allow_sampled_image() const = 0;
};

class brx_pal_d3d12_depth_stencil_attachment_image : public brx_pal_depth_stencil_attachment_image
{
public:
    virtual ID3D12Resource *get_resource() const = 0;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE get_depth_stencil_view_descriptor() const = 0;
    virtual bool allow_sampled_image() const = 0;
};

class brx_pal_d3d12_storage_image : public brx_pal_storage_image
{
public:
    virtual ID3D12Resource *get_resource() const = 0;
    virtual D3D12_UNORDERED_ACCESS_VIEW_DESC const *get_unordered_access_view_desc() const = 0;
    virtual bool allow_sampled_image() const = 0;
};

class brx_pal_d3d12_color_attachment_intermediate_image final : public brx_pal_d3d12_color_attachment_image, brx_pal_d3d12_sampled_image
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    ID3D12DescriptorHeap *m_render_target_view_descriptor_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_render_target_view_descriptor;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_shader_resource_view_desc;
    bool m_allow_sampled_image;

public:
    brx_pal_d3d12_color_attachment_intermediate_image();
    void init(ID3D12Device *device, D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *color_attachment_intermediate_image_memory_pool, BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT color_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image);
    void uninit();
    ~brx_pal_d3d12_color_attachment_intermediate_image();

private:
    ID3D12Resource *get_resource() const override;
    D3D12_CPU_DESCRIPTOR_HANDLE get_render_target_view_descriptor() const override;
    D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const override;
    bool allow_sampled_image() const override;
    brx_pal_sampled_image const *get_sampled_image() const override;
};

class brx_pal_d3d12_depth_stencil_attachment_intermediate_image final : public brx_pal_d3d12_depth_stencil_attachment_image, brx_pal_d3d12_sampled_image
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    ID3D12DescriptorHeap *m_depth_stencil_view_descriptor_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_depth_stencil_view_descriptor;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_shader_resource_view_desc;
    bool m_allow_sampled_image;

public:
    brx_pal_d3d12_depth_stencil_attachment_intermediate_image();
    void init(ID3D12Device *device, D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *depth_stencil_attachment_intermediate_image_memory_pool, BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT wrapped_depth_stencil_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image);
    void uninit();
    ~brx_pal_d3d12_depth_stencil_attachment_intermediate_image();

private:
    ID3D12Resource *get_resource() const override;
    D3D12_CPU_DESCRIPTOR_HANDLE get_depth_stencil_view_descriptor() const override;
    D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const override;
    bool allow_sampled_image() const override;
    brx_pal_sampled_image const *get_sampled_image() const override;
};

class brx_pal_d3d12_storage_intermediate_image final : public brx_pal_d3d12_storage_image, brx_pal_d3d12_sampled_image
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    D3D12_UNORDERED_ACCESS_VIEW_DESC m_unordered_access_view_desc;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_shader_resource_view_desc;
    bool m_allow_sampled_image;

public:
    brx_pal_d3d12_storage_intermediate_image();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *storage_intermediate_image_memory_pool, BRX_PAL_STORAGE_IMAGE_FORMAT wrapped_storage_image_format, uint32_t width, uint32_t height, bool volume, uint32_t depth, bool allow_sampled_image);
    void uninit();
    ~brx_pal_d3d12_storage_intermediate_image();

private:
    ID3D12Resource *get_resource() const override;
    D3D12_UNORDERED_ACCESS_VIEW_DESC const *get_unordered_access_view_desc() const override;
    D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const override;
    bool allow_sampled_image() const override;
    brx_pal_sampled_image const *get_sampled_image() const override;
};

class brx_pal_d3d12_sampled_asset_image final : public brx_pal_sampled_asset_image, brx_pal_d3d12_sampled_image
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_shader_resource_view_desc;
    uint32_t m_mip_levels;
    uint32_t m_array_layers;

public:
    brx_pal_d3d12_sampled_asset_image();
    void init(bool uma, D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *sampled_asset_image_memory_pool, DXGI_FORMAT sampled_asset_image_format, uint32_t width, uint32_t height, bool array, uint32_t array_layers, uint32_t mip_levels);
    void uninit();
    ~brx_pal_d3d12_sampled_asset_image();

    ID3D12Resource *get_resource() const override;
    uint32_t get_mip_levels() const;
    uint32_t get_array_layers() const;

private:
    D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const override;
    brx_pal_sampled_image const *get_sampled_image() const override;
};

class brx_pal_d3d12_sampler : public brx_pal_sampler
{
    D3D12_SAMPLER_DESC m_sampler_desc;

public:
    brx_pal_d3d12_sampler(D3D12_SAMPLER_DESC const *sampler_desc);
    D3D12_SAMPLER_DESC const *get_sampler_desc() const;
};

class brx_pal_d3d12_swap_chain_image final : public brx_pal_d3d12_color_attachment_image
{
    ID3D12Resource *m_resource;
    D3D12_CPU_DESCRIPTOR_HANDLE m_render_target_view_descriptor;

public:
    brx_pal_d3d12_swap_chain_image(ID3D12Resource *resource, D3D12_CPU_DESCRIPTOR_HANDLE render_target_view_descriptor);
    void steal(ID3D12Resource **out_resource);
    ID3D12Resource *get_resource() const override;
    D3D12_CPU_DESCRIPTOR_HANDLE get_render_target_view_descriptor() const override;
    bool allow_sampled_image() const override;
    brx_pal_sampled_image const *get_sampled_image() const override;
};

class brx_pal_d3d12_swap_chain final : public brx_pal_swap_chain
{
    IDXGISwapChain3 *m_swap_chain;
    DXGI_FORMAT m_image_format;
    uint32_t m_image_width;
    uint32_t m_image_height;
    uint32_t m_image_count;
    ID3D12DescriptorHeap *m_rtv_descriptor_heap;
    mcrt_vector<brx_pal_d3d12_swap_chain_image> m_images;

public:
    brx_pal_d3d12_swap_chain(IDXGISwapChain3 *swap_chain, DXGI_FORMAT image_format, uint32_t image_width, uint32_t image_height, uint32_t image_count, ID3D12DescriptorHeap *rtv_descriptor_heap, mcrt_vector<brx_pal_d3d12_swap_chain_image> &&m_images);
    IDXGISwapChain3 *get_swap_chain() const;
    BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT get_image_format() const override;
    uint32_t get_image_width() const override;
    uint32_t get_image_height() const override;
    uint32_t get_image_count() const override;
    brx_pal_color_attachment_image const *get_image(uint32_t swap_chain_image_index) const override;
    void steal(IDXGISwapChain3 **out_swap_chain, ID3D12DescriptorHeap **out_rtv_descriptor_heap, mcrt_vector<brx_pal_d3d12_swap_chain_image> &out_images);
    ~brx_pal_d3d12_swap_chain();
};

class brx_pal_d3d12_scratch_buffer final : public brx_pal_scratch_buffer
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;

public:
    brx_pal_d3d12_scratch_buffer();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *scratch_buffer_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_scratch_buffer();
    ID3D12Resource *get_resource() const;
};

class brx_pal_d3d12_bottom_level_acceleration_structure : public brx_pal_bottom_level_acceleration_structure
{
public:
    virtual ID3D12Resource *get_resource() const = 0;
};

class brx_pal_d3d12_intermediate_bottom_level_acceleration_structure final : public brx_pal_intermediate_bottom_level_acceleration_structure, brx_pal_d3d12_bottom_level_acceleration_structure
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    mcrt_vector<BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY> m_bottom_level_acceleration_structure_geometries;

public:
    brx_pal_d3d12_intermediate_bottom_level_acceleration_structure();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *intermediate_bottom_level_acceleration_structure_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_intermediate_bottom_level_acceleration_structure();
    ID3D12Resource *get_resource() const override;
    void set_bottom_level_acceleration_structure_geometries(uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *wrapped_bottom_level_acceleration_structure_geometries);
    mcrt_vector<BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY> const &get_bottom_level_acceleration_structure_geometries() const;

private:
    brx_pal_bottom_level_acceleration_structure const *get_bottom_level_acceleration_structure() const override;
};

class brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure final : public brx_pal_non_compacted_bottom_level_acceleration_structure
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;

public:
    brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *non_compacted_bottom_level_acceleration_structure_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure();
    ID3D12Resource *get_resource() const;
};

class brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool final : public brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC volatile *m_host_memory_range_base;

public:
    brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool, uint32_t query_count);
    void uninit();
    ~brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool();
    ID3D12Resource *get_resource() const;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC volatile *get_host_memory_range_base() const;
};

class brx_pal_d3d12_compacted_bottom_level_acceleration_structure final : public brx_pal_compacted_bottom_level_acceleration_structure, brx_pal_d3d12_bottom_level_acceleration_structure
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;

public:
    brx_pal_d3d12_compacted_bottom_level_acceleration_structure();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *compacted_bottom_level_acceleration_structure_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_compacted_bottom_level_acceleration_structure();
    ID3D12Resource *get_resource() const override;

private:
    brx_pal_bottom_level_acceleration_structure const *get_bottom_level_acceleration_structure() const override;
};

class brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer final : public brx_pal_top_level_acceleration_structure_instance_upload_buffer
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    D3D12_RAYTRACING_INSTANCE_DESC *m_host_memory_range_base;

public:
    brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer();
    void init(uint32_t instance_count, D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *top_level_acceleration_structure_instance_upload_buffer_memory_pool);
    void uninit();
    ~brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer();
    void write_instance(uint32_t instance_index, BRX_PAL_TOP_LEVEL_ACCELERATION_STRUCTURE_INSTANCE const *bottom_top_acceleration_structure_instance) override;
    ID3D12Resource *get_resource() const;
};

class brx_pal_d3d12_top_level_acceleration_structure final : public brx_pal_top_level_acceleration_structure
{
    ID3D12Resource *m_resource;
    D3D12MA::Allocation *m_allocation;
    D3D12_SHADER_RESOURCE_VIEW_DESC m_shader_resource_view_desc;
    uint32_t m_instance_count;

public:
    brx_pal_d3d12_top_level_acceleration_structure();
    void init(D3D12MA::Allocator *memory_allocator, D3D12MA::Pool *top_level_acceleration_structure_memory_pool, uint32_t size);
    void uninit();
    ~brx_pal_d3d12_top_level_acceleration_structure();
    ID3D12Resource *get_resource() const;
    D3D12_SHADER_RESOURCE_VIEW_DESC const *get_shader_resource_view_desc() const;
    void set_instance_count(uint32_t instance_count);
    uint32_t get_instance_count() const;
};

#endif