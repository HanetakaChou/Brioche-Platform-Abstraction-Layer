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
#include "../../McRT-Malloc/include/mcrt_malloc.h"
#include <assert.h>
#include <new>

static constexpr DXGI_FORMAT const g_preferred_swap_chain_image_format = DXGI_FORMAT_R8G8B8A8_UNORM;
static constexpr uint32_t const g_preferred_swap_chain_image_count = 3U;

extern brx_pal_device *brx_pal_create_d3d12_device(void *wsi_connection, bool support_ray_tracing)
{
    void *new_unwrapped_device_base = mcrt_malloc(sizeof(brx_pal_d3d12_device), alignof(brx_pal_d3d12_device));
    assert(NULL != new_unwrapped_device_base);

    brx_pal_d3d12_device *new_unwrapped_device = new (new_unwrapped_device_base) brx_pal_d3d12_device{};
    new_unwrapped_device->init(support_ray_tracing);
    return new_unwrapped_device;
}

brx_pal_d3d12_device::brx_pal_d3d12_device()
    : m_pfn_d3d12_serialize_root_signature(NULL),
      m_factory(NULL),
      m_adapter(NULL),
      m_device(NULL),
      m_graphics_queue(NULL),
      m_upload_queue(NULL),
      m_memory_allocator(NULL),
      m_uniform_upload_buffer_memory_pool(NULL),
      m_staging_upload_buffer_memory_pool(NULL),
      m_storage_intermediate_buffer_memory_pool(NULL),
      m_storage_asset_buffer_memory_pool(NULL),
      m_color_attachment_intermediate_image_memory_pool(NULL),
      m_depth_stencil_attachment_intermediate_image_memory_pool(NULL),
      m_storage_intermediate_image_memory_pool(NULL),
      m_sampled_asset_image_memory_pool(NULL),
      m_scratch_buffer_memory_pool(NULL),
      m_intermediate_bottom_level_acceleration_structure_memory_pool(NULL),
      m_non_compacted_bottom_level_acceleration_structure_memory_pool(NULL),
      m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool(NULL),
      m_compacted_bottom_level_acceleration_structure_memory_pool(NULL),
      m_top_level_acceleration_structure_instance_upload_buffer_memory_pool(NULL),
      m_top_level_acceleration_structure_memory_pool(NULL)
{
}

void brx_pal_d3d12_device::init(bool support_ray_tracing)
{
    HMODULE const dynamic_library_dxgi = GetModuleHandleW(L"DXGI.dll");
    assert(NULL != dynamic_library_dxgi);

    HMODULE const dynamic_library_d3d12 = GetModuleHandleW(L"D3D12.dll");
    assert(NULL != dynamic_library_d3d12);

    decltype(CreateDXGIFactory2) *const pfn_create_dxgi_factory_2 = reinterpret_cast<decltype(CreateDXGIFactory2) *>(GetProcAddress(dynamic_library_dxgi, "CreateDXGIFactory2"));
    assert(NULL != pfn_create_dxgi_factory_2);

#ifndef NDEBUG
    decltype(D3D12GetDebugInterface) *const pfn_d3d12_get_debug_interface = reinterpret_cast<decltype(D3D12GetDebugInterface) *>(GetProcAddress(dynamic_library_d3d12, "D3D12GetDebugInterface"));
    assert(NULL != pfn_d3d12_get_debug_interface);
#endif

    decltype(D3D12CreateDevice) *const pfn_d3d12_create_device = reinterpret_cast<decltype(D3D12CreateDevice) *>(GetProcAddress(dynamic_library_d3d12, "D3D12CreateDevice"));
    assert(NULL != pfn_d3d12_create_device);

    assert(NULL == this->m_pfn_d3d12_serialize_root_signature);
    this->m_pfn_d3d12_serialize_root_signature = reinterpret_cast<decltype(D3D12SerializeRootSignature) *>(GetProcAddress(dynamic_library_d3d12, "D3D12SerializeRootSignature"));
    assert(NULL != this->m_pfn_d3d12_serialize_root_signature);

    this->m_support_ray_tracing = support_ray_tracing;

#ifndef NDEBUG
    {
        ID3D12Debug *debug = NULL;

        HRESULT hr_debug_interface = pfn_d3d12_get_debug_interface(IID_PPV_ARGS(&debug));
        assert(SUCCEEDED(hr_debug_interface));

        debug->EnableDebugLayer();
        debug->Release();
    }
#endif

    assert(NULL == this->m_factory);
    {
        HRESULT hr_create_factory = pfn_create_dxgi_factory_2(
#ifndef NDEBUG
            DXGI_CREATE_FACTORY_DEBUG,
#else
            0U,
#endif
            IID_PPV_ARGS(&this->m_factory));
        assert(SUCCEEDED(hr_create_factory));
    }

    IDXGIAdapter *first_discrete_gpu_adapter = NULL;
    IDXGIAdapter *first_non_discrete_gpu_adapter = NULL;

    for (UINT adapter_index = 0U; adapter_index < 7U; ++adapter_index)
    {
        IDXGIAdapter *new_adapter = NULL;
        HRESULT hr_enum_adapters = SUCCEEDED(this->m_factory->EnumAdapters(adapter_index, &new_adapter));
        if (!(SUCCEEDED(hr_enum_adapters)))
        {
            break;
        }

        IDXGIAdapter3 *new_adapter_3 = NULL;
        HRESULT hr_query_interface = new_adapter->QueryInterface(IID_PPV_ARGS(&new_adapter_3));
        assert(SUCCEEDED(hr_query_interface));

        DXGI_QUERY_VIDEO_MEMORY_INFO local_video_memory_info;
        HRESULT hr_query_local_video_memory_info = new_adapter_3->QueryVideoMemoryInfo(0U, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local_video_memory_info);
        assert(SUCCEEDED(hr_query_local_video_memory_info));

        DXGI_QUERY_VIDEO_MEMORY_INFO non_local_video_memory_info;
        HRESULT hr_query_non_local_video_memory_info = new_adapter_3->QueryVideoMemoryInfo(0U, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &non_local_video_memory_info);
        assert(SUCCEEDED(hr_query_non_local_video_memory_info));

        new_adapter_3->Release();

        if (local_video_memory_info.Budget > 777U && non_local_video_memory_info.Budget > 777U)
        {
            assert(NULL == first_discrete_gpu_adapter);
            first_discrete_gpu_adapter = new_adapter;
            break;
        }
        else
        {
            // we should already "break" and exit the loop
            assert(NULL == first_discrete_gpu_adapter);

            if (NULL == first_non_discrete_gpu_adapter)
            {
                first_non_discrete_gpu_adapter = new_adapter;
            }
            else
            {
                new_adapter->Release();
            }
        }
    }

    assert(NULL == this->m_adapter);
    if (NULL != first_discrete_gpu_adapter)
    {
        this->m_adapter = first_discrete_gpu_adapter;
        first_discrete_gpu_adapter = NULL;

        if (NULL != first_non_discrete_gpu_adapter)
        {
            first_non_discrete_gpu_adapter->Release();
            first_non_discrete_gpu_adapter = NULL;
        }
    }
    else if (NULL != first_non_discrete_gpu_adapter)
    {
        // The discrete gpu is preferred
        assert(NULL == first_discrete_gpu_adapter);

        this->m_adapter = first_non_discrete_gpu_adapter;
        first_non_discrete_gpu_adapter = NULL;
    }
    else
    {
        assert(false);
    }

    assert(NULL == this->m_device);
    {
        HRESULT hr_create_device = pfn_d3d12_create_device(this->m_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&this->m_device));
        assert(SUCCEEDED(hr_create_device));
    }

    // UMA may not be the integrated GPU
    // XBOX and PlayStation are typically UMA
    {
        D3D12_FEATURE_DATA_ARCHITECTURE feature_support_data = {0U, FALSE, FALSE, FALSE};
        HRESULT hr_check_feature_support = this->m_device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &feature_support_data, sizeof(feature_support_data));
        assert(SUCCEEDED(hr_check_feature_support));

        // UMA may not be the integrated GPU
        // XBOX and PlayStation are typically UMA
        this->m_uma = (feature_support_data.UMA != FALSE);
        this->m_cache_coherent_uma = (feature_support_data.CacheCoherentUMA != FALSE);
        assert((!this->m_cache_coherent_uma) || this->m_uma);
    }

    assert(NULL == this->m_graphics_queue);
    {
        D3D12_COMMAND_QUEUE_DESC command_queue_desc = {
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            D3D12_COMMAND_QUEUE_FLAG_NONE,
            0U};
        HRESULT hr_create_command_queue = this->m_device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&this->m_graphics_queue));
        assert(SUCCEEDED(hr_create_command_queue));
    }

    assert(NULL == this->m_upload_queue);
    if ((!this->m_uma) || this->m_support_ray_tracing)
    {
        // On PlayStation, the copy operation is implemented by the compute shader.
        // Besides, the initial state must be common if the copy queue is involved.
        // We believe the peformance would be better if the application can provide more informations rather than depend on the implicit state transitions.
        D3D12_COMMAND_QUEUE_DESC command_queue_desc = {
            D3D12_COMMAND_LIST_TYPE_COMPUTE,
            D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
            D3D12_COMMAND_QUEUE_FLAG_NONE,
            0U};
        HRESULT hr_create_command_queue = this->m_device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&this->m_upload_queue));
        assert(SUCCEEDED(hr_create_command_queue));
    }

    assert(NULL == this->m_memory_allocator);
    {
        D3D12MA::ALLOCATOR_DESC allocator_desc = {D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED, this->m_device, 0U, NULL, this->m_adapter};

        HRESULT hr_create_allocator = D3D12MA::CreateAllocator(&allocator_desc, &this->m_memory_allocator);
        assert(SUCCEEDED(hr_create_allocator));
        assert((this->m_memory_allocator->IsUMA() != FALSE) == this->m_uma);
        assert((this->m_memory_allocator->IsCacheCoherentUMA() != FALSE) == this->m_cache_coherent_uma);
    }

    assert(NULL == this->m_uniform_upload_buffer_memory_pool);
    {
        D3D12MA::POOL_DESC const pool_desc = {
            D3D12MA::POOL_FLAG_NONE,
            {D3D12_HEAP_TYPE_CUSTOM,
             (this->m_uma && this->m_cache_coherent_uma) ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE,
             D3D12_MEMORY_POOL_L0,
             0U,
             0U},
            D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
            0U,
            0U,
            0U,
            D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT,
            NULL};
        HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_uniform_upload_buffer_memory_pool);
        assert(SUCCEEDED(hr_create_pool));
    }

    assert(NULL == this->m_staging_upload_buffer_memory_pool);
    {
        D3D12MA::POOL_DESC const pool_desc = {
            D3D12MA::POOL_FLAG_NONE,
            {D3D12_HEAP_TYPE_CUSTOM,
             (this->m_uma && this->m_cache_coherent_uma) ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE,
             D3D12_MEMORY_POOL_L0,
             0U,
             0U},
            D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
            0U,
            0U,
            0U,
            D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT,
            NULL};
        HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_staging_upload_buffer_memory_pool);
        assert(SUCCEEDED(hr_create_pool));
    }

    assert(NULL == this->m_storage_intermediate_buffer_memory_pool);
    {
        D3D12MA::POOL_DESC const pool_desc = {
            D3D12MA::POOL_FLAG_NONE,
            {D3D12_HEAP_TYPE_CUSTOM,
             this->m_uma ? (this->m_cache_coherent_uma ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE) : D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
             this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
             0U,
             0U},
            D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
            0U,
            0U,
            0U,
            D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT,
            NULL};
        HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_storage_intermediate_buffer_memory_pool);
        assert(SUCCEEDED(hr_create_pool));
    }

    assert(NULL == this->m_storage_asset_buffer_memory_pool);
    {
        D3D12MA::POOL_DESC const pool_desc = {
            D3D12MA::POOL_FLAG_NONE,
            {D3D12_HEAP_TYPE_CUSTOM,
             this->m_uma ? (this->m_cache_coherent_uma ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE) : D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
             this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
             0U,
             0U},
            D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
            0U,
            0U,
            0U,
            D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT,
            NULL};
        HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_storage_asset_buffer_memory_pool);
        assert(SUCCEEDED(hr_create_pool));
    }

    assert(NULL == this->m_color_attachment_intermediate_image_memory_pool);
    {
        D3D12MA::POOL_DESC const pool_desc = {
            D3D12MA::POOL_FLAG_NONE,
            {D3D12_HEAP_TYPE_CUSTOM,
             this->m_uma ? (this->m_cache_coherent_uma ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE) : D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
             this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
             0U,
             0U},
            D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
            0U,
            0U,
            0U,
            0U,
            NULL};
        HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_color_attachment_intermediate_image_memory_pool);
        assert(SUCCEEDED(hr_create_pool));
    }

    assert(NULL == this->m_depth_stencil_attachment_intermediate_image_memory_pool);
    {
        D3D12MA::POOL_DESC const pool_desc = {
            D3D12MA::POOL_FLAG_NONE,
            {D3D12_HEAP_TYPE_CUSTOM,
             this->m_uma ? (this->m_cache_coherent_uma ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE) : D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
             this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
             0U,
             0U},
            D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
            0U,
            0U,
            0U,
            0U,
            NULL};
        HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_depth_stencil_attachment_intermediate_image_memory_pool);
        assert(SUCCEEDED(hr_create_pool));
    }

    assert(NULL == this->m_storage_intermediate_image_memory_pool);
    {
        D3D12MA::POOL_DESC const pool_desc = {
            D3D12MA::POOL_FLAG_NONE,
            {D3D12_HEAP_TYPE_CUSTOM,
             this->m_uma ? (this->m_cache_coherent_uma ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE) : D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
             this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
             0U,
             0U},
            D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES,
            0U,
            0U,
            0U,
            0U,
            NULL};
        HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_storage_intermediate_image_memory_pool);
        assert(SUCCEEDED(hr_create_pool));
    }

    assert(NULL == this->m_sampled_asset_image_memory_pool);
    {
        D3D12MA::POOL_DESC const pool_desc = {
            D3D12MA::POOL_FLAG_NONE,
            {D3D12_HEAP_TYPE_CUSTOM,
             this->m_uma ? (this->m_cache_coherent_uma ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE) : D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
             this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
             0U,
             0U},
            D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES,
            0U,
            0U,
            0U,
            0U,
            NULL};
        HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_sampled_asset_image_memory_pool);
        assert(SUCCEEDED(hr_create_pool));
    }

    if (this->m_support_ray_tracing)
    {
        assert(NULL == this->m_scratch_buffer_memory_pool);
        {
            D3D12MA::POOL_DESC const pool_desc = {
                D3D12MA::POOL_FLAG_NONE,
                {D3D12_HEAP_TYPE_CUSTOM,
                 D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
                 this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
                 0U,
                 0U},
                D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
                0U,
                0U,
                0U,
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                NULL};

            HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_scratch_buffer_memory_pool);
            assert(SUCCEEDED(hr_create_pool));
        }

        assert(NULL == this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
        {
            D3D12MA::POOL_DESC const pool_desc = {
                D3D12MA::POOL_FLAG_NONE,
                {D3D12_HEAP_TYPE_CUSTOM,
                 D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
                 this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
                 0U,
                 0U},
                D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
                0U,
                0U,
                0U,
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                NULL};

            HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
            assert(SUCCEEDED(hr_create_pool));
        }

        assert(NULL == this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
        {
            D3D12MA::POOL_DESC const pool_desc = {
                D3D12MA::POOL_FLAG_NONE,
                {D3D12_HEAP_TYPE_CUSTOM,
                 D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
                 this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
                 0U,
                 0U},
                D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
                0U,
                0U,
                0U,
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                NULL};
            HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
            assert(SUCCEEDED(hr_create_pool));
        }

        assert(NULL == this->m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool);
        {
            D3D12MA::POOL_DESC const pool_desc = {
                D3D12MA::POOL_FLAG_NONE,
                {D3D12_HEAP_TYPE_CUSTOM,
                 D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
                 D3D12_MEMORY_POOL_L0,
                 0U,
                 0U},
                D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
                0U,
                0U,
                0U,
                alignof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC),
                NULL};
            HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool);
            assert(SUCCEEDED(hr_create_pool));
        }

        assert(NULL == this->m_compacted_bottom_level_acceleration_structure_memory_pool);
        {
            D3D12MA::POOL_DESC const pool_desc = {
                D3D12MA::POOL_FLAG_NONE,
                {D3D12_HEAP_TYPE_CUSTOM,
                 D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
                 this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
                 0U,
                 0U},
                D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
                0U,
                0U,
                0U,
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                NULL};
            HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_compacted_bottom_level_acceleration_structure_memory_pool);
            assert(SUCCEEDED(hr_create_pool));
        }

        assert(NULL == this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
        {
            D3D12MA::POOL_DESC const pool_desc = {
                D3D12MA::POOL_FLAG_NONE,
                {D3D12_HEAP_TYPE_CUSTOM,
                 (this->m_uma && this->m_cache_coherent_uma) ? D3D12_CPU_PAGE_PROPERTY_WRITE_BACK : D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE,
                 D3D12_MEMORY_POOL_L0,
                 0U,
                 0U},
                D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
                0U,
                0U,
                0U,
                D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT,
                NULL};
            HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
            assert(SUCCEEDED(hr_create_pool));
        }

        assert(NULL == this->m_top_level_acceleration_structure_memory_pool);
        {
            D3D12MA::POOL_DESC const pool_desc = {
                D3D12MA::POOL_FLAG_NONE,
                {D3D12_HEAP_TYPE_CUSTOM,
                 D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE,
                 this->m_uma ? D3D12_MEMORY_POOL_L0 : D3D12_MEMORY_POOL_L1,
                 0U,
                 0U},
                D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES,
                0U,
                0U,
                0U,
                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                NULL};
            HRESULT const hr_create_pool = this->m_memory_allocator->CreatePool(&pool_desc, &this->m_top_level_acceleration_structure_memory_pool);
            assert(SUCCEEDED(hr_create_pool));
        }
    }

    this->m_descriptor_allocator.init(this->m_device);
}

extern void brx_pal_destroy_d3d12_device(brx_pal_device *wrapped_device)
{
    assert(NULL != wrapped_device);
    brx_pal_d3d12_device *delete_unwrapped_device = static_cast<brx_pal_d3d12_device *>(wrapped_device);

    delete_unwrapped_device->uninit();

    delete_unwrapped_device->~brx_pal_d3d12_device();
    mcrt_free(delete_unwrapped_device);
}

void brx_pal_d3d12_device::uninit()
{
    this->m_descriptor_allocator.uninit();

    assert(NULL != this->m_uniform_upload_buffer_memory_pool);
    this->m_uniform_upload_buffer_memory_pool->Release();
    this->m_uniform_upload_buffer_memory_pool = NULL;

    assert(NULL != this->m_staging_upload_buffer_memory_pool);
    this->m_staging_upload_buffer_memory_pool->Release();
    this->m_staging_upload_buffer_memory_pool = NULL;

    assert(NULL != this->m_storage_intermediate_buffer_memory_pool);
    this->m_storage_intermediate_buffer_memory_pool->Release();
    this->m_storage_intermediate_buffer_memory_pool = NULL;

    assert(NULL != this->m_storage_asset_buffer_memory_pool);
    this->m_storage_asset_buffer_memory_pool->Release();
    this->m_storage_asset_buffer_memory_pool = NULL;

    assert(NULL != this->m_color_attachment_intermediate_image_memory_pool);
    this->m_color_attachment_intermediate_image_memory_pool->Release();
    this->m_color_attachment_intermediate_image_memory_pool = NULL;

    assert(NULL != this->m_depth_stencil_attachment_intermediate_image_memory_pool);
    this->m_depth_stencil_attachment_intermediate_image_memory_pool->Release();
    this->m_depth_stencil_attachment_intermediate_image_memory_pool = NULL;

    assert(NULL != this->m_storage_intermediate_image_memory_pool);
    this->m_storage_intermediate_image_memory_pool->Release();
    this->m_storage_intermediate_image_memory_pool = NULL;

    assert(NULL != this->m_sampled_asset_image_memory_pool);
    this->m_sampled_asset_image_memory_pool->Release();
    this->m_sampled_asset_image_memory_pool = NULL;

    if (this->m_support_ray_tracing)
    {
        assert(NULL != this->m_scratch_buffer_memory_pool);
        this->m_scratch_buffer_memory_pool->Release();
        this->m_scratch_buffer_memory_pool = NULL;

        assert(NULL != this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
        this->m_intermediate_bottom_level_acceleration_structure_memory_pool->Release();
        this->m_intermediate_bottom_level_acceleration_structure_memory_pool = NULL;

        assert(NULL != this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
        this->m_non_compacted_bottom_level_acceleration_structure_memory_pool->Release();
        this->m_non_compacted_bottom_level_acceleration_structure_memory_pool = NULL;

        assert(NULL != this->m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool);
        this->m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool->Release();
        this->m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool = NULL;

        assert(NULL != this->m_compacted_bottom_level_acceleration_structure_memory_pool);
        this->m_compacted_bottom_level_acceleration_structure_memory_pool->Release();
        this->m_compacted_bottom_level_acceleration_structure_memory_pool = NULL;

        assert(NULL != this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
        this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool->Release();
        this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool = NULL;

        assert(NULL != this->m_top_level_acceleration_structure_memory_pool);
        this->m_top_level_acceleration_structure_memory_pool->Release();
        this->m_top_level_acceleration_structure_memory_pool = NULL;
    }

    assert(NULL != this->m_memory_allocator);
    this->m_memory_allocator->Release();
    this->m_memory_allocator = NULL;

    assert(NULL != this->m_graphics_queue);
    this->m_graphics_queue->Release();
    this->m_graphics_queue = NULL;

    if ((!this->m_uma) || this->m_support_ray_tracing)
    {
        this->m_upload_queue->Release();
        this->m_upload_queue = NULL;
    }
    else
    {
        assert(NULL == this->m_upload_queue);
    }

    assert(NULL != this->m_device);
    this->m_device->Release();
    this->m_device = NULL;

    assert(NULL != this->m_adapter);
    this->m_adapter->Release();
    this->m_adapter = NULL;

    assert(NULL != this->m_factory);
    this->m_factory->Release();
    this->m_factory = NULL;

    this->m_pfn_d3d12_serialize_root_signature = NULL;
}

brx_pal_d3d12_device::~brx_pal_d3d12_device()
{
    assert(NULL == this->m_pfn_d3d12_serialize_root_signature);
    assert(NULL == this->m_factory);
    assert(NULL == this->m_adapter);
    assert(NULL == this->m_device);
    assert(NULL == this->m_memory_allocator);
    assert(NULL == this->m_uniform_upload_buffer_memory_pool);
    assert(NULL == this->m_staging_upload_buffer_memory_pool);
    assert(NULL == this->m_storage_intermediate_buffer_memory_pool);
    assert(NULL == this->m_storage_asset_buffer_memory_pool);
    assert(NULL == this->m_color_attachment_intermediate_image_memory_pool);
    assert(NULL == this->m_depth_stencil_attachment_intermediate_image_memory_pool);
    assert(NULL == this->m_storage_intermediate_image_memory_pool);
    assert(NULL == this->m_sampled_asset_image_memory_pool);
    assert(NULL == this->m_scratch_buffer_memory_pool);
    assert(NULL == this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
    assert(NULL == this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
    assert(NULL == this->m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool);
    assert(NULL == this->m_compacted_bottom_level_acceleration_structure_memory_pool);
    assert(NULL == this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
    assert(NULL == this->m_top_level_acceleration_structure_memory_pool);
}

BRX_PAL_BACKEND_NAME brx_pal_d3d12_device::get_backend_name() const
{
    return BRX_PAL_BACKEND_NAME_D3D12;
}

bool brx_pal_d3d12_device::is_ray_tracing_supported() const
{
    return this->m_support_ray_tracing;
}

brx_pal_graphics_queue *brx_pal_d3d12_device::create_graphics_queue() const
{
    void *new_unwrapped_graphics_queue_base = mcrt_malloc(sizeof(brx_pal_d3d12_graphics_queue), alignof(brx_pal_d3d12_graphics_queue));
    assert(NULL != new_unwrapped_graphics_queue_base);

    brx_pal_d3d12_graphics_queue *new_unwrapped_graphics_queue = new (new_unwrapped_graphics_queue_base) brx_pal_d3d12_graphics_queue{};
    new_unwrapped_graphics_queue->init(this->m_graphics_queue, this->m_uma, this->m_support_ray_tracing);
    return new_unwrapped_graphics_queue;
}

void brx_pal_d3d12_device::destroy_graphics_queue(brx_pal_graphics_queue *wrapped_graphics_queue) const
{
    assert(NULL != wrapped_graphics_queue);
    brx_pal_d3d12_graphics_queue *delete_unwrapped_graphics_queue = static_cast<brx_pal_d3d12_graphics_queue *>(wrapped_graphics_queue);

    delete_unwrapped_graphics_queue->uninit(this->m_graphics_queue);

    delete_unwrapped_graphics_queue->~brx_pal_d3d12_graphics_queue();
    mcrt_free(delete_unwrapped_graphics_queue);
}

brx_pal_upload_queue *brx_pal_d3d12_device::create_upload_queue() const
{
    void *new_unwrapped_upload_queue_base = mcrt_malloc(sizeof(brx_pal_d3d12_upload_queue), alignof(brx_pal_d3d12_upload_queue));
    assert(NULL != new_unwrapped_upload_queue_base);

    brx_pal_d3d12_upload_queue *new_unwrapped_upload_queue = new (new_unwrapped_upload_queue_base) brx_pal_d3d12_upload_queue{};
    new_unwrapped_upload_queue->init(this->m_upload_queue, this->m_uma, this->m_support_ray_tracing);
    return new_unwrapped_upload_queue;
}

void brx_pal_d3d12_device::destroy_upload_queue(brx_pal_upload_queue *wrapped_upload_queue) const
{
    assert(NULL != wrapped_upload_queue);
    brx_pal_d3d12_upload_queue *delete_unwrapped_upload_queue = static_cast<brx_pal_d3d12_upload_queue *>(wrapped_upload_queue);

    delete_unwrapped_upload_queue->uninit(this->m_upload_queue);

    delete_unwrapped_upload_queue->~brx_pal_d3d12_upload_queue();
    mcrt_free(delete_unwrapped_upload_queue);
}

brx_pal_graphics_command_buffer *brx_pal_d3d12_device::create_graphics_command_buffer() const
{
    void *new_unwrapped_graphics_command_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_graphics_command_buffer), alignof(brx_pal_d3d12_graphics_command_buffer));
    assert(NULL != new_unwrapped_graphics_command_buffer_base);

    brx_pal_d3d12_graphics_command_buffer *new_unwrapped_graphics_command_buffer = new (new_unwrapped_graphics_command_buffer_base) brx_pal_d3d12_graphics_command_buffer{};
    new_unwrapped_graphics_command_buffer->init(this->m_device, this->m_uma, this->m_support_ray_tracing, const_cast<brx_pal_d3d12_descriptor_allocator *>(&this->m_descriptor_allocator));
    return new_unwrapped_graphics_command_buffer;
}

void brx_pal_d3d12_device::reset_graphics_command_buffer(brx_pal_graphics_command_buffer *graphics_command_buffer) const
{
    assert(NULL != graphics_command_buffer);

    ID3D12CommandAllocator *command_allocator = static_cast<brx_pal_d3d12_graphics_command_buffer *>(graphics_command_buffer)->get_command_allocator();

    HRESULT hr_reset = command_allocator->Reset();
    assert(SUCCEEDED(hr_reset));
}

void brx_pal_d3d12_device::destroy_graphics_command_buffer(brx_pal_graphics_command_buffer *wrapped_graphics_command_buffer) const
{
    assert(NULL != wrapped_graphics_command_buffer);
    brx_pal_d3d12_graphics_command_buffer *delete_unwrapped_graphics_command_buffer = static_cast<brx_pal_d3d12_graphics_command_buffer *>(wrapped_graphics_command_buffer);

    delete_unwrapped_graphics_command_buffer->uninit();

    delete_unwrapped_graphics_command_buffer->~brx_pal_d3d12_graphics_command_buffer();
    mcrt_free(delete_unwrapped_graphics_command_buffer);
}

brx_pal_upload_command_buffer *brx_pal_d3d12_device::create_upload_command_buffer() const
{
    void *new_unwrapped_upload_command_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_upload_command_buffer), alignof(brx_pal_d3d12_upload_command_buffer));
    assert(NULL != new_unwrapped_upload_command_buffer_base);

    brx_pal_d3d12_upload_command_buffer *new_unwrapped_upload_command_buffer = new (new_unwrapped_upload_command_buffer_base) brx_pal_d3d12_upload_command_buffer{};
    new_unwrapped_upload_command_buffer->init(this->m_device, this->m_uma, this->m_support_ray_tracing);
    return new_unwrapped_upload_command_buffer;
}

void brx_pal_d3d12_device::reset_upload_command_buffer(brx_pal_upload_command_buffer *upload_command_buffer) const
{
    assert(NULL != upload_command_buffer);

    ID3D12CommandAllocator *command_allocator = static_cast<brx_pal_d3d12_upload_command_buffer *>(upload_command_buffer)->get_command_allocator();

    if ((!this->m_uma) || this->m_support_ray_tracing)
    {
        HRESULT hr_reset = command_allocator->Reset();
        assert(SUCCEEDED(hr_reset));
    }
    else
    {
        assert(NULL == command_allocator);
    }
}

void brx_pal_d3d12_device::destroy_upload_command_buffer(brx_pal_upload_command_buffer *wrapped_upload_command_buffer) const
{
    assert(NULL != wrapped_upload_command_buffer);
    brx_pal_d3d12_upload_command_buffer *delete_unwrapped_upload_command_buffer = static_cast<brx_pal_d3d12_upload_command_buffer *>(wrapped_upload_command_buffer);

    delete_unwrapped_upload_command_buffer->uninit();

    delete_unwrapped_upload_command_buffer->~brx_pal_d3d12_upload_command_buffer();
    mcrt_free(delete_unwrapped_upload_command_buffer);
}

brx_pal_fence *brx_pal_d3d12_device::create_fence(bool signaled) const
{
    ID3D12Fence *new_fence = NULL;
    {
        HRESULT hr_create_fence = this->m_device->CreateFence(1U, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&new_fence));
        assert(SUCCEEDED(hr_create_fence));

        HRESULT hr_signal = new_fence->Signal(signaled ? 1U : 0U);
        assert(SUCCEEDED(hr_signal));

        assert((signaled ? 1U : 0U) == new_fence->GetCompletedValue());
    }

    void *new_brx_pal_fence_base = mcrt_malloc(sizeof(brx_pal_d3d12_fence), alignof(brx_pal_d3d12_fence));
    assert(NULL != new_brx_pal_fence_base);

    brx_pal_d3d12_fence *new_brx_pal_fence = new (new_brx_pal_fence_base) brx_pal_d3d12_fence{new_fence};
    return new_brx_pal_fence;
}

void brx_pal_d3d12_device::wait_for_fence(brx_pal_fence *brx_pal_fence) const
{
    assert(NULL != brx_pal_fence);
    ID3D12Fence *fence = static_cast<brx_pal_d3d12_fence *>(brx_pal_fence)->get_fence();

    while (0U == fence->GetCompletedValue())
    {
        SwitchToThread();
    }

    assert(1U == fence->GetCompletedValue());
}

void brx_pal_d3d12_device::reset_fence(brx_pal_fence *brx_pal_fence) const
{
    assert(NULL != brx_pal_fence);
    ID3D12Fence *fence = static_cast<brx_pal_d3d12_fence *>(brx_pal_fence)->get_fence();

    HRESULT hr_signal = fence->Signal(0U);
    assert(SUCCEEDED(hr_signal));

    assert(0U == fence->GetCompletedValue());
}

void brx_pal_d3d12_device::destroy_fence(brx_pal_fence *brx_pal_fence) const
{
    assert(NULL != brx_pal_fence);
    brx_pal_d3d12_fence *delete_fence = static_cast<brx_pal_d3d12_fence *>(brx_pal_fence);

    ID3D12Fence *stealed_fence = NULL;
    delete_fence->steal(&stealed_fence);

    delete_fence->~brx_pal_d3d12_fence();
    mcrt_free(delete_fence);

    stealed_fence->Release();
}

brx_pal_descriptor_set_layout *brx_pal_d3d12_device::create_descriptor_set_layout(uint32_t descriptor_set_binding_count, BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const *descriptor_set_bindings) const
{
    void *new_unwrapped_descriptor_set_layout_base = mcrt_malloc(sizeof(brx_pal_d3d12_descriptor_set_layout), alignof(brx_pal_d3d12_descriptor_set_layout));
    assert(NULL != new_unwrapped_descriptor_set_layout_base);

    brx_pal_d3d12_descriptor_set_layout *new_unwrapped_descriptor_set_layout = new (new_unwrapped_descriptor_set_layout_base) brx_pal_d3d12_descriptor_set_layout{};
    new_unwrapped_descriptor_set_layout->init(descriptor_set_binding_count, descriptor_set_bindings);
    return new_unwrapped_descriptor_set_layout;
}

void brx_pal_d3d12_device::destroy_descriptor_set_layout(brx_pal_descriptor_set_layout *wrapped_descriptor_set_layout) const
{
    assert(NULL != wrapped_descriptor_set_layout);
    brx_pal_d3d12_descriptor_set_layout *delete_unwrapped_descriptor_set_layout = static_cast<brx_pal_d3d12_descriptor_set_layout *>(wrapped_descriptor_set_layout);

    delete_unwrapped_descriptor_set_layout->uninit();

    delete_unwrapped_descriptor_set_layout->~brx_pal_d3d12_descriptor_set_layout();
    mcrt_free(delete_unwrapped_descriptor_set_layout);
}

brx_pal_pipeline_layout *brx_pal_d3d12_device::create_pipeline_layout(uint32_t descriptor_set_layout_count, brx_pal_descriptor_set_layout const *const *descriptor_set_layouts) const
{
    void *new_unwrapped_pipeline_layout_base = mcrt_malloc(sizeof(brx_pal_d3d12_pipeline_layout), alignof(brx_pal_d3d12_pipeline_layout));
    assert(NULL != new_unwrapped_pipeline_layout_base);

    brx_pal_d3d12_pipeline_layout *new_unwrapped_pipeline_layout = new (new_unwrapped_pipeline_layout_base) brx_pal_d3d12_pipeline_layout{};
    new_unwrapped_pipeline_layout->init(this->m_device, this->m_pfn_d3d12_serialize_root_signature, descriptor_set_layout_count, descriptor_set_layouts);
    return new_unwrapped_pipeline_layout;
}

void brx_pal_d3d12_device::destroy_pipeline_layout(brx_pal_pipeline_layout *wrapped_pipeline_layout) const
{
    assert(NULL != wrapped_pipeline_layout);
    brx_pal_d3d12_pipeline_layout *delete_unwrapped_pipeline_layout = static_cast<brx_pal_d3d12_pipeline_layout *>(wrapped_pipeline_layout);

    delete_unwrapped_pipeline_layout->uninit();

    delete_unwrapped_pipeline_layout->~brx_pal_d3d12_pipeline_layout();
    mcrt_free(delete_unwrapped_pipeline_layout);
}

brx_pal_descriptor_set *brx_pal_d3d12_device::create_descriptor_set(brx_pal_descriptor_set_layout const *descriptor_set_layout, uint32_t unbounded_descriptor_count)
{
    void *new_unwrapped_descriptor_set_base = mcrt_malloc(sizeof(brx_pal_d3d12_descriptor_set), alignof(brx_pal_d3d12_descriptor_set));
    assert(NULL != new_unwrapped_descriptor_set_base);

    brx_pal_d3d12_descriptor_set *new_unwrapped_descriptor_set = new (new_unwrapped_descriptor_set_base) brx_pal_d3d12_descriptor_set{};
    new_unwrapped_descriptor_set->init(&this->m_descriptor_allocator, descriptor_set_layout, unbounded_descriptor_count);
    return new_unwrapped_descriptor_set;
}

void brx_pal_d3d12_device::write_descriptor_set(brx_pal_descriptor_set *wrapped_descriptor_set, uint32_t dst_binding, BRX_PAL_DESCRIPTOR_TYPE descriptor_type, uint32_t dst_descriptor_start_index, uint32_t src_descriptor_count, brx_pal_uniform_upload_buffer const *const *src_dynamic_uniform_buffers, uint32_t const *src_dynamic_uniform_buffer_ranges, brx_pal_read_only_storage_buffer const *const *src_read_only_storage_buffers, brx_pal_storage_buffer const *const *src_storage_buffers, brx_pal_sampled_image const *const *src_sampled_images, brx_pal_storage_image const *const *src_storage_images, brx_pal_sampler const *const *src_samplers, brx_pal_top_level_acceleration_structure const *const *src_top_level_acceleration_structures)
{
    assert(NULL != wrapped_descriptor_set);
    brx_pal_d3d12_descriptor_set *const unwrapped_descriptor_set = static_cast<brx_pal_d3d12_descriptor_set *>(wrapped_descriptor_set);

    unwrapped_descriptor_set->write_descriptor(this->m_device, &this->m_descriptor_allocator, dst_binding, descriptor_type, dst_descriptor_start_index, src_descriptor_count, src_dynamic_uniform_buffers, src_dynamic_uniform_buffer_ranges, src_read_only_storage_buffers, src_storage_buffers, src_sampled_images, src_storage_images, src_samplers, src_top_level_acceleration_structures);
}

void brx_pal_d3d12_device::destroy_descriptor_set(brx_pal_descriptor_set *wrapped_descriptor_set)
{
    assert(NULL != wrapped_descriptor_set);
    brx_pal_d3d12_descriptor_set *delete_unwrapped_descriptor_set = static_cast<brx_pal_d3d12_descriptor_set *>(wrapped_descriptor_set);

    delete_unwrapped_descriptor_set->uninit(&this->m_descriptor_allocator);

    delete_unwrapped_descriptor_set->~brx_pal_d3d12_descriptor_set();
    mcrt_free(delete_unwrapped_descriptor_set);
}

brx_pal_render_pass *brx_pal_d3d12_device::create_render_pass(uint32_t color_attachment_count, BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const *color_attachments, BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT const *depth_stencil_attachment) const
{
    mcrt_vector<BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT> new_color_attachment_formats;
    mcrt_vector<uint32_t> new_color_attachment_clear_indices;
    mcrt_vector<uint32_t> new_color_attachment_flush_for_sampled_image_indices;
    mcrt_vector<uint32_t> new_color_attachment_flush_for_present_indices;
    mcrt_vector<BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT> new_depth_stencil_attachment_format;
    bool new_depth_stencil_attachment_clear = false;
    bool new_depth_stencil_attachment_flush_for_sampled_image = false;
    {
        for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
        {
            new_color_attachment_formats.push_back(color_attachments[color_attachment_index].format);

            switch (color_attachments[color_attachment_index].load_operation)
            {
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_DONT_CARE:
                // Do Nothing
                break;
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR:
                new_color_attachment_clear_indices.push_back(color_attachment_index);
                break;
            default:
                assert(false);
            }

            switch (color_attachments[color_attachment_index].store_operation)
            {
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_DONT_CARE:
                // Do Nothing
                break;
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE:
                new_color_attachment_flush_for_sampled_image_indices.push_back(color_attachment_index);
                break;
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_PRESENT:
                new_color_attachment_flush_for_present_indices.push_back(color_attachment_index);
                break;
            default:
                assert(false);
            }
        }

        if (NULL != depth_stencil_attachment)
        {
            new_depth_stencil_attachment_format.push_back(depth_stencil_attachment->format);

            switch (depth_stencil_attachment->load_operation)
            {
            case BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_LOAD_OPERATION_DONT_CARE:
                // Do Nothing
                break;
            case BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_LOAD_OPERATION_CLEAR:
                new_depth_stencil_attachment_clear = true;
                break;
            default:
                assert(false);
            }

            switch (depth_stencil_attachment->store_operation)
            {
            case BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_STORE_OPERATION_DONT_CARE:
                // Do Nothing
                break;
            case BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE:
                new_depth_stencil_attachment_flush_for_sampled_image = true;
                break;
            default:
                assert(false);
            }
        }
        else
        {
            assert(0U == new_depth_stencil_attachment_format.size());
            assert(false == new_depth_stencil_attachment_clear);
            assert(false == new_depth_stencil_attachment_flush_for_sampled_image);
        }
    }

    void *new_brx_pal_render_pass_base = mcrt_malloc(sizeof(brx_pal_d3d12_render_pass), alignof(brx_pal_d3d12_render_pass));
    assert(NULL != new_brx_pal_render_pass_base);

    brx_pal_d3d12_render_pass *new_brx_pal_render_pass = new (new_brx_pal_render_pass_base) brx_pal_d3d12_render_pass{
        std::move(new_color_attachment_formats),
        std::move(new_color_attachment_clear_indices),
        std::move(new_color_attachment_flush_for_sampled_image_indices),
        std::move(new_color_attachment_flush_for_present_indices),
        std::move(new_depth_stencil_attachment_format),
        new_depth_stencil_attachment_clear,
        new_depth_stencil_attachment_flush_for_sampled_image};
    return new_brx_pal_render_pass;
}

void brx_pal_d3d12_device::destroy_render_pass(brx_pal_render_pass *brx_pal_render_pass) const
{
    assert(NULL != brx_pal_render_pass);
    brx_pal_d3d12_render_pass *delete_render_pass = static_cast<brx_pal_d3d12_render_pass *>(brx_pal_render_pass);

    delete_render_pass->~brx_pal_d3d12_render_pass();
    mcrt_free(delete_render_pass);
}

brx_pal_graphics_pipeline *brx_pal_d3d12_device::create_graphics_pipeline(brx_pal_render_pass const *render_pass, brx_pal_pipeline_layout const *pipeline_layout, size_t vertex_shader_module_code_size, void const *vertex_shader_module_code, size_t fragment_shader_module_code_size, void const *fragment_shader_module_code, bool enable_back_face_cull, bool front_ccw, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION depth_compare_operation, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION blend_operation) const
{
    void *new_unwrapped_graphics_pipeline_base = mcrt_malloc(sizeof(brx_pal_d3d12_graphics_pipeline), alignof(brx_pal_d3d12_graphics_pipeline));
    assert(NULL != new_unwrapped_graphics_pipeline_base);

    brx_pal_d3d12_graphics_pipeline *new_unwrapped_graphics_pipeline = new (new_unwrapped_graphics_pipeline_base) brx_pal_d3d12_graphics_pipeline{};
    new_unwrapped_graphics_pipeline->init(this->m_device, render_pass, pipeline_layout, vertex_shader_module_code_size, vertex_shader_module_code, fragment_shader_module_code_size, fragment_shader_module_code, enable_back_face_cull, front_ccw, depth_compare_operation, blend_operation);
    return new_unwrapped_graphics_pipeline;
}

void brx_pal_d3d12_device::destroy_graphics_pipeline(brx_pal_graphics_pipeline *wrapped_graphics_pipeline) const
{
    assert(NULL != wrapped_graphics_pipeline);
    brx_pal_d3d12_graphics_pipeline *delete_unwrapped_graphics_pipeline = static_cast<brx_pal_d3d12_graphics_pipeline *>(wrapped_graphics_pipeline);

    delete_unwrapped_graphics_pipeline->uninit();

    delete_unwrapped_graphics_pipeline->~brx_pal_d3d12_graphics_pipeline();
    mcrt_free(delete_unwrapped_graphics_pipeline);
}

brx_pal_compute_pipeline *brx_pal_d3d12_device::create_compute_pipeline(brx_pal_pipeline_layout const *pipeline_layout, size_t compute_shader_module_code_size, void const *compute_shader_module_code) const
{
    void *new_unwrapped_compute_pipeline_base = mcrt_malloc(sizeof(brx_pal_d3d12_compute_pipeline), alignof(brx_pal_d3d12_compute_pipeline));
    assert(NULL != new_unwrapped_compute_pipeline_base);

    brx_pal_d3d12_compute_pipeline *new_unwrapped_compute_pipeline = new (new_unwrapped_compute_pipeline_base) brx_pal_d3d12_compute_pipeline{};
    new_unwrapped_compute_pipeline->init(this->m_device, pipeline_layout, compute_shader_module_code_size, compute_shader_module_code);
    return new_unwrapped_compute_pipeline;
}

void brx_pal_d3d12_device::destroy_compute_pipeline(brx_pal_compute_pipeline *wrapped_compute_pipeline) const
{
    assert(NULL != wrapped_compute_pipeline);
    brx_pal_d3d12_compute_pipeline *delete_unwrapped_compute_pipeline = static_cast<brx_pal_d3d12_compute_pipeline *>(wrapped_compute_pipeline);

    delete_unwrapped_compute_pipeline->uninit();

    delete_unwrapped_compute_pipeline->~brx_pal_d3d12_compute_pipeline();
    mcrt_free(delete_unwrapped_compute_pipeline);
}

brx_pal_frame_buffer *brx_pal_d3d12_device::create_frame_buffer(brx_pal_render_pass const *brx_pal_render_pass, uint32_t width, uint32_t height, uint32_t color_attachment_count, brx_pal_color_attachment_image const *const *color_attachments, brx_pal_depth_stencil_attachment_image const *depth_stencil_attachment) const
{
    assert(NULL != brx_pal_render_pass);
    assert(static_cast<brx_pal_d3d12_render_pass const *>(brx_pal_render_pass)->get_color_attachment_count() == color_attachment_count);
    assert((NULL != static_cast<brx_pal_d3d12_render_pass const *>(brx_pal_render_pass)->get_depth_stencil_attachment_format()) == (NULL != depth_stencil_attachment));

    mcrt_vector<ID3D12Resource *> new_render_target_resources;
    mcrt_vector<D3D12_CPU_DESCRIPTOR_HANDLE> new_render_target_view_descriptors;
    ID3D12Resource *new_depth_stencil_resource;
    mcrt_vector<D3D12_CPU_DESCRIPTOR_HANDLE> new_depth_stencil_view_descriptor;
    {
        new_render_target_resources.resize(color_attachment_count);
        new_render_target_view_descriptors.resize(color_attachment_count);
        assert(NULL != color_attachments || 0U == color_attachment_count);
        for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
        {
            assert(NULL != color_attachments[color_attachment_index]);
            new_render_target_resources[color_attachment_index] = static_cast<brx_pal_d3d12_color_attachment_image const *>(color_attachments[color_attachment_index])->get_resource();
            new_render_target_view_descriptors[color_attachment_index] = static_cast<brx_pal_d3d12_color_attachment_image const *>(color_attachments[color_attachment_index])->get_render_target_view_descriptor();
        }

        if (NULL != depth_stencil_attachment)
        {
            new_depth_stencil_resource = static_cast<brx_pal_d3d12_depth_stencil_attachment_image const *>(depth_stencil_attachment)->get_resource();
            new_depth_stencil_view_descriptor.resize(1U);
            new_depth_stencil_view_descriptor[0] = static_cast<brx_pal_d3d12_depth_stencil_attachment_image const *>(depth_stencil_attachment)->get_depth_stencil_view_descriptor();
        }
        else
        {
            new_depth_stencil_resource = NULL;
            assert(0U == new_depth_stencil_view_descriptor.size());
        }
    }

    void *new_brx_pal_frame_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_frame_buffer), alignof(brx_pal_d3d12_frame_buffer));
    assert(NULL != new_brx_pal_frame_buffer_base);

    brx_pal_d3d12_frame_buffer *new_brx_pal_frame_buffer = new (new_brx_pal_frame_buffer_base) brx_pal_d3d12_frame_buffer{std::move(new_render_target_resources), std::move(new_render_target_view_descriptors), new_depth_stencil_resource, std::move(new_depth_stencil_view_descriptor)};
    return new_brx_pal_frame_buffer;
}

void brx_pal_d3d12_device::destroy_frame_buffer(brx_pal_frame_buffer *brx_pal_frame_buffer) const
{
    assert(NULL != brx_pal_frame_buffer);
    brx_pal_d3d12_frame_buffer *delete_frame_buffer = static_cast<brx_pal_d3d12_frame_buffer *>(brx_pal_frame_buffer);

    delete_frame_buffer->~brx_pal_d3d12_frame_buffer();
    mcrt_free(delete_frame_buffer);
}

uint32_t brx_pal_d3d12_device::get_uniform_upload_buffer_offset_alignment() const
{
    return D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
}

brx_pal_uniform_upload_buffer *brx_pal_d3d12_device::create_uniform_upload_buffer(uint32_t size) const
{
    void *new_unwrapped_uniform_upload_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_uniform_upload_buffer), alignof(brx_pal_d3d12_uniform_upload_buffer));
    assert(NULL != new_unwrapped_uniform_upload_buffer_base);

    brx_pal_d3d12_uniform_upload_buffer *new_unwrapped_uniform_upload_buffer = new (new_unwrapped_uniform_upload_buffer_base) brx_pal_d3d12_uniform_upload_buffer{};
    new_unwrapped_uniform_upload_buffer->init(this->m_memory_allocator, this->m_uniform_upload_buffer_memory_pool, size);
    return new_unwrapped_uniform_upload_buffer;
}

void brx_pal_d3d12_device::destroy_uniform_upload_buffer(brx_pal_uniform_upload_buffer *wrapped_uniform_upload_buffer) const
{
    assert(NULL != wrapped_uniform_upload_buffer);
    brx_pal_d3d12_uniform_upload_buffer *delete_unwrapped_uniform_upload_buffer = static_cast<brx_pal_d3d12_uniform_upload_buffer *>(wrapped_uniform_upload_buffer);

    delete_unwrapped_uniform_upload_buffer->uninit();

    delete_unwrapped_uniform_upload_buffer->~brx_pal_d3d12_uniform_upload_buffer();
    mcrt_free(delete_unwrapped_uniform_upload_buffer);
}

uint32_t brx_pal_d3d12_device::get_staging_upload_buffer_offset_alignment() const
{
    return D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
}

uint32_t brx_pal_d3d12_device::get_staging_upload_buffer_row_pitch_alignment() const
{
    return D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
}

brx_pal_staging_upload_buffer *brx_pal_d3d12_device::create_staging_upload_buffer(uint32_t size) const
{
    void *new_unwrapped_staging_upload_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_staging_upload_buffer), alignof(brx_pal_d3d12_staging_upload_buffer));
    assert(NULL != new_unwrapped_staging_upload_buffer_base);

    brx_pal_d3d12_staging_upload_buffer *new_unwrapped_staging_upload_buffer = new (new_unwrapped_staging_upload_buffer_base) brx_pal_d3d12_staging_upload_buffer{};
    new_unwrapped_staging_upload_buffer->init(this->m_memory_allocator, this->m_staging_upload_buffer_memory_pool, size);
    return new_unwrapped_staging_upload_buffer;
}

void brx_pal_d3d12_device::destroy_staging_upload_buffer(brx_pal_staging_upload_buffer *wrapped_staging_upload_buffer) const
{
    assert(NULL != wrapped_staging_upload_buffer);
    brx_pal_d3d12_staging_upload_buffer *delete_unwrapped_staging_upload_buffer = static_cast<brx_pal_d3d12_staging_upload_buffer *>(wrapped_staging_upload_buffer);

    delete_unwrapped_staging_upload_buffer->uninit();

    delete_unwrapped_staging_upload_buffer->~brx_pal_d3d12_staging_upload_buffer();
    mcrt_free(delete_unwrapped_staging_upload_buffer);
}

brx_pal_storage_intermediate_buffer *brx_pal_d3d12_device::create_storage_intermediate_buffer(uint32_t size) const
{
    void *new_unwrapped_storage_intermediate_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_storage_intermediate_buffer), alignof(brx_pal_d3d12_storage_intermediate_buffer));
    assert(NULL != new_unwrapped_storage_intermediate_buffer_base);

    brx_pal_d3d12_storage_intermediate_buffer *new_unwrapped_storage_intermediate_buffer = new (new_unwrapped_storage_intermediate_buffer_base) brx_pal_d3d12_storage_intermediate_buffer{};
    new_unwrapped_storage_intermediate_buffer->init(this->m_memory_allocator, this->m_storage_intermediate_buffer_memory_pool, size);
    return new_unwrapped_storage_intermediate_buffer;
}

void brx_pal_d3d12_device::destroy_storage_intermediate_buffer(brx_pal_storage_intermediate_buffer *wrapped_storage_intermediate_buffer) const
{
    assert(NULL != wrapped_storage_intermediate_buffer);
    brx_pal_d3d12_storage_intermediate_buffer *delete_unwrapped_storage_intermediate_buffer = static_cast<brx_pal_d3d12_storage_intermediate_buffer *>(wrapped_storage_intermediate_buffer);

    delete_unwrapped_storage_intermediate_buffer->uninit();

    delete_unwrapped_storage_intermediate_buffer->~brx_pal_d3d12_storage_intermediate_buffer();
    mcrt_free(delete_unwrapped_storage_intermediate_buffer);
}

brx_pal_storage_asset_buffer *brx_pal_d3d12_device::create_storage_asset_buffer(uint32_t size) const
{
    void *new_unwrapped_storage_asset_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_storage_asset_buffer), alignof(brx_pal_d3d12_storage_asset_buffer));
    assert(NULL != new_unwrapped_storage_asset_buffer_base);

    brx_pal_d3d12_storage_asset_buffer *new_unwrapped_storage_asset_buffer = new (new_unwrapped_storage_asset_buffer_base) brx_pal_d3d12_storage_asset_buffer{};
    new_unwrapped_storage_asset_buffer->init(this->m_uma, this->m_memory_allocator, this->m_storage_asset_buffer_memory_pool, size);
    return new_unwrapped_storage_asset_buffer;
}

void brx_pal_d3d12_device::destroy_storage_asset_buffer(brx_pal_storage_asset_buffer *wrapped_asset_storage_buffer) const
{
    assert(NULL != wrapped_asset_storage_buffer);
    brx_pal_d3d12_storage_asset_buffer *delete_unwrapped_asset_storage_buffer = static_cast<brx_pal_d3d12_storage_asset_buffer *>(wrapped_asset_storage_buffer);

    delete_unwrapped_asset_storage_buffer->uninit();

    delete_unwrapped_asset_storage_buffer->~brx_pal_d3d12_storage_asset_buffer();
    mcrt_free(delete_unwrapped_asset_storage_buffer);
}

BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT brx_pal_d3d12_device::get_depth_attachment_image_format() const
{
    return BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT;
}

BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT brx_pal_d3d12_device::get_depth_stencil_attachment_image_format() const
{
    return BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT_S8_UINT;
}

brx_pal_color_attachment_image *brx_pal_d3d12_device::create_color_attachment_image(BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT color_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image) const
{
    void *new_unwrapped_color_attachment_image_base = mcrt_malloc(sizeof(brx_pal_d3d12_color_attachment_intermediate_image), alignof(brx_pal_d3d12_color_attachment_intermediate_image));
    assert(NULL != new_unwrapped_color_attachment_image_base);

    brx_pal_d3d12_color_attachment_intermediate_image *new_unwrapped_color_attachment_image = new (new_unwrapped_color_attachment_image_base) brx_pal_d3d12_color_attachment_intermediate_image{};
    new_unwrapped_color_attachment_image->init(this->m_device, this->m_memory_allocator, this->m_color_attachment_intermediate_image_memory_pool, color_attachment_image_format, width, height, allow_sampled_image);
    return new_unwrapped_color_attachment_image;
}

void brx_pal_d3d12_device::destroy_color_attachment_image(brx_pal_color_attachment_image *wrapped_color_attachment_image) const
{
    assert(NULL != wrapped_color_attachment_image);
    brx_pal_d3d12_color_attachment_intermediate_image *delete_unwrapped_color_attachment_image = static_cast<brx_pal_d3d12_color_attachment_intermediate_image *>(wrapped_color_attachment_image);

    delete_unwrapped_color_attachment_image->uninit();

    delete_unwrapped_color_attachment_image->~brx_pal_d3d12_color_attachment_intermediate_image();
    mcrt_free(delete_unwrapped_color_attachment_image);
}

brx_pal_depth_stencil_attachment_image *brx_pal_d3d12_device::create_depth_stencil_attachment_image(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT depth_stencil_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image) const
{
    void *new_unwrapped_depth_stencil_attachment_image_base = mcrt_malloc(sizeof(brx_pal_d3d12_depth_stencil_attachment_intermediate_image), alignof(brx_pal_d3d12_depth_stencil_attachment_intermediate_image));
    assert(NULL != new_unwrapped_depth_stencil_attachment_image_base);

    brx_pal_d3d12_depth_stencil_attachment_intermediate_image *new_unwrapped_depth_stencil_attachment_image = new (new_unwrapped_depth_stencil_attachment_image_base) brx_pal_d3d12_depth_stencil_attachment_intermediate_image{};
    new_unwrapped_depth_stencil_attachment_image->init(this->m_device, this->m_memory_allocator, this->m_depth_stencil_attachment_intermediate_image_memory_pool, depth_stencil_attachment_image_format, width, height, allow_sampled_image);
    return new_unwrapped_depth_stencil_attachment_image;
}

void brx_pal_d3d12_device::destroy_depth_stencil_attachment_image(brx_pal_depth_stencil_attachment_image *wrapped_depth_stencil_attachment_image) const
{
    assert(NULL != wrapped_depth_stencil_attachment_image);
    brx_pal_d3d12_depth_stencil_attachment_intermediate_image *delete_unwrapped_depth_stencil_attachment_image = static_cast<brx_pal_d3d12_depth_stencil_attachment_intermediate_image *>(wrapped_depth_stencil_attachment_image);

    delete_unwrapped_depth_stencil_attachment_image->uninit();

    delete_unwrapped_depth_stencil_attachment_image->~brx_pal_d3d12_depth_stencil_attachment_intermediate_image();
    mcrt_free(delete_unwrapped_depth_stencil_attachment_image);
}

brx_pal_storage_image *brx_pal_d3d12_device::create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT wrapped_storage_image_format, uint32_t width, uint32_t height, bool allow_sampled_image) const
{
    DXGI_FORMAT unwrapped_storage_image_format;
    switch (wrapped_storage_image_format)
    {
    case BRX_PAL_STORAGE_IMAGE_FORMAT_R16_SFLOAT:
        unwrapped_storage_image_format = DXGI_FORMAT_R16_FLOAT;
        break;
    case BRX_PAL_STORAGE_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
        unwrapped_storage_image_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        break;
    case BRX_PAL_STORAGE_IMAGE_FORMAT_R32_UINT:
        unwrapped_storage_image_format = DXGI_FORMAT_R32_UINT;
        break;
    default:
        assert(false);
        unwrapped_storage_image_format = static_cast<DXGI_FORMAT>(-1);
    }

    void *new_unwrapped_storage_image_base = mcrt_malloc(sizeof(brx_pal_d3d12_storage_intermediate_image), alignof(brx_pal_d3d12_storage_intermediate_image));
    assert(NULL != new_unwrapped_storage_image_base);

    brx_pal_d3d12_storage_intermediate_image *new_unwrapped_storage_image = new (new_unwrapped_storage_image_base) brx_pal_d3d12_storage_intermediate_image{};
    new_unwrapped_storage_image->init(this->m_memory_allocator, this->m_storage_intermediate_image_memory_pool, unwrapped_storage_image_format, width, height, allow_sampled_image);
    return new_unwrapped_storage_image;
}

void brx_pal_d3d12_device::destroy_storage_image(brx_pal_storage_image *wrapped_storage_image) const
{
    assert(NULL != wrapped_storage_image);
    brx_pal_d3d12_storage_intermediate_image *delete_unwrapped_storage_image = static_cast<brx_pal_d3d12_storage_intermediate_image *>(wrapped_storage_image);

    delete_unwrapped_storage_image->uninit();

    delete_unwrapped_storage_image->~brx_pal_d3d12_storage_intermediate_image();
    mcrt_free(delete_unwrapped_storage_image);
}

bool brx_pal_d3d12_device::is_sampled_asset_image_compression_bc_supported() const
{
    return true;
}

bool brx_pal_d3d12_device::is_sampled_asset_image_compression_astc_supported() const
{
    return false;
}

brx_pal_sampled_asset_image *brx_pal_d3d12_device::create_sampled_asset_image(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT wrapped_sampled_asset_image_format, uint32_t width, uint32_t height, uint32_t mip_levels) const
{
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

    void *new_unwrapped_sampled_asset_image_base = mcrt_malloc(sizeof(brx_pal_d3d12_sampled_asset_image), alignof(brx_pal_d3d12_sampled_asset_image));
    assert(NULL != new_unwrapped_sampled_asset_image_base);

    brx_pal_d3d12_sampled_asset_image *new_unwrapped_sampled_asset_image = new (new_unwrapped_sampled_asset_image_base) brx_pal_d3d12_sampled_asset_image{};
    new_unwrapped_sampled_asset_image->init(this->m_uma, this->m_memory_allocator, this->m_sampled_asset_image_memory_pool, unwrapped_sampled_asset_image_format, width, height, mip_levels);

    return new_unwrapped_sampled_asset_image;
}

void brx_pal_d3d12_device::destroy_sampled_asset_image(brx_pal_sampled_asset_image *wrapped_sampled_asset_image) const
{
    assert(NULL != wrapped_sampled_asset_image);
    brx_pal_d3d12_sampled_asset_image *delete_unwrapped_sampled_asset_image = static_cast<brx_pal_d3d12_sampled_asset_image *>(wrapped_sampled_asset_image);

    delete_unwrapped_sampled_asset_image->uninit();

    delete_unwrapped_sampled_asset_image->~brx_pal_d3d12_sampled_asset_image();
    mcrt_free(delete_unwrapped_sampled_asset_image);
}

brx_pal_sampler *brx_pal_d3d12_device::create_sampler(BRX_PAL_SAMPLER_FILTER wrapped_filter, BRX_PAL_SAMPLER_ADDRESS_MODE wrapped_address_mode) const
{
    D3D12_FILTER unwrapped_filter;
    switch (wrapped_filter)
    {
    case BRX_PAL_SAMPLER_FILTER_NEAREST:
        unwrapped_filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        break;
    case BRX_PAL_SAMPLER_FILTER_LINEAR:
        unwrapped_filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    default:
        assert(false);
        unwrapped_filter = static_cast<D3D12_FILTER>(-1);
    }

    D3D12_TEXTURE_ADDRESS_MODE unwrapped_address_mode;
    switch (wrapped_address_mode)
    {
    case BRX_PAL_SAMPLER_ADDRESS_MODE_WRAP:
        unwrapped_address_mode = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        break;
    case BRX_PAL_SAMPLER_ADDRESS_MODE_CLAMP:
        unwrapped_address_mode = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        break;
    default:
        assert(false);
        unwrapped_address_mode = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(-1);
    }

    D3D12_SAMPLER_DESC sampler_desc = {
        unwrapped_filter,
        unwrapped_address_mode,
        unwrapped_address_mode,
        unwrapped_address_mode,
        0.0F,
        1U,
        D3D12_COMPARISON_FUNC_NEVER,
        {0.0F, 0.0F, 0.0F, 0.0F},
        0.0F,
        1000.0F};

    void *new_brx_pal_sampler_base = mcrt_malloc(sizeof(brx_pal_d3d12_sampler), alignof(brx_pal_d3d12_sampler));
    assert(NULL != new_brx_pal_sampler_base);

    brx_pal_d3d12_sampler *new_brx_pal_sampler = new (new_brx_pal_sampler_base) brx_pal_d3d12_sampler{&sampler_desc};
    return new_brx_pal_sampler;
}

void brx_pal_d3d12_device::destroy_sampler(brx_pal_sampler *brx_pal_sampler) const
{
    assert(NULL != brx_pal_sampler);
    brx_pal_d3d12_sampler *delete_sampler = static_cast<brx_pal_d3d12_sampler *>(brx_pal_sampler);

    delete_sampler->~brx_pal_d3d12_sampler();
    mcrt_free(delete_sampler);
}

brx_pal_surface *brx_pal_d3d12_device::create_surface(void *wsi_window) const
{
    static_assert(sizeof(brx_pal_surface *) == sizeof(HWND), "");
    return static_cast<brx_pal_surface *>(wsi_window);
}

void brx_pal_d3d12_device::destroy_surface(brx_pal_surface *) const
{
    static_assert(sizeof(brx_pal_surface *) == sizeof(HWND), "");
    return;
}

brx_pal_swap_chain *brx_pal_d3d12_device::create_swap_chain(brx_pal_surface *surface) const
{
    assert(NULL != surface);
    static_assert(sizeof(brx_pal_surface *) == sizeof(HWND), "");
    HWND hWnd = reinterpret_cast<HWND>(surface);

    DXGI_FORMAT new_swap_chain_image_format = g_preferred_swap_chain_image_format;
    uint32_t new_swap_chain_image_count = g_preferred_swap_chain_image_count;

    uint32_t new_swap_chain_image_width = -1;
    uint32_t new_swap_chain_image_height = -1;
    {
        RECT rect;

        BOOL res_get_clent_rect = GetClientRect(hWnd, &rect);
        assert(FALSE != res_get_clent_rect);

        new_swap_chain_image_width = rect.right - rect.left;
        new_swap_chain_image_height = rect.bottom - rect.top;
    }

    assert(new_swap_chain_image_width > 0U);
    assert(new_swap_chain_image_height > 0U);

    IDXGISwapChain3 *new_swap_chain = NULL;
    {
        IDXGISwapChain1 *new_swap_chain_1 = NULL;

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{
            new_swap_chain_image_width,
            new_swap_chain_image_height,
            new_swap_chain_image_format,
            FALSE,
            {1U, 0U},
            DXGI_USAGE_RENDER_TARGET_OUTPUT,
            new_swap_chain_image_count,
            DXGI_SCALING_STRETCH,
            DXGI_SWAP_EFFECT_FLIP_DISCARD,
            DXGI_ALPHA_MODE_UNSPECIFIED,
            0U};

        HRESULT hr_create_swap_chain = this->m_factory->CreateSwapChainForHwnd(this->m_graphics_queue, hWnd, &swap_chain_desc, NULL, NULL, &new_swap_chain_1);
        assert(SUCCEEDED(hr_create_swap_chain));

        HRESULT hr_query_interface = new_swap_chain_1->QueryInterface(IID_PPV_ARGS(&new_swap_chain));
        assert(SUCCEEDED(hr_query_interface));

        new_swap_chain_1->Release();
    }

    ID3D12DescriptorHeap *new_swap_chain_rtv_descriptor_heap = NULL;
    {
        D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            new_swap_chain_image_count,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            0U};
        HRESULT hr_create_descriptor_heap = this->m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&new_swap_chain_rtv_descriptor_heap));
        assert(SUCCEEDED(hr_create_descriptor_heap));
    }

    mcrt_vector<brx_pal_d3d12_swap_chain_image> new_swap_chain_images;
    {
        UINT const new_rtv_descriptor_heap_descriptor_increment_size = this->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        D3D12_CPU_DESCRIPTOR_HANDLE const new_rtv_descriptor_heap_cpu_descriptor_handle_start = new_swap_chain_rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart();

        for (uint32_t swap_chain_image_index = 0U; swap_chain_image_index < new_swap_chain_image_count; ++swap_chain_image_index)
        {
            ID3D12Resource *resource = NULL;
            HRESULT hr_get_buffer = new_swap_chain->GetBuffer(swap_chain_image_index, IID_PPV_ARGS(&resource));
            assert(SUCCEEDED(hr_get_buffer));

            D3D12_RENDER_TARGET_VIEW_DESC render_target_view_desc{
                .Format = DXGI_FORMAT_UNKNOWN,
                .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
                .Texture2D = {
                    0U,
                    0U}};

            D3D12_CPU_DESCRIPTOR_HANDLE new_render_target_view_descriptor{new_rtv_descriptor_heap_cpu_descriptor_handle_start.ptr + new_rtv_descriptor_heap_descriptor_increment_size * swap_chain_image_index};

            this->m_device->CreateRenderTargetView(resource, &render_target_view_desc, new_render_target_view_descriptor);

            new_swap_chain_images.emplace_back(resource, new_render_target_view_descriptor);
        }
    }

    void *new_brx_pal_swap_chain_base = mcrt_malloc(sizeof(brx_pal_d3d12_swap_chain), alignof(brx_pal_d3d12_swap_chain));
    assert(NULL != new_brx_pal_swap_chain_base);

    brx_pal_d3d12_swap_chain *new_brx_pal_swap_chain = new (new_brx_pal_swap_chain_base) brx_pal_d3d12_swap_chain{new_swap_chain, new_swap_chain_image_format, new_swap_chain_image_width, new_swap_chain_image_height, new_swap_chain_image_count, new_swap_chain_rtv_descriptor_heap, std::move(new_swap_chain_images)};
    return new_brx_pal_swap_chain;
}

bool brx_pal_d3d12_device::acquire_next_image(brx_pal_graphics_command_buffer *brx_pal_graphics_command_buffer, brx_pal_swap_chain const *brx_pal_swap_chain, uint32_t *out_swap_chain_image_index) const
{
    assert(NULL != brx_pal_graphics_command_buffer);
    assert(NULL != brx_pal_swap_chain);
    assert(NULL != out_swap_chain_image_index);
    IDXGISwapChain3 *swap_chain = static_cast<brx_pal_d3d12_swap_chain const *>(brx_pal_swap_chain)->get_swap_chain();

    // TODO: do we need to wait?

    (*out_swap_chain_image_index) = swap_chain->GetCurrentBackBufferIndex();

    return true;
}

void brx_pal_d3d12_device::destroy_swap_chain(brx_pal_swap_chain *brx_pal_swap_chain) const
{
    assert(NULL != brx_pal_swap_chain);
    brx_pal_d3d12_swap_chain *delete_swap_chain = static_cast<brx_pal_d3d12_swap_chain *>(brx_pal_swap_chain);

    IDXGISwapChain3 *stealed_swap_chain = NULL;
    ID3D12DescriptorHeap *stealed_rtv_descriptor_heap = NULL;
    mcrt_vector<brx_pal_d3d12_swap_chain_image> stealed_images;
    delete_swap_chain->steal(&stealed_swap_chain, &stealed_rtv_descriptor_heap, stealed_images);

    delete_swap_chain->~brx_pal_d3d12_swap_chain();
    mcrt_free(delete_swap_chain);

    uint32_t const stealed_swap_chain_image_count = static_cast<uint32_t>(stealed_images.size());
    for (uint32_t swap_chain_image_index = 0U; swap_chain_image_index < stealed_swap_chain_image_count; ++swap_chain_image_index)
    {
        ID3D12Resource *stealed_resource = NULL;
        stealed_images[swap_chain_image_index].steal(&stealed_resource);

        stealed_resource->Release();
    }

    stealed_swap_chain->Release();
    stealed_rtv_descriptor_heap->Release();
}

brx_pal_scratch_buffer *brx_pal_d3d12_device::create_scratch_buffer(uint32_t size) const
{
    void *new_unwrapped_scratch_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_scratch_buffer), alignof(brx_pal_d3d12_scratch_buffer));
    assert(NULL != new_unwrapped_scratch_buffer_base);

    brx_pal_d3d12_scratch_buffer *new_unwrapped_scratch_buffer = new (new_unwrapped_scratch_buffer_base) brx_pal_d3d12_scratch_buffer{};
    new_unwrapped_scratch_buffer->init(this->m_memory_allocator, this->m_scratch_buffer_memory_pool, size);
    return new_unwrapped_scratch_buffer;
}

void brx_pal_d3d12_device::destroy_scratch_buffer(brx_pal_scratch_buffer *wrapped_scratch_buffer) const
{
    assert(NULL != wrapped_scratch_buffer);
    brx_pal_d3d12_scratch_buffer *delete_unwrapped_scratch_buffer = static_cast<brx_pal_d3d12_scratch_buffer *>(wrapped_scratch_buffer);

    delete_unwrapped_scratch_buffer->uninit();

    delete_unwrapped_scratch_buffer->~brx_pal_d3d12_scratch_buffer();
    mcrt_free(delete_unwrapped_scratch_buffer);
}

void brx_pal_d3d12_device::get_intermediate_bottom_level_acceleration_structure_size(uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *wrapped_bottom_level_acceleration_structure_geometries, uint32_t *intermediate_bottom_level_acceleration_structure_size, uint32_t *build_scratch_size, uint32_t *update_scratch_size) const
{
    assert(NULL != wrapped_bottom_level_acceleration_structure_geometries);
    assert(NULL != intermediate_bottom_level_acceleration_structure_size);
    assert(NULL != build_scratch_size);

    mcrt_vector<D3D12_RAYTRACING_GEOMETRY_DESC> ray_tracing_geometry_descs(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));
    for (uint32_t bottom_level_acceleration_structure_geometry_index = 0U; bottom_level_acceleration_structure_geometry_index < bottom_level_acceleration_structure_geometry_count; ++bottom_level_acceleration_structure_geometry_index)
    {
        BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const &wrapped_bottom_level_acceleration_structure_geometry = wrapped_bottom_level_acceleration_structure_geometries[bottom_level_acceleration_structure_geometry_index];

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

        D3D12_GPU_VIRTUAL_ADDRESS const vertex_position_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.vertex_position_buffer)->get_resource()->GetGPUVirtualAddress();

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

        D3D12_GPU_VIRTUAL_ADDRESS const index_buffer_device_memory_range_base = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.index_buffer)->get_resource()->GetGPUVirtualAddress() : NULL;

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
    assert(bottom_level_acceleration_structure_geometry_count == ray_tracing_geometry_descs.size());

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS const build_ray_tracing_acceleration_structure_inputs = {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
        static_cast<UINT>(ray_tracing_geometry_descs.size()),
        D3D12_ELEMENTS_LAYOUT_ARRAY,
        {.pGeometryDescs = &ray_tracing_geometry_descs[0]}};

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ray_tracing_acceleration_structure_prebuild_info = {
        static_cast<UINT64>(-1),
        static_cast<UINT64>(-1),
        static_cast<UINT64>(-1)};

    this->m_device->GetRaytracingAccelerationStructurePrebuildInfo(&build_ray_tracing_acceleration_structure_inputs, &ray_tracing_acceleration_structure_prebuild_info);

    (*intermediate_bottom_level_acceleration_structure_size) = static_cast<uint32_t>(ray_tracing_acceleration_structure_prebuild_info.ResultDataMaxSizeInBytes);
    (*build_scratch_size) = static_cast<uint32_t>(ray_tracing_acceleration_structure_prebuild_info.ScratchDataSizeInBytes);
    (*update_scratch_size) = static_cast<uint32_t>(ray_tracing_acceleration_structure_prebuild_info.UpdateScratchDataSizeInBytes);
}

brx_pal_intermediate_bottom_level_acceleration_structure *brx_pal_d3d12_device::create_intermediate_bottom_level_acceleration_structure(uint32_t size) const
{
    void *new_unwrapped_intermediate_bottom_level_acceleration_structure_base = mcrt_malloc(sizeof(brx_pal_d3d12_intermediate_bottom_level_acceleration_structure), alignof(brx_pal_d3d12_intermediate_bottom_level_acceleration_structure));
    assert(NULL != new_unwrapped_intermediate_bottom_level_acceleration_structure_base);

    brx_pal_d3d12_intermediate_bottom_level_acceleration_structure *new_unwrapped_intermediate_bottom_level_acceleration_structure = new (new_unwrapped_intermediate_bottom_level_acceleration_structure_base) brx_pal_d3d12_intermediate_bottom_level_acceleration_structure{};
    new_unwrapped_intermediate_bottom_level_acceleration_structure->init(this->m_memory_allocator, this->m_intermediate_bottom_level_acceleration_structure_memory_pool, size);
    return new_unwrapped_intermediate_bottom_level_acceleration_structure;
}

void brx_pal_d3d12_device::destroy_intermediate_bottom_level_acceleration_structure(brx_pal_intermediate_bottom_level_acceleration_structure *wrapped_intermediate_bottom_level_acceleration_structure) const
{
    assert(NULL != wrapped_intermediate_bottom_level_acceleration_structure);
    brx_pal_d3d12_intermediate_bottom_level_acceleration_structure *delete_unwrapped_intermediate_bottom_level_acceleration_structure = static_cast<brx_pal_d3d12_intermediate_bottom_level_acceleration_structure *>(wrapped_intermediate_bottom_level_acceleration_structure);

    delete_unwrapped_intermediate_bottom_level_acceleration_structure->uninit();

    delete_unwrapped_intermediate_bottom_level_acceleration_structure->~brx_pal_d3d12_intermediate_bottom_level_acceleration_structure();
    mcrt_free(delete_unwrapped_intermediate_bottom_level_acceleration_structure);
}

void brx_pal_d3d12_device::get_non_compacted_bottom_level_acceleration_structure_size(uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *wrapped_bottom_level_acceleration_structure_geometries, uint32_t *non_compacted_bottom_level_acceleration_structure_size, uint32_t *build_scratch_size) const
{
    assert(NULL != wrapped_bottom_level_acceleration_structure_geometries);
    assert(NULL != non_compacted_bottom_level_acceleration_structure_size);
    assert(NULL != build_scratch_size);

    mcrt_vector<D3D12_RAYTRACING_GEOMETRY_DESC> ray_tracing_geometry_descs(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));
    for (uint32_t bottom_level_acceleration_structure_geometry_index = 0U; bottom_level_acceleration_structure_geometry_index < bottom_level_acceleration_structure_geometry_count; ++bottom_level_acceleration_structure_geometry_index)
    {
        BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const &wrapped_bottom_level_acceleration_structure_geometry = wrapped_bottom_level_acceleration_structure_geometries[bottom_level_acceleration_structure_geometry_index];

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

        D3D12_GPU_VIRTUAL_ADDRESS const vertex_position_buffer_device_memory_range_base = static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.vertex_position_buffer)->get_resource()->GetGPUVirtualAddress();

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

        D3D12_GPU_VIRTUAL_ADDRESS const index_buffer_device_memory_range_base = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? static_cast<brx_pal_d3d12_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.index_buffer)->get_resource()->GetGPUVirtualAddress() : NULL;

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
    assert(bottom_level_acceleration_structure_geometry_count == ray_tracing_geometry_descs.size());

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS const build_ray_tracing_acceleration_structure_inputs = {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
        static_cast<UINT>(ray_tracing_geometry_descs.size()),
        D3D12_ELEMENTS_LAYOUT_ARRAY,
        {.pGeometryDescs = &ray_tracing_geometry_descs[0]}};

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ray_tracing_acceleration_structure_prebuild_info = {
        static_cast<UINT64>(-1),
        static_cast<UINT64>(-1),
        static_cast<UINT64>(-1)};

    this->m_device->GetRaytracingAccelerationStructurePrebuildInfo(&build_ray_tracing_acceleration_structure_inputs, &ray_tracing_acceleration_structure_prebuild_info);

    (*non_compacted_bottom_level_acceleration_structure_size) = static_cast<uint32_t>(ray_tracing_acceleration_structure_prebuild_info.ResultDataMaxSizeInBytes);
    (*build_scratch_size) = static_cast<uint32_t>(ray_tracing_acceleration_structure_prebuild_info.ScratchDataSizeInBytes);
}

brx_pal_non_compacted_bottom_level_acceleration_structure *brx_pal_d3d12_device::create_non_compacted_bottom_level_acceleration_structure(uint32_t size) const
{
    void *new_unwrapped_non_compacted_bottom_level_acceleration_structure_base = mcrt_malloc(sizeof(brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure), alignof(brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure));
    assert(NULL != new_unwrapped_non_compacted_bottom_level_acceleration_structure_base);

    brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure *new_unwrapped_non_compacted_bottom_level_acceleration_structure = new (new_unwrapped_non_compacted_bottom_level_acceleration_structure_base) brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure{};
    new_unwrapped_non_compacted_bottom_level_acceleration_structure->init(this->m_memory_allocator, this->m_non_compacted_bottom_level_acceleration_structure_memory_pool, size);
    return new_unwrapped_non_compacted_bottom_level_acceleration_structure;
}

void brx_pal_d3d12_device::destroy_non_compacted_bottom_level_acceleration_structure(brx_pal_non_compacted_bottom_level_acceleration_structure *wrapped_non_compacted_bottom_level_acceleration_structure) const
{
    assert(NULL != wrapped_non_compacted_bottom_level_acceleration_structure);
    brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure *delete_unwrapped_non_compacted_bottom_level_acceleration_structure = static_cast<brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure *>(wrapped_non_compacted_bottom_level_acceleration_structure);

    delete_unwrapped_non_compacted_bottom_level_acceleration_structure->uninit();

    delete_unwrapped_non_compacted_bottom_level_acceleration_structure->~brx_pal_d3d12_non_compacted_bottom_level_acceleration_structure();
    mcrt_free(delete_unwrapped_non_compacted_bottom_level_acceleration_structure);
}

brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool *brx_pal_d3d12_device::create_compacted_bottom_level_acceleration_structure_size_query_pool(uint32_t query_count) const
{
    void *new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool_base = mcrt_malloc(sizeof(brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool), alignof(brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool));
    assert(NULL != new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool_base);

    brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool *new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool = new (new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool_base) brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool{};
    new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool->init(this->m_memory_allocator, this->m_compacted_bottom_level_acceleration_structure_size_query_buffer_memory_pool, query_count);
    return new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool;
}

uint32_t brx_pal_d3d12_device::get_compacted_bottom_level_acceleration_structure_size_query_pool_result(brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool const *wrapped_compacted_bottom_level_acceleration_structure_size_query_pool, uint32_t query_index) const
{
    assert(NULL != wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);
    brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool const *const unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool = static_cast<brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool const *>(wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC volatile *const query_pool_memory_range_base = unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool->get_host_memory_range_base();

    return static_cast<uint32_t>(query_pool_memory_range_base[query_index].CompactedSizeInBytes);
}

void brx_pal_d3d12_device::destroy_compacted_bottom_level_acceleration_structure_size_query_pool(brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool *wrapped_compacted_bottom_level_acceleration_structure_size_query_pool) const
{
    assert(NULL != wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);
    brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool *delete_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool = static_cast<brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool *>(wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);

    delete_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool->uninit();

    delete_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool->~brx_pal_d3d12_compacted_bottom_level_acceleration_structure_size_query_pool();
    mcrt_free(delete_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool);
}

brx_pal_compacted_bottom_level_acceleration_structure *brx_pal_d3d12_device::create_compacted_bottom_level_acceleration_structure(uint32_t size) const
{
    void *new_unwrapped_compacted_bottom_level_acceleration_structure_base = mcrt_malloc(sizeof(brx_pal_d3d12_compacted_bottom_level_acceleration_structure), alignof(brx_pal_d3d12_compacted_bottom_level_acceleration_structure));
    assert(NULL != new_unwrapped_compacted_bottom_level_acceleration_structure_base);

    brx_pal_d3d12_compacted_bottom_level_acceleration_structure *new_unwrapped_compacted_bottom_level_acceleration_structure = new (new_unwrapped_compacted_bottom_level_acceleration_structure_base) brx_pal_d3d12_compacted_bottom_level_acceleration_structure{};
    new_unwrapped_compacted_bottom_level_acceleration_structure->init(this->m_memory_allocator, this->m_compacted_bottom_level_acceleration_structure_memory_pool, size);
    return new_unwrapped_compacted_bottom_level_acceleration_structure;
}

void brx_pal_d3d12_device::destroy_compacted_bottom_level_acceleration_structure(brx_pal_compacted_bottom_level_acceleration_structure *wrapped_compacted_bottom_level_acceleration_structure) const
{
    assert(NULL != wrapped_compacted_bottom_level_acceleration_structure);
    brx_pal_d3d12_compacted_bottom_level_acceleration_structure *delete_unwrapped_compacted_bottom_level_acceleration_structure = static_cast<brx_pal_d3d12_compacted_bottom_level_acceleration_structure *>(wrapped_compacted_bottom_level_acceleration_structure);

    delete_unwrapped_compacted_bottom_level_acceleration_structure->uninit();

    delete_unwrapped_compacted_bottom_level_acceleration_structure->~brx_pal_d3d12_compacted_bottom_level_acceleration_structure();
    mcrt_free(delete_unwrapped_compacted_bottom_level_acceleration_structure);
}

brx_pal_top_level_acceleration_structure_instance_upload_buffer *brx_pal_d3d12_device::create_top_level_acceleration_structure_instance_upload_buffer(uint32_t instance_count) const
{
    void *new_unwrapped_top_level_acceleration_structure_instance_upload_buffer_base = mcrt_malloc(sizeof(brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer), alignof(brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer));
    assert(NULL != new_unwrapped_top_level_acceleration_structure_instance_upload_buffer_base);

    brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer *new_unwrapped_top_level_acceleration_structure_instance_upload_buffer = new (new_unwrapped_top_level_acceleration_structure_instance_upload_buffer_base) brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer{};
    new_unwrapped_top_level_acceleration_structure_instance_upload_buffer->init(instance_count, this->m_memory_allocator, this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
    return new_unwrapped_top_level_acceleration_structure_instance_upload_buffer;
}

void brx_pal_d3d12_device::destroy_top_level_acceleration_structure_instance_upload_buffer(brx_pal_top_level_acceleration_structure_instance_upload_buffer *wrapped_top_level_acceleration_structure_instance_upload_buffer) const
{
    assert(NULL != wrapped_top_level_acceleration_structure_instance_upload_buffer);
    brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer *delete_unwrapped_top_level_acceleration_structure_instance_upload_buffer = static_cast<brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer *>(wrapped_top_level_acceleration_structure_instance_upload_buffer);

    delete_unwrapped_top_level_acceleration_structure_instance_upload_buffer->uninit();

    delete_unwrapped_top_level_acceleration_structure_instance_upload_buffer->~brx_pal_d3d12_top_level_acceleration_structure_instance_upload_buffer();
    mcrt_free(delete_unwrapped_top_level_acceleration_structure_instance_upload_buffer);
}

void brx_pal_d3d12_device::get_top_level_acceleration_structure_size(uint32_t top_level_acceleration_structure_instance_count, uint32_t *top_level_acceleration_structure_size, uint32_t *build_scratch_size, uint32_t *update_scratch_size) const
{
    assert(NULL != top_level_acceleration_structure_size);
    assert(NULL != build_scratch_size);
    assert(NULL != update_scratch_size);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS const build_build_ray_tracing_acceleration_structure_inputs = {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE,
        static_cast<UINT>(top_level_acceleration_structure_instance_count),
        D3D12_ELEMENTS_LAYOUT_ARRAY,
        {.InstanceDescs = 0U}};

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO build_ray_tracing_acceleration_structure_prebuild_info = {
        static_cast<UINT64>(-1),
        static_cast<UINT64>(-1),
        static_cast<UINT64>(-1)};

    this->m_device->GetRaytracingAccelerationStructurePrebuildInfo(&build_build_ray_tracing_acceleration_structure_inputs, &build_ray_tracing_acceleration_structure_prebuild_info);

    (*top_level_acceleration_structure_size) = static_cast<uint32_t>(build_ray_tracing_acceleration_structure_prebuild_info.ResultDataMaxSizeInBytes);
    (*build_scratch_size) = static_cast<uint32_t>(build_ray_tracing_acceleration_structure_prebuild_info.ScratchDataSizeInBytes);
    (*update_scratch_size) = static_cast<uint32_t>(build_ray_tracing_acceleration_structure_prebuild_info.UpdateScratchDataSizeInBytes);
}

brx_pal_top_level_acceleration_structure *brx_pal_d3d12_device::create_top_level_acceleration_structure(uint32_t size) const
{
    void *new_unwrapped_top_level_acceleration_structure_base = mcrt_malloc(sizeof(brx_pal_d3d12_top_level_acceleration_structure), alignof(brx_pal_d3d12_top_level_acceleration_structure));
    assert(NULL != new_unwrapped_top_level_acceleration_structure_base);

    brx_pal_d3d12_top_level_acceleration_structure *new_unwrapped_top_level_acceleration_structure = new (new_unwrapped_top_level_acceleration_structure_base) brx_pal_d3d12_top_level_acceleration_structure{};
    new_unwrapped_top_level_acceleration_structure->init(this->m_memory_allocator, this->m_top_level_acceleration_structure_memory_pool, size);
    return new_unwrapped_top_level_acceleration_structure;
}

void brx_pal_d3d12_device::destroy_top_level_acceleration_structure(brx_pal_top_level_acceleration_structure *wrapped_top_level_acceleration_structure) const
{
    assert(NULL != wrapped_top_level_acceleration_structure);
    brx_pal_d3d12_top_level_acceleration_structure *delete_unwrapped_top_level_acceleration_structure = static_cast<brx_pal_d3d12_top_level_acceleration_structure *>(wrapped_top_level_acceleration_structure);

    delete_unwrapped_top_level_acceleration_structure->uninit();

    delete_unwrapped_top_level_acceleration_structure->~brx_pal_d3d12_top_level_acceleration_structure();
    mcrt_free(delete_unwrapped_top_level_acceleration_structure);
}
