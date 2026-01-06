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
#include "../../McRT-Malloc/include/mcrt_malloc.h"
#include <assert.h>
#include <new>

#if defined(__GNUC__)
#if defined(__linux__)
#include <dlfcn.h>
#if defined(__ANDROID__)
#ifndef NDEBUG
#include <android/log.h>
#endif
#else
#ifndef NDEBUG
#include <cstdio>
#endif
#endif
#elif defined(__MACH__)
#ifndef NDEBUG
#include <cstdio>
#endif
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
#else
#error Unknown Compiler
#endif

extern VkPipelineStageFlags const g_graphics_queue_family_graphics_compute_pipeline_shader_write_stages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

extern VkPipelineStageFlags const g_graphics_queue_family_graphics_compute_pipeline_shader_read_stages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

// we use ray query instead of ray tracing pipeline
// VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
extern VkPipelineStageFlags const g_graphics_queue_family_ray_tracing_pipeline_shader_read_stages = 0U;

extern VkPipelineStageFlags const g_graphics_queue_family_acceleration_structure_build_shader_read_stages = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;

#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
static VkBool32 VKAPI_PTR _internal_debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);
#else
// No Validation Layer
#endif
#else
#error "0 or 1"
#endif

static inline uint32_t _internal_find_lowest_memory_type_index(struct VkPhysicalDeviceMemoryProperties const *physical_device_memory_properties, VkDeviceSize memory_requirements_size, uint32_t memory_requirements_memory_type_bits, VkMemoryPropertyFlags required_property_flags);

static inline uint32_t _internal_find_lowest_memory_type_index(struct VkPhysicalDeviceMemoryProperties const *physical_device_memory_properties, VkDeviceSize memory_requirements_size, uint32_t memory_requirements_memory_type_bits, VkMemoryPropertyFlags required_property_flags, VkMemoryPropertyFlags preferred_property_flags);

static inline void _internal_pause();

extern brx_pal_device *brx_pal_create_vk_device(void *wsi_connection, bool support_ray_tracing)
{
    void *new_unwrapped_device_base = mcrt_malloc(sizeof(brx_pal_vk_device), alignof(brx_pal_vk_device));
    assert(NULL != new_unwrapped_device_base);

    brx_pal_vk_device *new_unwrapped_device = new (new_unwrapped_device_base) brx_pal_vk_device{};
    new_unwrapped_device->init(wsi_connection, support_ray_tracing);
    return new_unwrapped_device;
}

brx_pal_vk_device::brx_pal_vk_device()
    : m_pfn_get_instance_proc_addr(NULL),
      m_support_ray_tracing(false),
      m_allocation_callbacks(NULL),
      m_instance(VK_NULL_HANDLE),
#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
      m_message(VK_NULL_HANDLE),
#else
// No Validation Layer
#endif
#else
#error "0 or 1"
#endif
      m_physical_device(VK_NULL_HANDLE),
      m_min_uniform_buffer_offset_alignment(static_cast<uint32_t>(-1)),
      m_min_storage_buffer_offset_alignment(static_cast<uint32_t>(-1)),
      m_optimal_buffer_copy_offset_alignment(static_cast<uint32_t>(-1)),
      m_optimal_buffer_copy_row_pitch_alignment(static_cast<uint32_t>(-1)),
      m_max_per_stage_descriptor_storage_buffers(static_cast<uint32_t>(-1)),
      m_max_per_stage_descriptor_sampled_images(static_cast<uint32_t>(-1)),
      m_max_descriptor_set_storage_buffers(static_cast<uint32_t>(-1)),
      m_max_descriptor_set_sampled_images(static_cast<uint32_t>(-1)),
      m_has_dedicated_upload_queue(false),
      m_graphics_queue_family_index(VK_QUEUE_FAMILY_IGNORED),
      m_upload_queue_family_index(VK_QUEUE_FAMILY_IGNORED),
      m_pfn_get_device_proc_addr(NULL),
      m_physical_device_feature_texture_compression_BC(false),
      m_physical_device_feature_texture_compression_ASTC_LDR(false),
      m_device(VK_NULL_HANDLE),
      m_graphics_queue(VK_NULL_HANDLE),
      m_upload_queue(VK_NULL_HANDLE),
      m_depth_attachment_image_format(VK_FORMAT_UNDEFINED),
      m_depth_stencil_attachment_image_format(VK_FORMAT_UNDEFINED),
      m_depth_attachment_image_format_support_sampled_image(false),
      m_depth_stencil_attachment_image_format_support_sampled_image(false),
      m_memory_allocator(VK_NULL_HANDLE),
      m_uniform_upload_buffer_memory_pool(VK_NULL_HANDLE),
      m_staging_upload_buffer_memory_pool(VK_NULL_HANDLE),
      m_storage_intermediate_buffer_memory_pool(VK_NULL_HANDLE),
      m_storage_asset_buffer_memory_pool(VK_NULL_HANDLE),
      m_color_transient_attachment_image_memory_pool(VK_NULL_HANDLE),
      m_color_attachment_sampled_image_memory_pool(VK_NULL_HANDLE),
      m_depth_transient_attachment_image_memory_pool(VK_NULL_HANDLE),
      m_depth_attachment_sampled_image_memory_pool(VK_NULL_HANDLE),
      m_depth_stencil_transient_attachment_image_memory_pool(VK_NULL_HANDLE),
      m_depth_stencil_attachment_sampled_image_memory_pool(VK_NULL_HANDLE),
      m_storage_intermediate_image_memory_pool(VK_NULL_HANDLE),
      m_sampled_asset_image_memory_pool(VK_NULL_HANDLE),
      m_scratch_buffer_memory_pool(VK_NULL_HANDLE),
      m_intermediate_bottom_level_acceleration_structure_memory_pool(VK_NULL_HANDLE),
      m_non_compacted_bottom_level_acceleration_structure_memory_pool(VK_NULL_HANDLE),
      m_compacted_bottom_level_acceleration_structure_memory_pool(VK_NULL_HANDLE),
      m_top_level_acceleration_structure_instance_upload_buffer_memory_pool(VK_NULL_HANDLE),
      m_top_level_acceleration_structure_memory_pool(VK_NULL_HANDLE),
      m_pfn_wait_for_fences(NULL),
      m_pfn_reset_fences(NULL),
      m_pfn_reset_command_pool(NULL),
      m_pfn_acquire_next_image(NULL),
      m_pfn_create_image_view(NULL),
      m_pfn_destroy_image_view(NULL),
      m_pfn_get_buffer_device_address(NULL),
      m_pfn_create_acceleration_structure(NULL),
      m_pfn_destroy_acceleration_structure(NULL),
      m_pfn_get_acceleration_structure_device_address(NULL),
      m_pfn_get_query_pool_results(NULL) {

      };

void brx_pal_vk_device::init(void *wsi_connection, bool support_ray_tracing)
{
#if defined(__GNUC__)

#if defined(__linux__)
    assert(NULL == this->m_pfn_get_instance_proc_addr);
    this->m_pfn_get_instance_proc_addr = vkGetInstanceProcAddr;
#elif defined(__MACH__)
    assert(NULL == this->m_pfn_get_instance_proc_addr);
    this->m_pfn_get_instance_proc_addr = vkGetInstanceProcAddr;
#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)
    assert(NULL == this->m_pfn_get_instance_proc_addr);
    {
        HMODULE const dynamic_library_vulkan = GetModuleHandleW(L"vulkan-1.dll");
        assert(NULL != dynamic_library_vulkan);

        this->m_pfn_get_instance_proc_addr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(dynamic_library_vulkan, "vkGetInstanceProcAddr"));
    }
#else
#error Unknown Compiler
#endif

    assert(!this->m_support_ray_tracing);
    this->m_support_ray_tracing = support_ray_tracing;

    assert(NULL == this->m_allocation_callbacks);

    uint32_t const vulkan_api_version = VK_API_VERSION_1_0;

    assert(VK_NULL_HANDLE == this->m_instance);
    {
        PFN_vkCreateInstance const pfn_vk_create_instance = reinterpret_cast<PFN_vkCreateInstance>(this->m_pfn_get_instance_proc_addr(VK_NULL_HANDLE, "vkCreateInstance"));
        assert(NULL != pfn_vk_create_instance);

        VkApplicationInfo const application_info = {
            VK_STRUCTURE_TYPE_APPLICATION_INFO,
            NULL,
            "BRX-PAL-VK",
            0,
            "BRX-PAL-VK",
            0,
            vulkan_api_version};

#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
        char const *const enabled_layer_names[] = {
            "VK_LAYER_KHRONOS_validation"};
#else
        // No Validation Layer
#endif
#else
#error "0 or 1"
#endif

        char const *const enabled_extension_names[] = {
#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#else
        // No Validation Layer
#endif
#else
#error "0 or 1"
#endif
            VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(__GNUC__)
#if defined(__linux__)
#if defined(__ANDROID__)
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#else
            VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
#elif defined(__MACH__)
            VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#else
#error Unknown Compiler
#endif
        };

        VkInstanceCreateInfo const instance_create_info = {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            NULL,
            0U,
            &application_info,
#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
            sizeof(enabled_layer_names) / sizeof(enabled_layer_names[0]),
            enabled_layer_names,
#else
            0U,
            NULL,
#endif
#else
#error "0 or 1"
#endif
            sizeof(enabled_extension_names) / sizeof(enabled_extension_names[0]),
            enabled_extension_names};

        // TODO: validation layer will crash on Android
        VkResult const res_create_instance = pfn_vk_create_instance(&instance_create_info, this->m_allocation_callbacks, &this->m_instance);
        assert(VK_SUCCESS == res_create_instance);
    }

    this->m_pfn_get_instance_proc_addr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetInstanceProcAddr"));
    assert(NULL != this->m_pfn_get_instance_proc_addr);

#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
    assert(VK_NULL_HANDLE == this->m_message);
    {
        PFN_vkCreateDebugUtilsMessengerEXT const pfn_create_debug_utils_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateDebugUtilsMessengerEXT"));
        assert(NULL != pfn_create_debug_utils_messenger);

        VkDebugUtilsMessengerCreateInfoEXT const debug_utils_messenger_create_info =
            {
                VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                0U,
                0U,
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                _internal_debug_utils_messenger_callback,
                NULL};

        VkResult const res_create_debug_utils_messenger = pfn_create_debug_utils_messenger(this->m_instance, &debug_utils_messenger_create_info, this->m_allocation_callbacks, &this->m_message);
        assert(VK_SUCCESS == res_create_debug_utils_messenger);
    }
#else
    // No Validation Layer
#endif
#else
#error "0 or 1"
#endif

    assert(VK_NULL_HANDLE == this->m_physical_device);
    assert(static_cast<uint32_t>(-1) == this->m_min_uniform_buffer_offset_alignment);
    assert(static_cast<uint32_t>(-1) == this->m_min_storage_buffer_offset_alignment);
    assert(static_cast<uint32_t>(-1) == this->m_optimal_buffer_copy_offset_alignment);
    assert(static_cast<uint32_t>(-1) == this->m_optimal_buffer_copy_row_pitch_alignment);
    assert(static_cast<uint32_t>(-1) == this->m_max_per_stage_descriptor_storage_buffers);
    assert(static_cast<uint32_t>(-1) == this->m_max_per_stage_descriptor_sampled_images);
    assert(static_cast<uint32_t>(-1) == this->m_max_descriptor_set_storage_buffers);
    assert(static_cast<uint32_t>(-1) == this->m_max_descriptor_set_sampled_images);
    {
        PFN_vkEnumeratePhysicalDevices const pfn_enumerate_physical_devices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkEnumeratePhysicalDevices"));
        assert(NULL != pfn_enumerate_physical_devices);
        PFN_vkGetPhysicalDeviceProperties const pfn_get_physical_device_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceProperties"));
        assert(NULL != pfn_get_physical_device_properties);

        uint32_t physical_device_count = static_cast<uint32_t>(-1);
        VkResult const res_enumerate_physical_devices_1 = pfn_enumerate_physical_devices(this->m_instance, &physical_device_count, NULL);
        assert(VK_SUCCESS == res_enumerate_physical_devices_1 && 0U < physical_device_count);

        mcrt_vector<VkPhysicalDevice> physical_devices(static_cast<size_t>(physical_device_count));

        VkResult const res_enumerate_physical_devices_2 = pfn_enumerate_physical_devices(this->m_instance, &physical_device_count, &physical_devices[0]);
        assert(VK_SUCCESS == res_enumerate_physical_devices_2 && physical_devices.size() == physical_device_count);

        uint32_t gpu_physical_device_index = static_cast<uint32_t>(-1);
        VkDeviceSize gpu_min_uniform_buffer_offset_alignment = static_cast<VkDeviceSize>(-1);
        VkDeviceSize gpu_min_storage_buffer_offset_alignment = static_cast<VkDeviceSize>(-1);
        VkDeviceSize gpu_optimal_buffer_copy_offset_alignment = static_cast<VkDeviceSize>(-1);
        VkDeviceSize gpu_optimal_buffer_copy_row_pitch_alignment = static_cast<VkDeviceSize>(-1);
        uint32_t gpu_max_per_stage_descriptor_storage_buffers = static_cast<uint32_t>(-1);
        uint32_t gpu_max_Per_stage_descriptor_sampled_images = static_cast<uint32_t>(-1);
        uint32_t gpu_max_descriptor_set_storage_buffers = static_cast<uint32_t>(-1);
        uint32_t gpu_max_descriptor_set_sampled_images = static_cast<uint32_t>(-1);
        bool gpu_physical_device_found = false;
        for (uint32_t physical_device_index = 0U; physical_device_index < physical_device_count; ++physical_device_index)
        {
            VkPhysicalDeviceProperties physical_device_properties;
            pfn_get_physical_device_properties(physical_devices[physical_device_index], &physical_device_properties);
            if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == physical_device_properties.deviceType)
            {
                gpu_physical_device_index = physical_device_index;
                gpu_min_uniform_buffer_offset_alignment = physical_device_properties.limits.minUniformBufferOffsetAlignment;
                gpu_min_storage_buffer_offset_alignment = physical_device_properties.limits.minStorageBufferOffsetAlignment;
                gpu_optimal_buffer_copy_offset_alignment = physical_device_properties.limits.optimalBufferCopyOffsetAlignment;
                gpu_optimal_buffer_copy_row_pitch_alignment = physical_device_properties.limits.optimalBufferCopyRowPitchAlignment;
                gpu_max_per_stage_descriptor_storage_buffers = physical_device_properties.limits.maxPerStageDescriptorStorageBuffers;
                gpu_max_Per_stage_descriptor_sampled_images = physical_device_properties.limits.maxPerStageDescriptorSampledImages;
                gpu_max_descriptor_set_storage_buffers = physical_device_properties.limits.maxDescriptorSetStorageBuffers;
                gpu_max_descriptor_set_sampled_images = physical_device_properties.limits.maxDescriptorSetSampledImages;
                assert(physical_device_properties.limits.standardSampleLocations);
                assert(0 != (VK_SAMPLE_COUNT_8_BIT & physical_device_properties.limits.framebufferNoAttachmentsSampleCounts));

                gpu_physical_device_found = true;

                // The discrete gpu is preferred
                break;
            }
            else if (!gpu_physical_device_found)
            {
                // The lower index may imply the user preference (e.g. VK_LAYER_MESA_device_select)
                assert(static_cast<uint32_t>(-1) == gpu_physical_device_index);

                assert(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == physical_device_properties.deviceType);

                gpu_physical_device_index = physical_device_index;
                gpu_min_uniform_buffer_offset_alignment = physical_device_properties.limits.minUniformBufferOffsetAlignment;
                gpu_min_storage_buffer_offset_alignment = physical_device_properties.limits.minStorageBufferOffsetAlignment;
                gpu_optimal_buffer_copy_offset_alignment = physical_device_properties.limits.optimalBufferCopyOffsetAlignment;
                gpu_optimal_buffer_copy_row_pitch_alignment = physical_device_properties.limits.optimalBufferCopyRowPitchAlignment;
                gpu_max_per_stage_descriptor_storage_buffers = physical_device_properties.limits.maxPerStageDescriptorStorageBuffers;
                gpu_max_Per_stage_descriptor_sampled_images = physical_device_properties.limits.maxPerStageDescriptorSampledImages;
                gpu_max_descriptor_set_storage_buffers = physical_device_properties.limits.maxDescriptorSetStorageBuffers;
                gpu_max_descriptor_set_sampled_images = physical_device_properties.limits.maxDescriptorSetSampledImages;
                assert(physical_device_properties.limits.standardSampleLocations);
                assert(0 != (VK_SAMPLE_COUNT_8_BIT & physical_device_properties.limits.framebufferNoAttachmentsSampleCounts));

                gpu_physical_device_found = true;
            }
        }

        if (gpu_physical_device_found)
        {
            this->m_physical_device = physical_devices[gpu_physical_device_index];
            this->m_min_uniform_buffer_offset_alignment = static_cast<uint32_t>(gpu_min_uniform_buffer_offset_alignment);
            this->m_min_storage_buffer_offset_alignment = static_cast<uint32_t>(gpu_min_storage_buffer_offset_alignment);
            this->m_optimal_buffer_copy_offset_alignment = static_cast<uint32_t>(gpu_optimal_buffer_copy_offset_alignment);
            this->m_optimal_buffer_copy_row_pitch_alignment = static_cast<uint32_t>(gpu_optimal_buffer_copy_row_pitch_alignment);
            this->m_max_per_stage_descriptor_storage_buffers = static_cast<uint32_t>(gpu_max_per_stage_descriptor_storage_buffers);
            this->m_max_per_stage_descriptor_sampled_images = static_cast<uint32_t>(gpu_max_Per_stage_descriptor_sampled_images);
            this->m_max_descriptor_set_storage_buffers = static_cast<uint32_t>(gpu_max_descriptor_set_storage_buffers);
            this->m_max_descriptor_set_sampled_images = static_cast<uint32_t>(gpu_max_descriptor_set_sampled_images);
        }
        else
        {
            assert(false);
        }
    }

    // https://github.com/ValveSoftware/dxvk
    // src/dxvk/dxvk_device.h
    // DxvkDevice::hasDedicatedTransferQueue
    assert(false == this->m_has_dedicated_upload_queue);
    // nvpro-samples/shared_sources/nvvk/context_vk.cpp
    // Context::initDevice
    // m_queue_GCT
    // m_queue_CT
    // m_upload_queue
    assert(VK_QUEUE_FAMILY_IGNORED == this->m_graphics_queue_family_index);
    assert(VK_QUEUE_FAMILY_IGNORED == this->m_upload_queue_family_index);
    uint32_t new_graphics_queue_queue_index = static_cast<uint32_t>(-1);
    uint32_t new_upload_queue_queue_index = static_cast<uint32_t>(-1);
    {
        PFN_vkGetPhysicalDeviceQueueFamilyProperties const pfn_vk_get_physical_device_queue_family_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
        assert(NULL != pfn_vk_get_physical_device_queue_family_properties);

        uint32_t queue_family_property_count = static_cast<uint32_t>(-1);
        pfn_vk_get_physical_device_queue_family_properties(this->m_physical_device, &queue_family_property_count, NULL);

        mcrt_vector<VkQueueFamilyProperties> queue_family_properties(static_cast<size_t>(queue_family_property_count));

        pfn_vk_get_physical_device_queue_family_properties(this->m_physical_device, &queue_family_property_count, &queue_family_properties[0]);
        assert(queue_family_properties.size() == queue_family_property_count);

        // TODO: support seperated present queue
        // src/dxvk/dxvk_adapter.cpp
        // DxvkAdapter::findQueueFamilies
        // src/d3d11/d3d11_swapchain.cpp
        // D3D11SwapChain::CreatePresenter
        {
#if defined(__GNUC__)
#if defined(__linux__)
#if defined(__ANDROID__)
            // Android always supported
#else
            PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR const pfn_vk_get_physical_device_xcb_presentation_support = reinterpret_cast<PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR"));
            assert(NULL != pfn_vk_get_physical_device_xcb_presentation_support);

            struct brx_pal_xcb_connection_T
            {
                xcb_connection_t *m_connection;
                xcb_visualid_t m_visual_id;
            };
            brx_pal_xcb_connection_T *const brx_pal_xcb_connection = static_cast<brx_pal_xcb_connection_T *>(wsi_connection);
#endif
#elif defined(__MACH__)
            // metal always supported
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
            PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR const pfn_vk_get_physical_device_win32_presentation_support = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
            assert(NULL != pfn_vk_get_physical_device_win32_presentation_support);
#else
#error Unknown Compiler
#endif

            for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_property_count; ++queue_family_index)
            {
                VkQueueFamilyProperties const &queue_family_property = queue_family_properties[queue_family_index];

                if ((queue_family_property.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
#if defined(__GNUC__)
#if defined(__linux__)
#if defined(__ANDROID__)
                    true
#else
                    pfn_vk_get_physical_device_xcb_presentation_support(this->m_physical_device, queue_family_index, brx_pal_xcb_connection->m_connection, brx_pal_xcb_connection->m_visual_id)

#endif
#elif defined(__MACH__)
                    true
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
                    // According to the "IDXGIFactory::CreateSwapChain", the "direct" (namely, graphics) commmand queue is used to create the swapchain
                    pfn_vk_get_physical_device_win32_presentation_support(this->m_physical_device, queue_family_index)
#else
#error Unknown Compiler
#endif
                )
                {
                    this->m_graphics_queue_family_index = queue_family_index;
                    new_graphics_queue_queue_index = 0U;
                    break;
                }
            }
        }

        // We should have alreadyfound the graphics and present queue
        assert(VK_QUEUE_FAMILY_IGNORED != this->m_graphics_queue_family_index && static_cast<uint32_t>(-1) != new_graphics_queue_queue_index);

        // Find upload queue
        if (!this->m_support_ray_tracing)
        {
            // Find transfer queue
            for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_property_count; ++queue_family_index)
            {
                if ((this->m_graphics_queue_family_index != queue_family_index) && (VK_QUEUE_TRANSFER_BIT == (queue_family_properties[queue_family_index].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))))
                {
                    this->m_upload_queue_family_index = queue_family_index;
                    new_upload_queue_queue_index = 0U;
                    this->m_has_dedicated_upload_queue = true;
                    break;
                }
            }

            // Fallback to other graphics/compute queues
            // By vkspec, "either GRAPHICS or COMPUTE implies TRANSFER". This means TRANSFER is optional.
            if (VK_QUEUE_FAMILY_IGNORED == this->m_upload_queue_family_index)
            {
                for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_property_count; ++queue_family_index)
                {
                    if ((this->m_graphics_queue_family_index != queue_family_index) && (0U != (queue_family_properties[queue_family_index].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))))
                    {
                        this->m_upload_queue_family_index = queue_family_index;
                        new_upload_queue_queue_index = 0U;
                        this->m_has_dedicated_upload_queue = true;
                        break;
                    }
                }
            }

            // Try the same queue family
            if (VK_QUEUE_FAMILY_IGNORED == this->m_upload_queue_family_index)
            {
                if (2U <= queue_family_properties[this->m_graphics_queue_family_index].queueCount)
                {
                    this->m_upload_queue_family_index = this->m_graphics_queue_family_index;
                    new_upload_queue_queue_index = 1U;
                    this->m_has_dedicated_upload_queue = true;
                }
                else
                {
                    this->m_upload_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
                    assert(-1 == new_upload_queue_queue_index);
                    this->m_has_dedicated_upload_queue = false;
                }
            }
        }
        else
        {
            // We need to use the compute queue to build the acceleration structure
            // Actually, the copy command is simulated by compute shader for most GPUs, e.g. PS5
            for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_property_count; ++queue_family_index)
            {
                if ((this->m_graphics_queue_family_index != queue_family_index) && (0U != (queue_family_properties[queue_family_index].queueFlags & VK_QUEUE_COMPUTE_BIT)))
                {
                    this->m_upload_queue_family_index = queue_family_index;
                    new_upload_queue_queue_index = 0U;
                    this->m_has_dedicated_upload_queue = true;
                    break;
                }
            }

            // Fallback to other graphics queues
            if (VK_QUEUE_FAMILY_IGNORED == this->m_upload_queue_family_index)
            {
                for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_property_count; ++queue_family_index)
                {
                    if ((this->m_graphics_queue_family_index != queue_family_index) && (0U != (queue_family_properties[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT)))
                    {
                        this->m_upload_queue_family_index = queue_family_index;
                        new_upload_queue_queue_index = 0U;
                        this->m_has_dedicated_upload_queue = true;
                        break;
                    }
                }
            }

            // Try the same queue family
            if (VK_QUEUE_FAMILY_IGNORED == this->m_upload_queue_family_index)
            {
                if (2U <= queue_family_properties[this->m_graphics_queue_family_index].queueCount)
                {
                    this->m_upload_queue_family_index = this->m_graphics_queue_family_index;
                    new_upload_queue_queue_index = 1U;
                    this->m_has_dedicated_upload_queue = true;
                }
                else
                {
                    this->m_upload_queue_family_index = VK_QUEUE_FAMILY_IGNORED;
                    assert(-1 == new_upload_queue_queue_index);
                    this->m_has_dedicated_upload_queue = false;
                }
            }
        }

        assert(!this->m_has_dedicated_upload_queue || (VK_QUEUE_FAMILY_IGNORED != this->m_upload_queue_family_index && static_cast<uint32_t>(-1) != new_upload_queue_queue_index));
    }

    assert(false == this->m_physical_device_feature_texture_compression_BC);
    assert(false == this->m_physical_device_feature_texture_compression_ASTC_LDR);
    assert(VK_NULL_HANDLE == this->m_device);
    {
        float const graphics_queue_priority = 1.0F;
        float const upload_queue_priority = 1.0F;
        float const graphics_upload_queue_priorities[2] = {graphics_queue_priority, upload_queue_priority};
        VkDeviceQueueCreateInfo device_queue_create_infos[2];
        uint32_t device_queue_create_info_count = static_cast<uint32_t>(-1);
        if (this->m_has_dedicated_upload_queue)
        {
            if (this->m_graphics_queue_family_index != this->m_upload_queue_family_index)
            {
                device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                device_queue_create_infos[0].pNext = NULL;
                device_queue_create_infos[0].flags = 0U;
                device_queue_create_infos[0].queueFamilyIndex = this->m_graphics_queue_family_index;
                assert(0U == new_graphics_queue_queue_index);
                device_queue_create_infos[0].queueCount = 1U;
                device_queue_create_infos[0].pQueuePriorities = &graphics_queue_priority;

                device_queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                device_queue_create_infos[1].pNext = NULL;
                device_queue_create_infos[1].flags = 0U;
                device_queue_create_infos[1].queueFamilyIndex = this->m_upload_queue_family_index;
                assert(0U == new_upload_queue_queue_index);
                device_queue_create_infos[1].queueCount = 1U;
                device_queue_create_infos[1].pQueuePriorities = &upload_queue_priority;

                device_queue_create_info_count = 2U;
            }
            else
            {
                device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                device_queue_create_infos[0].pNext = NULL;
                device_queue_create_infos[0].flags = 0U;
                device_queue_create_infos[0].queueFamilyIndex = this->m_graphics_queue_family_index;
                assert(0U == new_graphics_queue_queue_index);
                assert(1U == new_upload_queue_queue_index);
                device_queue_create_infos[0].queueCount = 2U;
                device_queue_create_infos[0].pQueuePriorities = graphics_upload_queue_priorities;
                device_queue_create_info_count = 1U;
            }
        }
        else
        {
            device_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            device_queue_create_infos[0].pNext = NULL;
            device_queue_create_infos[0].flags = 0U;
            device_queue_create_infos[0].queueFamilyIndex = this->m_graphics_queue_family_index;
            assert(0U == new_graphics_queue_queue_index);
            device_queue_create_infos[0].queueCount = 1U;
            device_queue_create_infos[0].pQueuePriorities = &graphics_queue_priority;
            device_queue_create_info_count = 1U;
        }

        // TODO: VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME

        char const *const enabled_extension_names[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_RAY_QUERY_EXTENSION_NAME,
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};

        uint32_t const enabled_extension_count = !this->m_support_ray_tracing ? 1U : (sizeof(enabled_extension_names) / sizeof(enabled_extension_names[0]));

        PFN_vkGetPhysicalDeviceFeatures const pfn_get_physical_device_features = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceFeatures"));
        assert(NULL != pfn_get_physical_device_features);
        PFN_vkCreateDevice const pfn_create_device = reinterpret_cast<PFN_vkCreateDevice>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateDevice"));
        assert(NULL != pfn_create_device);

        VkPhysicalDeviceFeatures physical_device_supported_features;
        pfn_get_physical_device_features(this->m_physical_device, &physical_device_supported_features);

        this->m_physical_device_feature_texture_compression_BC = (VK_FALSE != physical_device_supported_features.textureCompressionBC) ? true : false;
        // Fallback to ASTC if BC is not supported
        this->m_physical_device_feature_texture_compression_ASTC_LDR = ((!this->m_physical_device_feature_texture_compression_BC) && (VK_FALSE != physical_device_supported_features.textureCompressionASTC_LDR)) ? true : false;
        // we do not need both at the same time
        assert(!(this->m_physical_device_feature_texture_compression_BC && this->m_physical_device_feature_texture_compression_ASTC_LDR));

        VkPhysicalDeviceFeatures const physical_device_enabled_features = {
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            // Independent Blend
            VK_TRUE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            ((this->m_physical_device_feature_texture_compression_ASTC_LDR) ? static_cast<VkBool32>(VK_TRUE) : static_cast<VkBool32>(VK_FALSE)),
            ((this->m_physical_device_feature_texture_compression_BC) ? static_cast<VkBool32>(VK_TRUE) : static_cast<VkBool32>(VK_FALSE)),
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            // Fragment Stores And Atomics
            VK_TRUE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            // Shader Storage Image Write Without Format
            VK_TRUE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
        };

        VkPhysicalDeviceDepthClipEnableFeaturesEXT const physical_device_depth_clip_enable_features =
            {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT,
                NULL,
                VK_TRUE};

        // TODO: VkPhysicalDeviceRayTracingPipelineFeaturesKHR

        VkPhysicalDeviceRayQueryFeaturesKHR const physical_device_ray_query_features =
            {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
                const_cast<VkPhysicalDeviceDepthClipEnableFeaturesEXT *>(&physical_device_depth_clip_enable_features),
                VK_TRUE};

        VkPhysicalDeviceAccelerationStructureFeaturesKHR const physical_device_acceleration_structure_features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
            const_cast<VkPhysicalDeviceRayQueryFeaturesKHR *>(&physical_device_ray_query_features),
            VK_TRUE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE};

        VkPhysicalDeviceBufferDeviceAddressFeaturesKHR const physical_device_buffer_device_address_features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR,
            const_cast<VkPhysicalDeviceAccelerationStructureFeaturesKHR *>(&physical_device_acceleration_structure_features),
            VK_TRUE,
            VK_FALSE,
            VK_FALSE};

        VkPhysicalDeviceDescriptorIndexingFeaturesEXT const physical_device_descriptor_indexing_features = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
            const_cast<VkPhysicalDeviceBufferDeviceAddressFeaturesKHR *>(&physical_device_buffer_device_address_features),
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_TRUE,
            VK_TRUE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_FALSE,
            VK_TRUE,
            VK_TRUE};

        void const *const device_create_info_next = (!this->m_support_ray_tracing) ? static_cast<void const *>(&physical_device_depth_clip_enable_features) : static_cast<void const *>(&physical_device_descriptor_indexing_features);

        VkDeviceCreateInfo const device_create_info = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            device_create_info_next,
            0U,
            device_queue_create_info_count,
            device_queue_create_infos,
            0U,
            NULL,
            enabled_extension_count,
            enabled_extension_names,
            &physical_device_enabled_features};
        VkResult const res_create_device = pfn_create_device(this->m_physical_device, &device_create_info, this->m_allocation_callbacks, &this->m_device);
        assert(VK_SUCCESS == res_create_device);
    }

    assert(NULL == this->m_pfn_get_device_proc_addr);
    this->m_pfn_get_device_proc_addr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetDeviceProcAddr"));
    assert(NULL != this->m_pfn_get_device_proc_addr);
    this->m_pfn_get_device_proc_addr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetDeviceProcAddr"));
    assert(NULL != this->m_pfn_get_device_proc_addr);

    this->m_graphics_queue = VK_NULL_HANDLE;
    this->m_upload_queue = VK_NULL_HANDLE;
    {
        PFN_vkGetDeviceQueue const pfn_get_device_queue = reinterpret_cast<PFN_vkGetDeviceQueue>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetDeviceQueue"));
        assert(NULL != pfn_get_device_queue);

        pfn_get_device_queue(this->m_device, this->m_graphics_queue_family_index, new_graphics_queue_queue_index, &this->m_graphics_queue);

        if (this->m_has_dedicated_upload_queue)
        {
            assert(VK_QUEUE_FAMILY_IGNORED != this->m_upload_queue_family_index);
            assert(static_cast<uint32_t>(-1) != new_upload_queue_queue_index);
            pfn_get_device_queue(this->m_device, this->m_upload_queue_family_index, new_upload_queue_queue_index, &this->m_upload_queue);
        }
    }
    assert(VK_NULL_HANDLE != this->m_graphics_queue);
    assert(!this->m_has_dedicated_upload_queue || VK_NULL_HANDLE != this->m_upload_queue);

    // https://registry.khronos.org/vulkan/specs/1.0/html/chap33.html#features-required-format-support
    // VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT feature must be supported for at least one of
    // VK_FORMAT_X8_D24_UNORM_PACK32 and VK_FORMAT_D32_SFLOAT, and must be supported for at least one of
    // VK_FORMAT_D24_UNORM_S8_UINT and VK_FORMAT_D32_SFLOAT_S8_UINT.

    assert(VK_FORMAT_UNDEFINED == this->m_depth_attachment_image_format);
    assert(VK_FORMAT_UNDEFINED == this->m_depth_stencil_attachment_image_format);
    assert(false == this->m_depth_attachment_image_format_support_sampled_image);
    assert(false == this->m_depth_stencil_attachment_image_format_support_sampled_image);
    {
        PFN_vkGetPhysicalDeviceFormatProperties const pfn_get_physical_device_format_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceFormatProperties"));

        VkFormatProperties format_d32_sfloat_properties;
        pfn_get_physical_device_format_properties(this->m_physical_device, VK_FORMAT_D32_SFLOAT, &format_d32_sfloat_properties);
        VkFormatProperties format_d24_unorm_properties;
        pfn_get_physical_device_format_properties(this->m_physical_device, VK_FORMAT_X8_D24_UNORM_PACK32, &format_d24_unorm_properties);
        if (format_d32_sfloat_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            this->m_depth_attachment_image_format = VK_FORMAT_D32_SFLOAT;
            this->m_depth_attachment_image_format_support_sampled_image = (format_d32_sfloat_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ? true : false;
        }
        else if (format_d24_unorm_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            this->m_depth_attachment_image_format = VK_FORMAT_X8_D24_UNORM_PACK32;
            this->m_depth_attachment_image_format_support_sampled_image = (format_d24_unorm_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ? true : false;
        }
        else
        {
            assert(false);
            this->m_depth_attachment_image_format = VK_FORMAT_UNDEFINED;
            this->m_depth_attachment_image_format_support_sampled_image = false;
        }

        VkFormatProperties format_d32_sfloat_s8_uint_properties;
        pfn_get_physical_device_format_properties(this->m_physical_device, VK_FORMAT_D32_SFLOAT_S8_UINT, &format_d32_sfloat_s8_uint_properties);
        VkFormatProperties format_d24_unorm_s8_uint_properties;
        pfn_get_physical_device_format_properties(this->m_physical_device, VK_FORMAT_D24_UNORM_S8_UINT, &format_d24_unorm_s8_uint_properties);
        if (format_d32_sfloat_s8_uint_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            this->m_depth_stencil_attachment_image_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
            this->m_depth_stencil_attachment_image_format_support_sampled_image = (format_d32_sfloat_s8_uint_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ? true : false;
        }
        else if (format_d24_unorm_s8_uint_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            this->m_depth_stencil_attachment_image_format = VK_FORMAT_D24_UNORM_S8_UINT;
            this->m_depth_stencil_attachment_image_format_support_sampled_image = (format_d24_unorm_s8_uint_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ? true : false;
        }
        else
        {
            assert(false);
            this->m_depth_stencil_attachment_image_format = VK_FORMAT_UNDEFINED;
            this->m_depth_stencil_attachment_image_format_support_sampled_image = false;
        }
    }

    assert(VK_NULL_HANDLE == this->m_memory_allocator);
    {
        VmaVulkanFunctions vulkan_functions = {};
        vulkan_functions.vkGetInstanceProcAddr = this->m_pfn_get_instance_proc_addr;
        vulkan_functions.vkGetDeviceProcAddr = this->m_pfn_get_device_proc_addr;

        VmaAllocatorCreateInfo allocator_create_info = {};
        if (this->m_support_ray_tracing)
        {
            allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }
        allocator_create_info.vulkanApiVersion = vulkan_api_version;
        allocator_create_info.physicalDevice = this->m_physical_device;
        allocator_create_info.device = this->m_device;
        allocator_create_info.instance = this->m_instance;
        allocator_create_info.pAllocationCallbacks = this->m_allocation_callbacks;
        allocator_create_info.pVulkanFunctions = &vulkan_functions;
        vmaCreateAllocator(&allocator_create_info, &this->m_memory_allocator);
    }

    assert(VK_NULL_HANDLE == this->m_uniform_upload_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_staging_upload_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_storage_intermediate_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_storage_asset_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_color_transient_attachment_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_color_attachment_sampled_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_depth_transient_attachment_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_depth_attachment_sampled_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_depth_stencil_transient_attachment_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_depth_stencil_attachment_sampled_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_storage_intermediate_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_sampled_asset_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_scratch_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
    assert(VK_NULL_HANDLE == this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
    assert(VK_NULL_HANDLE == this->m_compacted_bottom_level_acceleration_structure_memory_pool);
    assert(VK_NULL_HANDLE == this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_top_level_acceleration_structure_memory_pool);
    {
        PFN_vkGetPhysicalDeviceMemoryProperties const pfn_get_physical_device_memory_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceMemoryProperties"));
        PFN_vkCreateBuffer const pfn_create_buffer = reinterpret_cast<PFN_vkCreateBuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateBuffer"));
        PFN_vkGetBufferMemoryRequirements const pfn_get_buffer_memory_requirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetBufferMemoryRequirements"));
        PFN_vkDestroyBuffer const pfn_destroy_buffer = reinterpret_cast<PFN_vkDestroyBuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyBuffer"));
        PFN_vkGetPhysicalDeviceFormatProperties const pfn_get_physical_device_format_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceFormatProperties"));
        PFN_vkCreateImage const pfn_create_image = reinterpret_cast<PFN_vkCreateImage>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateImage"));
        PFN_vkGetImageMemoryRequirements const pfn_get_image_memory_requirements = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetImageMemoryRequirements"));
        PFN_vkDestroyImage const pfn_destroy_image = reinterpret_cast<PFN_vkDestroyImage>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyImage"));

        VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
        pfn_get_physical_device_memory_properties(this->m_physical_device, &physical_device_memory_properties);

        // https://www.khronos.org/registry/vulkan/specs/1.0/html/chap13.html#VkMemoryRequirements
        // If buffer is a VkBuffer not created with the VK_BUFFER_CREATE_SPARSE_BINDING_BIT bit set, or if image is linear image,
        // then the memoryTypeBits member always contains at least one bit set corresponding to a VkMemoryType with a
        // propertyFlags that has both the VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit and the
        // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit set. In other words, mappable coherent memory can always be attached to
        // these objects.

        // https://www.khronos.org/registry/vulkan/specs/1.0/html/chap13.html#VkMemoryRequirements
        // The memoryTypeBits member is identical for all VkBuffer objects created with the same value for the flags and usage
        // members in the VkBufferCreateInfo structure passed to vkCreateBuffer. Further, if usage1 and usage2 of type
        // VkBufferUsageFlags are such that the bits set in usage2 are a subset of the bits set in usage1, and they have the same flags,
        // then the bits set in memoryTypeBits returned for usage1 must be a subset of the bits set in memoryTypeBits returned for
        // usage2, for all values of flags.

        // uniform upload buffer
        assert(VK_NULL_HANDLE == this->m_uniform_upload_buffer_memory_pool);
        {
            uint32_t uniform_upload_buffer_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = static_cast<VkDeviceSize>(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkBufferCreateInfo const buffer_create_info = {
                    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    NULL,
                    0U,
                    // NVIDIA Driver 128 MB
                    // \[Gruen 2015\] [Holger Gruen. "Constant Buffers without Constant Pain." NVIDIA GameWorks Blog 2015.](https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0)
                    // AMD Special Pool 256MB
                    // \[Sawicki 2018\] [Adam Sawicki. "Memory Management in Vulkan and DX12." GDC 2018.](https://gpuopen.com/events/gdc-2018-presentations)
                    1,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL};

                VkBuffer dummy_buf;
                VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                assert(VK_SUCCESS == res_create_buffer);

                VkMemoryRequirements memory_requirements;
                pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
            }

            // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            // We use the "AMD Special Pool" for upload ring buffer
            uniform_upload_buffer_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            assert(VK_MAX_MEMORY_TYPES > uniform_upload_buffer_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > uniform_upload_buffer_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                uniform_upload_buffer_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                (1U == this->m_min_uniform_buffer_offset_alignment) ? 0U : this->m_min_uniform_buffer_offset_alignment,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_uniform_upload_buffer_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        // staging upload buffer
        assert(VK_NULL_HANDLE == this->m_staging_upload_buffer_memory_pool);
        {
            uint32_t staging_upload_buffer_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkBufferCreateInfo const buffer_create_info = {
                    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    NULL,
                    0U,
                    320ULL * 1024ULL * 1024ULL, // NOTE: 320 MB which is greater than 256MB "AMD Special Pool"
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL};

                VkBuffer dummy_buf;
                VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                assert(VK_SUCCESS == res_create_buffer);

                VkMemoryRequirements memory_requirements;
                pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
            }

            // Do NOT use "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT"
            // We leave the "AMD Special Pool" for upload ring buffer
            staging_upload_buffer_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            assert(VK_MAX_MEMORY_TYPES > staging_upload_buffer_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > staging_upload_buffer_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                staging_upload_buffer_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                (1U == this->m_optimal_buffer_copy_offset_alignment) ? 0U : this->m_optimal_buffer_copy_offset_alignment,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_staging_upload_buffer_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        // storage intermediate buffer
        assert(VK_NULL_HANDLE == this->m_storage_intermediate_buffer_memory_pool);
        {
            uint32_t storage_intermediate_buffer_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkBufferUsageFlags const usage = (!this->m_support_ray_tracing) ? (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) : (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR);

                VkBufferCreateInfo const buffer_create_info = {
                    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    NULL,
                    0U,
                    1U,
                    usage,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL};

                VkBuffer dummy_buf;
                VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                assert(VK_SUCCESS == res_create_buffer);

                VkMemoryRequirements memory_requirements;
                pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
            }

            storage_intermediate_buffer_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            assert(VK_MAX_MEMORY_TYPES > storage_intermediate_buffer_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > storage_intermediate_buffer_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                storage_intermediate_buffer_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                (1U == this->m_min_storage_buffer_offset_alignment) ? 0U : this->m_min_storage_buffer_offset_alignment,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_storage_intermediate_buffer_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        // storage asset buffer
        assert(VK_NULL_HANDLE == this->m_storage_asset_buffer_memory_pool);
        {
            uint32_t storage_asset_buffer_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkBufferUsageFlags const usage = (!this->m_support_ray_tracing) ? (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) : (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR);

                VkBufferCreateInfo const buffer_create_info = {
                    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    NULL,
                    0U,
                    1U,
                    usage,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL};

                VkBuffer dummy_buf;
                VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                assert(VK_SUCCESS == res_create_buffer);

                VkMemoryRequirements memory_requirements;
                pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
            }

            storage_asset_buffer_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            assert(VK_MAX_MEMORY_TYPES > storage_asset_buffer_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > storage_asset_buffer_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                storage_asset_buffer_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                (1U == this->m_min_storage_buffer_offset_alignment) ? 0U : this->m_min_storage_buffer_offset_alignment,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_storage_asset_buffer_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        // https://www.khronos.org/registry/vulkan/specs/1.0/html/chap13.html#VkMemoryRequirements
        // For images created with a color format, the memoryTypeBits member is identical for all VkImage objects created with the
        // same combination of values for the tiling member, the VK_IMAGE_CREATE_SPARSE_BINDING_BIT bit of the flags member, and
        // the VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT of the usage member in the VkImageCreateInfo structure passed to
        // vkCreateImage.
        assert(VK_NULL_HANDLE == this->m_color_transient_attachment_image_memory_pool);
        {
            uint32_t color_transient_attachment_image_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkImageCreateInfo const color_transient_attachment_image_create_info = {
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    NULL,
                    0U,
                    VK_IMAGE_TYPE_2D,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    {1U, 1U, 1U},
                    1U,
                    1U,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL,
                    VK_IMAGE_LAYOUT_UNDEFINED};

                VkImage dummy_img;
                VkResult const res_create_image = pfn_create_image(this->m_device, &color_transient_attachment_image_create_info, this->m_allocation_callbacks, &dummy_img);
                assert(VK_SUCCESS == res_create_image);

                VkMemoryRequirements memory_requirements;
                pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
            }

            // The lower index indicates the more performance
            color_transient_attachment_image_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
            assert(VK_MAX_MEMORY_TYPES > color_transient_attachment_image_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > color_transient_attachment_image_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                color_transient_attachment_image_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                0U,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_color_transient_attachment_image_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        assert(VK_NULL_HANDLE == this->m_color_attachment_sampled_image_memory_pool);
        {
            uint32_t color_attachment_sampled_image_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkImageCreateInfo depth_attachment_sampled_image_create_info = {
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    NULL,
                    0U,
                    VK_IMAGE_TYPE_2D,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    {1U, 1U, 1U},
                    1U,
                    1U,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL,
                    VK_IMAGE_LAYOUT_UNDEFINED};

                VkImage dummy_img;
                VkResult const res_create_image = pfn_create_image(this->m_device, &depth_attachment_sampled_image_create_info, this->m_allocation_callbacks, &dummy_img);
                assert(VK_SUCCESS == res_create_image);

                VkMemoryRequirements memory_requirements;
                pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
            }

            // The lower index indicates the more performance
            color_attachment_sampled_image_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            assert(VK_MAX_MEMORY_TYPES > color_attachment_sampled_image_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > color_attachment_sampled_image_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                color_attachment_sampled_image_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                0U,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_color_attachment_sampled_image_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        // https://registry.khronos.org/vulkan/specs/1.0/html/chap12.html#VkMemoryRequirements
        // For images created with a depth / stencil format, the memoryTypeBits member is identical for all VkImage objects created with the
        // same combination of values for the format member, the tiling member, the VK_IMAGE_CREATE_SPARSE_BINDING_BIT bit of the flags member, and
        // the VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT of the usage member in the VkImageCreateInfo structure passed to vkCreateImage.
        assert(VK_NULL_HANDLE == this->m_depth_transient_attachment_image_memory_pool);
        {
            uint32_t depth_transient_attachment_image_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkImageCreateInfo const depth_transient_attachment_image_create_info = {
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    NULL,
                    0U,
                    VK_IMAGE_TYPE_2D,
                    this->m_depth_attachment_image_format,
                    {1U, 1U, 1U},

                    1U,
                    1U,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL,
                    VK_IMAGE_LAYOUT_UNDEFINED};

                VkImage dummy_img;
                VkResult const res_create_image = pfn_create_image(this->m_device, &depth_transient_attachment_image_create_info, this->m_allocation_callbacks, &dummy_img);
                assert(VK_SUCCESS == res_create_image);

                VkMemoryRequirements memory_requirements;
                pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
            }

            // The lower index indicates the more performance
            depth_transient_attachment_image_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
            assert(VK_MAX_MEMORY_TYPES > depth_transient_attachment_image_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > depth_transient_attachment_image_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                depth_transient_attachment_image_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                0U,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_depth_transient_attachment_image_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        assert(VK_NULL_HANDLE == this->m_depth_attachment_sampled_image_memory_pool);
        if (this->m_depth_attachment_image_format_support_sampled_image)
        {
            uint32_t depth_attachment_sampled_image_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkImageCreateInfo depth_attachment_sampled_image_create_info = {
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    NULL,
                    0U,
                    VK_IMAGE_TYPE_2D,
                    this->m_depth_attachment_image_format,
                    {1U, 1U, 1U},
                    1U,
                    1U,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL,
                    VK_IMAGE_LAYOUT_UNDEFINED};

                VkImage dummy_img;
                VkResult const res_create_image = pfn_create_image(this->m_device, &depth_attachment_sampled_image_create_info, this->m_allocation_callbacks, &dummy_img);
                assert(VK_SUCCESS == res_create_image);

                VkMemoryRequirements memory_requirements;
                pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
            }

            // The lower index indicates the more performance
            depth_attachment_sampled_image_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            assert(VK_MAX_MEMORY_TYPES > depth_attachment_sampled_image_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > depth_attachment_sampled_image_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                depth_attachment_sampled_image_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                0U,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_depth_attachment_sampled_image_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        assert(VK_NULL_HANDLE == this->m_depth_stencil_transient_attachment_image_memory_pool);
        {
            uint32_t depth_stencil_transient_attachment_image_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = static_cast<VkDeviceSize>(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkImageCreateInfo const depth_transient_attachment_image_create_info = {
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    NULL,
                    0U,
                    VK_IMAGE_TYPE_2D,
                    this->m_depth_stencil_attachment_image_format,
                    {1U, 1U, 1U},
                    1U,
                    1U,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL,
                    VK_IMAGE_LAYOUT_UNDEFINED};

                VkImage dummy_img;
                VkResult const res_create_image = pfn_create_image(this->m_device, &depth_transient_attachment_image_create_info, this->m_allocation_callbacks, &dummy_img);
                assert(VK_SUCCESS == res_create_image);

                VkMemoryRequirements memory_requirements;
                pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
            }

            // The lower index indicates the more performance
            depth_stencil_transient_attachment_image_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
            assert(VK_MAX_MEMORY_TYPES > depth_stencil_transient_attachment_image_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > depth_stencil_transient_attachment_image_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                depth_stencil_transient_attachment_image_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                0U,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_depth_stencil_transient_attachment_image_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        assert(VK_NULL_HANDLE == this->m_depth_stencil_attachment_sampled_image_memory_pool);
        if (this->m_depth_stencil_attachment_image_format_support_sampled_image)
        {
            uint32_t depth_stencil_attachment_sampled_image_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkImageCreateInfo const depth_attachment_sampled_image_create_info = {
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    NULL,
                    0U,
                    VK_IMAGE_TYPE_2D,
                    this->m_depth_stencil_attachment_image_format,
                    {1U, 1U, 1U},
                    1U,
                    1U,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL,
                    VK_IMAGE_LAYOUT_UNDEFINED};

                VkImage dummy_img;
                VkResult const res_create_image = pfn_create_image(this->m_device, &depth_attachment_sampled_image_create_info, this->m_allocation_callbacks, &dummy_img);
                assert(VK_SUCCESS == res_create_image);

                VkMemoryRequirements memory_requirements;
                pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
            }

            // The lower index indicates the more performance
            depth_stencil_attachment_sampled_image_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            assert(VK_MAX_MEMORY_TYPES > depth_stencil_attachment_sampled_image_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > depth_stencil_attachment_sampled_image_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                depth_stencil_attachment_sampled_image_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                0U,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_depth_stencil_attachment_sampled_image_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        // https://registry.khronos.org/vulkan/specs/1.0/html/chap12.html#VkMemoryRequirements
        // For images created with a color format, the memoryTypeBits member is identical for all VkImage objects created with the
        // same combination of values for the tiling member, the VK_IMAGE_CREATE_SPARSE_BINDING_BIT bit of the flags member, and
        // the VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT of the usage member in the VkImageCreateInfo structure passed to
        // vkCreateImage.

        // vulkaninfo
        // https://github.com/KhronosGroup/Vulkan-Tools/tree/master/vulkaninfo/vulkaninfo/vulkaninfo.h
        // GetImageCreateInfo
        // FillImageTypeSupport
        // https://github.com/KhronosGroup/Vulkan-Tools/tree/master/vulkaninfo/vulkaninfo.cpp
        // GpuDumpMemoryProps //"usable for"

        assert(VK_NULL_HANDLE == this->m_storage_intermediate_image_memory_pool);
        {
            uint32_t storage_image_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkImageCreateInfo depth_attachment_sampled_image_create_info = {
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    NULL,
                    0U,
                    VK_IMAGE_TYPE_2D,
                    VK_FORMAT_R32_SFLOAT,
                    {8U, 8U, 1U},
                    1U,
                    1U,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL,
                    VK_IMAGE_LAYOUT_UNDEFINED};

                VkImage dummy_img;
                VkResult const res_create_image = pfn_create_image(this->m_device, &depth_attachment_sampled_image_create_info, this->m_allocation_callbacks, &dummy_img);
                assert(VK_SUCCESS == res_create_image);

                VkMemoryRequirements memory_requirements;
                pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
            }

            // The lower index indicates the more performance
            storage_image_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            assert(VK_MAX_MEMORY_TYPES > storage_image_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > storage_image_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                storage_image_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                0U,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_storage_intermediate_image_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        assert(VK_NULL_HANDLE == this->m_sampled_asset_image_memory_pool);
        {
            uint32_t sampled_asset_image_memory_index = VK_MAX_MEMORY_TYPES;

            VkDeviceSize memory_requirements_size = VkDeviceSize(-1);
            uint32_t memory_requirements_memory_type_bits = 0U;
            {
                VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;

                VkFormatProperties format_properties;
                pfn_get_physical_device_format_properties(this->m_physical_device, color_format, &format_properties);
                assert(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
                assert(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

                VkImageCreateInfo const image_create_info_regular_tiling_optimal = {
                    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                    NULL,
                    0U,
                    VK_IMAGE_TYPE_2D,
                    color_format,
                    {1U, 1U, 1U},
                    1U,
                    1U,
                    VK_SAMPLE_COUNT_1_BIT,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_SHARING_MODE_EXCLUSIVE,
                    0U,
                    NULL,
                    VK_IMAGE_LAYOUT_UNDEFINED};

                VkImage dummy_img;
                VkResult const res_create_image = pfn_create_image(this->m_device, &image_create_info_regular_tiling_optimal, this->m_allocation_callbacks, &dummy_img);
                assert(VK_SUCCESS == res_create_image);

                VkMemoryRequirements memory_requirements;
                pfn_get_image_memory_requirements(this->m_device, dummy_img, &memory_requirements);
                memory_requirements_size = memory_requirements.size;
                memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                pfn_destroy_image(this->m_device, dummy_img, this->m_allocation_callbacks);
            }

            sampled_asset_image_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            assert(VK_MAX_MEMORY_TYPES > sampled_asset_image_memory_index);
            assert(physical_device_memory_properties.memoryTypeCount > sampled_asset_image_memory_index);

            VmaPoolCreateInfo const pool_create_info = {
                sampled_asset_image_memory_index,
                VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                0U,
                0U,
                0U,
                1.0F,
                0U,
                NULL};

            VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_sampled_asset_image_memory_pool);
            assert(VK_SUCCESS == res_vma_create_pool);
        }

        if (this->m_support_ray_tracing)
        {
            constexpr VkDeviceSize const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT = 256U;
            constexpr VkDeviceSize const D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT = 16;

            // scratch buffer
            assert(VK_NULL_HANDLE == this->m_scratch_buffer_memory_pool);
            {
                uint32_t scratch_buffer_memory_index = VK_MAX_MEMORY_TYPES;

                VkDeviceSize memory_requirements_size = static_cast<VkDeviceSize>(-1);
                uint32_t memory_requirements_memory_type_bits = 0U;
                {
                    VkBufferCreateInfo const buffer_create_info = {
                        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                        NULL,
                        0U,
                        1U,
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
                        VK_SHARING_MODE_EXCLUSIVE,
                        0U,
                        NULL};

                    VkBuffer dummy_buf;
                    VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                    assert(VK_SUCCESS == res_create_buffer);

                    VkMemoryRequirements memory_requirements;
                    pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                    memory_requirements_size = memory_requirements.size;
                    memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                    pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
                }

                scratch_buffer_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                assert(VK_MAX_MEMORY_TYPES > scratch_buffer_memory_index);
                assert(physical_device_memory_properties.memoryTypeCount > scratch_buffer_memory_index);

                VmaPoolCreateInfo const pool_create_info = {
                    scratch_buffer_memory_index,
                    VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                    0U,
                    0U,
                    0U,
                    1.0F,
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                    NULL};

                VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_scratch_buffer_memory_pool);
                assert(VK_SUCCESS == res_vma_create_pool);
            }

            // intermediate bottom level acceleration structure
            assert(VK_NULL_HANDLE == this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
            {
                uint32_t intermediate_bottom_level_acceleration_structure_memory_index = VK_MAX_MEMORY_TYPES;

                VkDeviceSize memory_requirements_size = static_cast<VkDeviceSize>(-1);
                uint32_t memory_requirements_memory_type_bits = 0U;
                {
                    VkBufferCreateInfo const buffer_create_info = {
                        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                        NULL,
                        0U,
                        1U,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
                        VK_SHARING_MODE_EXCLUSIVE,
                        0U,
                        NULL};

                    VkBuffer dummy_buf;
                    VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                    assert(VK_SUCCESS == res_create_buffer);

                    VkMemoryRequirements memory_requirements;
                    pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                    memory_requirements_size = memory_requirements.size;
                    memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                    pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
                }

                intermediate_bottom_level_acceleration_structure_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                assert(VK_MAX_MEMORY_TYPES > intermediate_bottom_level_acceleration_structure_memory_index);
                assert(physical_device_memory_properties.memoryTypeCount > intermediate_bottom_level_acceleration_structure_memory_index);

                VmaPoolCreateInfo const pool_create_info = {
                    intermediate_bottom_level_acceleration_structure_memory_index,
                    VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                    0U,
                    0U,
                    0U,
                    1.0F,
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                    NULL};

                VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
                assert(VK_SUCCESS == res_vma_create_pool);
            }

            // non compacted bottom level acceleration structure buffer
            assert(VK_NULL_HANDLE == this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
            {
                uint32_t non_compacted_bottom_level_acceleration_structure_buffer_memory_index = VK_MAX_MEMORY_TYPES;

                VkDeviceSize memory_requirements_size = static_cast<VkDeviceSize>(-1);
                uint32_t memory_requirements_memory_type_bits = 0U;
                {
                    VkBufferCreateInfo const buffer_create_info = {
                        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                        NULL,
                        0U,
                        1U,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                        VK_SHARING_MODE_EXCLUSIVE,
                        0U,
                        NULL};

                    VkBuffer dummy_buf;
                    VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                    assert(VK_SUCCESS == res_create_buffer);

                    VkMemoryRequirements memory_requirements;
                    pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                    memory_requirements_size = memory_requirements.size;
                    memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                    pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
                }

                non_compacted_bottom_level_acceleration_structure_buffer_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                assert(VK_MAX_MEMORY_TYPES > non_compacted_bottom_level_acceleration_structure_buffer_memory_index);
                assert(physical_device_memory_properties.memoryTypeCount > non_compacted_bottom_level_acceleration_structure_buffer_memory_index);

                VmaPoolCreateInfo const pool_create_info = {
                    non_compacted_bottom_level_acceleration_structure_buffer_memory_index,
                    VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                    0U,
                    0U,
                    0U,
                    1.0F,
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                    NULL};

                VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
                assert(VK_SUCCESS == res_vma_create_pool);
            }

            // compacted bottom level acceleration structure
            assert(VK_NULL_HANDLE == this->m_compacted_bottom_level_acceleration_structure_memory_pool);
            {
                uint32_t compacted_bottom_level_acceleration_structure_memory_index = VK_MAX_MEMORY_TYPES;

                VkDeviceSize memory_requirements_size = static_cast<VkDeviceSize>(-1);
                uint32_t memory_requirements_memory_type_bits = 0U;
                {
                    VkBufferCreateInfo const buffer_create_info = {
                        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                        NULL,
                        0U,
                        1U,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
                        VK_SHARING_MODE_EXCLUSIVE,
                        0U,
                        NULL};

                    VkBuffer dummy_buf;
                    VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                    assert(VK_SUCCESS == res_create_buffer);

                    VkMemoryRequirements memory_requirements;
                    pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                    memory_requirements_size = memory_requirements.size;
                    memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                    pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
                }

                compacted_bottom_level_acceleration_structure_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                assert(VK_MAX_MEMORY_TYPES > compacted_bottom_level_acceleration_structure_memory_index);
                assert(physical_device_memory_properties.memoryTypeCount > compacted_bottom_level_acceleration_structure_memory_index);

                VmaPoolCreateInfo const pool_create_info = {
                    compacted_bottom_level_acceleration_structure_memory_index,
                    VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                    0U,
                    0U,
                    0U,
                    1.0F,
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                    NULL};

                VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_compacted_bottom_level_acceleration_structure_memory_pool);
                assert(VK_SUCCESS == res_vma_create_pool);
            }

            // top level acceleration structure instance buffer
            assert(VK_NULL_HANDLE == this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
            {
                uint32_t top_level_acceleration_structure_instance_upload_buffer_memory_index = VK_MAX_MEMORY_TYPES;

                VkDeviceSize memory_requirements_size = static_cast<VkDeviceSize>(-1);
                uint32_t memory_requirements_memory_type_bits = 0U;
                {
                    VkBufferCreateInfo const buffer_create_info = {
                        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                        NULL,
                        0U,
                        320ULL * 1024ULL * 1024ULL, // NOTE: 320 MB which is greater than 256MB "AMD Special Pool"
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
                        VK_SHARING_MODE_EXCLUSIVE,
                        0U,
                        NULL};

                    VkBuffer dummy_buf;
                    VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                    assert(VK_SUCCESS == res_create_buffer);

                    VkMemoryRequirements memory_requirements;
                    pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                    memory_requirements_size = memory_requirements.size;
                    memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                    pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
                }

                top_level_acceleration_structure_instance_upload_buffer_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                assert(VK_MAX_MEMORY_TYPES > top_level_acceleration_structure_instance_upload_buffer_memory_index);
                assert(physical_device_memory_properties.memoryTypeCount > top_level_acceleration_structure_instance_upload_buffer_memory_index);

                VmaPoolCreateInfo const pool_create_info = {
                    top_level_acceleration_structure_instance_upload_buffer_memory_index,
                    VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                    0U,
                    0U,
                    0U,
                    1.0F,
                    D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT,
                    NULL};

                VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
                assert(VK_SUCCESS == res_vma_create_pool);
            }

            // top level acceleration structure
            assert(VK_NULL_HANDLE == this->m_top_level_acceleration_structure_memory_pool);
            {
                uint32_t top_level_acceleration_structure_memory_index = VK_MAX_MEMORY_TYPES;

                VkDeviceSize memory_requirements_size = static_cast<VkDeviceSize>(-1);
                uint32_t memory_requirements_memory_type_bits = 0U;
                {
                    VkBufferCreateInfo const buffer_create_info = {
                        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                        NULL,
                        0U,
                        1U,
                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                        VK_SHARING_MODE_EXCLUSIVE,
                        0U,
                        NULL};

                    VkBuffer dummy_buf;
                    VkResult const res_create_buffer = pfn_create_buffer(this->m_device, &buffer_create_info, this->m_allocation_callbacks, &dummy_buf);
                    assert(VK_SUCCESS == res_create_buffer);

                    VkMemoryRequirements memory_requirements;
                    pfn_get_buffer_memory_requirements(this->m_device, dummy_buf, &memory_requirements);
                    memory_requirements_size = memory_requirements.size;
                    memory_requirements_memory_type_bits = memory_requirements.memoryTypeBits;

                    pfn_destroy_buffer(this->m_device, dummy_buf, this->m_allocation_callbacks);
                }

                top_level_acceleration_structure_memory_index = _internal_find_lowest_memory_type_index(&physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                assert(VK_MAX_MEMORY_TYPES > top_level_acceleration_structure_memory_index);
                assert(physical_device_memory_properties.memoryTypeCount > top_level_acceleration_structure_memory_index);

                VmaPoolCreateInfo const pool_create_info = {
                    top_level_acceleration_structure_memory_index,
                    VMA_POOL_CREATE_IGNORE_BUFFER_IMAGE_GRANULARITY_BIT,
                    0U,
                    0U,
                    0U,
                    1.0F,
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
                    NULL};

                VkResult const res_vma_create_pool = vmaCreatePool(this->m_memory_allocator, &pool_create_info, &this->m_top_level_acceleration_structure_memory_pool);
                assert(VK_SUCCESS == res_vma_create_pool);
            }
        }
    }

    assert(NULL == this->m_pfn_wait_for_fences);
    this->m_pfn_wait_for_fences = reinterpret_cast<PFN_vkWaitForFences>(this->m_pfn_get_device_proc_addr(this->m_device, "vkWaitForFences"));
    assert(NULL != this->m_pfn_wait_for_fences);

    assert(NULL == this->m_pfn_reset_fences);
    this->m_pfn_reset_fences = reinterpret_cast<PFN_vkResetFences>(this->m_pfn_get_device_proc_addr(this->m_device, "vkResetFences"));
    assert(NULL != this->m_pfn_reset_fences);

    assert(NULL == this->m_pfn_reset_command_pool);
    this->m_pfn_reset_command_pool = reinterpret_cast<PFN_vkResetCommandPool>(this->m_pfn_get_device_proc_addr(this->m_device, "vkResetCommandPool"));
    assert(NULL != this->m_pfn_reset_command_pool);

    assert(NULL == this->m_pfn_acquire_next_image);
    this->m_pfn_acquire_next_image = reinterpret_cast<PFN_vkAcquireNextImageKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkAcquireNextImageKHR"));
    assert(NULL != this->m_pfn_acquire_next_image);

    assert(NULL == this->m_pfn_create_image_view);
    this->m_pfn_create_image_view = reinterpret_cast<PFN_vkCreateImageView>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateImageView"));
    assert(NULL != this->m_pfn_create_image_view);

    assert(NULL == this->m_pfn_destroy_image_view);
    this->m_pfn_destroy_image_view = reinterpret_cast<PFN_vkDestroyImageView>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyImageView"));
    assert(NULL != this->m_pfn_destroy_image_view);

    if (this->m_support_ray_tracing)
    {
        assert(NULL == this->m_pfn_get_buffer_device_address);
        this->m_pfn_get_buffer_device_address = reinterpret_cast<PFN_vkGetBufferDeviceAddress>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetBufferDeviceAddressKHR"));
        assert(NULL != this->m_pfn_get_buffer_device_address);

        assert(NULL == this->m_pfn_create_acceleration_structure);
        this->m_pfn_create_acceleration_structure = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateAccelerationStructureKHR"));
        assert(NULL != this->m_pfn_create_acceleration_structure);

        assert(NULL == this->m_pfn_destroy_acceleration_structure);
        this->m_pfn_destroy_acceleration_structure = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyAccelerationStructureKHR"));
        assert(NULL != this->m_pfn_destroy_acceleration_structure);

        assert(NULL == this->m_pfn_get_acceleration_structure_device_address);
        this->m_pfn_get_acceleration_structure_device_address = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetAccelerationStructureDeviceAddressKHR"));
        assert(NULL != this->m_pfn_get_acceleration_structure_device_address);

        assert(NULL == this->m_pfn_get_query_pool_results);
        this->m_pfn_get_query_pool_results = reinterpret_cast<PFN_vkGetQueryPoolResults>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetQueryPoolResults"));
        assert(NULL != this->m_pfn_get_query_pool_results);
    }
}

extern void brx_pal_destroy_vk_device(brx_pal_device *wrapped_device)
{
    assert(NULL != wrapped_device);
    brx_pal_vk_device *delete_unwrapped_device = static_cast<brx_pal_vk_device *>(wrapped_device);

    delete_unwrapped_device->uninit();

    delete_unwrapped_device->~brx_pal_vk_device();
    mcrt_free(delete_unwrapped_device);
}

void brx_pal_vk_device::uninit()
{
    assert(NULL != this->m_pfn_get_instance_proc_addr);
    assert(VK_NULL_HANDLE != this->m_instance);
#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
    assert(VK_NULL_HANDLE != this->m_message);
#else
    // No Validation Layer
#endif
#else
#error "0 or 1"
#endif
    assert(NULL != this->m_pfn_get_device_proc_addr);
    assert(VK_NULL_HANDLE != this->m_device);
    assert(VK_NULL_HANDLE != this->m_memory_allocator);
    assert(VK_NULL_HANDLE != this->m_uniform_upload_buffer_memory_pool);
    assert(VK_NULL_HANDLE != this->m_staging_upload_buffer_memory_pool);
    assert(VK_NULL_HANDLE != this->m_storage_intermediate_buffer_memory_pool);
    assert(VK_NULL_HANDLE != this->m_storage_asset_buffer_memory_pool);
    assert(VK_NULL_HANDLE != this->m_color_transient_attachment_image_memory_pool);
    assert(VK_NULL_HANDLE != this->m_color_attachment_sampled_image_memory_pool);
    assert(VK_NULL_HANDLE != this->m_depth_transient_attachment_image_memory_pool);
    assert((!this->m_depth_attachment_image_format_support_sampled_image) || (VK_NULL_HANDLE != this->m_depth_attachment_sampled_image_memory_pool));
    assert(VK_NULL_HANDLE != this->m_depth_stencil_transient_attachment_image_memory_pool);
    assert((!this->m_depth_stencil_attachment_image_format_support_sampled_image) || (VK_NULL_HANDLE != this->m_depth_stencil_attachment_sampled_image_memory_pool));
    assert(VK_NULL_HANDLE != this->m_storage_intermediate_image_memory_pool);
    assert(VK_NULL_HANDLE != this->m_sampled_asset_image_memory_pool);
    assert((!this->m_support_ray_tracing) || (VK_NULL_HANDLE != this->m_scratch_buffer_memory_pool));
    assert((!this->m_support_ray_tracing) || (VK_NULL_HANDLE != this->m_intermediate_bottom_level_acceleration_structure_memory_pool));
    assert((!this->m_support_ray_tracing) || (VK_NULL_HANDLE != this->m_non_compacted_bottom_level_acceleration_structure_memory_pool));
    assert((!this->m_support_ray_tracing) || (VK_NULL_HANDLE != this->m_compacted_bottom_level_acceleration_structure_memory_pool));
    assert((!this->m_support_ray_tracing) || (VK_NULL_HANDLE != this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool));
    assert((!this->m_support_ray_tracing) || (VK_NULL_HANDLE != this->m_top_level_acceleration_structure_memory_pool));

    vmaDestroyPool(this->m_memory_allocator, this->m_uniform_upload_buffer_memory_pool);
    this->m_uniform_upload_buffer_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_staging_upload_buffer_memory_pool);
    this->m_staging_upload_buffer_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_storage_intermediate_buffer_memory_pool);
    this->m_storage_intermediate_buffer_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_storage_asset_buffer_memory_pool);
    this->m_storage_asset_buffer_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_color_transient_attachment_image_memory_pool);
    this->m_color_transient_attachment_image_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_color_attachment_sampled_image_memory_pool);
    this->m_color_attachment_sampled_image_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_depth_transient_attachment_image_memory_pool);
    this->m_depth_transient_attachment_image_memory_pool = VK_NULL_HANDLE;

    if (this->m_depth_attachment_image_format_support_sampled_image)
    {
        vmaDestroyPool(this->m_memory_allocator, this->m_depth_attachment_sampled_image_memory_pool);
        this->m_depth_attachment_sampled_image_memory_pool = VK_NULL_HANDLE;
    }
    else
    {
        assert(VK_NULL_HANDLE == this->m_depth_attachment_sampled_image_memory_pool);
    }

    vmaDestroyPool(this->m_memory_allocator, this->m_depth_stencil_transient_attachment_image_memory_pool);
    this->m_depth_stencil_transient_attachment_image_memory_pool = VK_NULL_HANDLE;

    if (this->m_depth_stencil_attachment_image_format_support_sampled_image)
    {
        vmaDestroyPool(this->m_memory_allocator, this->m_depth_stencil_attachment_sampled_image_memory_pool);
        this->m_depth_stencil_attachment_sampled_image_memory_pool = VK_NULL_HANDLE;
    }
    else
    {
        assert(VK_NULL_HANDLE == this->m_depth_stencil_attachment_sampled_image_memory_pool);
    }

    vmaDestroyPool(this->m_memory_allocator, this->m_storage_intermediate_image_memory_pool);
    this->m_storage_intermediate_image_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_sampled_asset_image_memory_pool);
    this->m_sampled_asset_image_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_scratch_buffer_memory_pool);
    this->m_scratch_buffer_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
    this->m_intermediate_bottom_level_acceleration_structure_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
    this->m_non_compacted_bottom_level_acceleration_structure_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_compacted_bottom_level_acceleration_structure_memory_pool);
    this->m_compacted_bottom_level_acceleration_structure_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
    this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool = VK_NULL_HANDLE;

    vmaDestroyPool(this->m_memory_allocator, this->m_top_level_acceleration_structure_memory_pool);
    this->m_top_level_acceleration_structure_memory_pool = VK_NULL_HANDLE;

    vmaDestroyAllocator(this->m_memory_allocator);
    this->m_memory_allocator = VK_NULL_HANDLE;

    PFN_vkDestroyDevice const pfn_destroy_device = reinterpret_cast<PFN_vkDestroyDevice>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyDevice"));
    assert(NULL != pfn_destroy_device);
    pfn_destroy_device(this->m_device, this->m_allocation_callbacks);
    this->m_device = VK_NULL_HANDLE;

    this->m_pfn_get_device_proc_addr = NULL;

#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
    PFN_vkDestroyDebugUtilsMessengerEXT const pfn_destroy_debug_utils_messenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkDestroyDebugUtilsMessengerEXT"));
    assert(NULL != pfn_destroy_debug_utils_messenger);
    pfn_destroy_debug_utils_messenger(this->m_instance, this->m_message, this->m_allocation_callbacks);
    this->m_message = VK_NULL_HANDLE;
#else
    // No Validation Layer
#endif
#else
#error "0 or 1"
#endif

    PFN_vkDestroyInstance pfn_destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkDestroyInstance"));
    assert(NULL != pfn_destroy_instance);
    pfn_destroy_instance(this->m_instance, this->m_allocation_callbacks);
    this->m_instance = VK_NULL_HANDLE;

    this->m_pfn_get_instance_proc_addr = NULL;
}

brx_pal_vk_device::~brx_pal_vk_device()
{
    assert(NULL == this->m_pfn_get_instance_proc_addr);
    assert(VK_NULL_HANDLE == this->m_instance);
#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
    assert(VK_NULL_HANDLE == this->m_message);
#else
    // No Validation Layer
#endif
#else
#error "0 or 1"
#endif
    assert(NULL == this->m_pfn_get_device_proc_addr);
    assert(VK_NULL_HANDLE == this->m_device);
    assert(VK_NULL_HANDLE == this->m_memory_allocator);
    assert(VK_NULL_HANDLE == this->m_uniform_upload_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_staging_upload_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_storage_intermediate_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_storage_asset_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_color_transient_attachment_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_color_attachment_sampled_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_depth_transient_attachment_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_depth_attachment_sampled_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_depth_stencil_transient_attachment_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_depth_stencil_attachment_sampled_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_storage_intermediate_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_sampled_asset_image_memory_pool);
    assert(VK_NULL_HANDLE == this->m_scratch_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_intermediate_bottom_level_acceleration_structure_memory_pool);
    assert(VK_NULL_HANDLE == this->m_non_compacted_bottom_level_acceleration_structure_memory_pool);
    assert(VK_NULL_HANDLE == this->m_compacted_bottom_level_acceleration_structure_memory_pool);
    assert(VK_NULL_HANDLE == this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool);
    assert(VK_NULL_HANDLE == this->m_top_level_acceleration_structure_memory_pool);
}

BRX_PAL_BACKEND_NAME brx_pal_vk_device::get_backend_name() const
{
    return BRX_PAL_BACKEND_NAME_VK;
}

bool brx_pal_vk_device::is_ray_tracing_supported() const
{
    return this->m_support_ray_tracing;
}

brx_pal_graphics_queue *brx_pal_vk_device::create_graphics_queue() const
{
    PFN_vkQueueSubmit pfn_queue_submit = reinterpret_cast<PFN_vkQueueSubmit>(this->m_pfn_get_device_proc_addr(this->m_device, "vkQueueSubmit"));
    assert(NULL != pfn_queue_submit);
    PFN_vkQueuePresentKHR pfn_queue_present = reinterpret_cast<PFN_vkQueuePresentKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkQueuePresentKHR"));
    assert(NULL != pfn_queue_present);

    void *new_brx_pal_graphics_queue_base = mcrt_malloc(sizeof(brx_pal_vk_graphics_queue), alignof(brx_pal_vk_graphics_queue));
    assert(NULL != new_brx_pal_graphics_queue_base);

    brx_pal_vk_graphics_queue *new_brx_pal_graphics_queue = new (new_brx_pal_graphics_queue_base) brx_pal_vk_graphics_queue{this->m_has_dedicated_upload_queue, this->m_upload_queue_family_index, this->m_graphics_queue_family_index, this->m_graphics_queue, pfn_queue_submit, pfn_queue_present};
    return new_brx_pal_graphics_queue;
}

void brx_pal_vk_device::destroy_graphics_queue(brx_pal_graphics_queue *brx_pal_graphics_queue) const
{
    assert(NULL != brx_pal_graphics_queue);
    brx_pal_vk_graphics_queue *delete_graphics_queue = static_cast<brx_pal_vk_graphics_queue *>(brx_pal_graphics_queue);

    VkQueue stealed_graphics_queue = VK_NULL_HANDLE;
    delete_graphics_queue->steal(&stealed_graphics_queue);

    delete_graphics_queue->~brx_pal_vk_graphics_queue();
    mcrt_free(delete_graphics_queue);

    assert(stealed_graphics_queue == this->m_graphics_queue);
}

brx_pal_upload_queue *brx_pal_vk_device::create_upload_queue() const
{
    PFN_vkQueueSubmit pfn_queue_submit = reinterpret_cast<PFN_vkQueueSubmit>(this->m_pfn_get_device_proc_addr(this->m_device, "vkQueueSubmit"));
    assert(NULL != pfn_queue_submit);

    void *new_brx_pal_upload_queue_base = mcrt_malloc(sizeof(brx_pal_vk_upload_queue), alignof(brx_pal_vk_upload_queue));
    assert(NULL != new_brx_pal_upload_queue_base);

    brx_pal_vk_upload_queue *new_brx_pal_upload_queue = new (new_brx_pal_upload_queue_base) brx_pal_vk_upload_queue{this->m_has_dedicated_upload_queue, this->m_upload_queue_family_index, this->m_graphics_queue_family_index, this->m_upload_queue, pfn_queue_submit};
    return new_brx_pal_upload_queue;
}

void brx_pal_vk_device::destroy_upload_queue(brx_pal_upload_queue *brx_pal_upload_queue) const
{
    assert(NULL != brx_pal_upload_queue);
    brx_pal_vk_upload_queue *delete_upload_queue = static_cast<brx_pal_vk_upload_queue *>(brx_pal_upload_queue);

    VkQueue stealed_upload_queue = VK_NULL_HANDLE;
    delete_upload_queue->steal(&stealed_upload_queue);

    delete_upload_queue->~brx_pal_vk_upload_queue();
    mcrt_free(delete_upload_queue);

    assert(stealed_upload_queue == this->m_upload_queue);
}

brx_pal_graphics_command_buffer *brx_pal_vk_device::create_graphics_command_buffer() const
{
    void *new_unwrapped_graphics_command_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_graphics_command_buffer), alignof(brx_pal_vk_graphics_command_buffer));
    assert(NULL != new_unwrapped_graphics_command_buffer_base);

    brx_pal_vk_graphics_command_buffer *new_unwrapped_graphics_command_buffer = new (new_unwrapped_graphics_command_buffer_base) brx_pal_vk_graphics_command_buffer{};
    new_unwrapped_graphics_command_buffer->init(this->m_support_ray_tracing, this->m_has_dedicated_upload_queue, this->m_graphics_queue_family_index, this->m_upload_queue_family_index, this->m_pfn_get_instance_proc_addr, this->m_instance, this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);
    return new_unwrapped_graphics_command_buffer;
}

void brx_pal_vk_device::reset_graphics_command_buffer(brx_pal_graphics_command_buffer *brx_pal_graphics_command_buffer) const
{
    assert(NULL != brx_pal_graphics_command_buffer);
    VkCommandPool command_pool = static_cast<brx_pal_vk_graphics_command_buffer *>(brx_pal_graphics_command_buffer)->get_command_pool();

    VkResult res_reset_command_pool = this->m_pfn_reset_command_pool(this->m_device, command_pool, 0U);
    assert(VK_SUCCESS == res_reset_command_pool);
}

void brx_pal_vk_device::destroy_graphics_command_buffer(brx_pal_graphics_command_buffer *wrapped_graphics_command_buffer) const
{
    assert(NULL != wrapped_graphics_command_buffer);
    brx_pal_vk_graphics_command_buffer *delete_unwrapped_graphics_command_buffer = static_cast<brx_pal_vk_graphics_command_buffer *>(wrapped_graphics_command_buffer);

    delete_unwrapped_graphics_command_buffer->uninit(this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);

    delete_unwrapped_graphics_command_buffer->~brx_pal_vk_graphics_command_buffer();
    mcrt_free(delete_unwrapped_graphics_command_buffer);
}

brx_pal_upload_command_buffer *brx_pal_vk_device::create_upload_command_buffer() const
{
    void *new_unwrapped_upload_command_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_upload_command_buffer), alignof(brx_pal_vk_upload_command_buffer));
    assert(NULL != new_unwrapped_upload_command_buffer_base);

    brx_pal_vk_upload_command_buffer *new_unwrapped_upload_command_buffer = new (new_unwrapped_upload_command_buffer_base) brx_pal_vk_upload_command_buffer{};
    new_unwrapped_upload_command_buffer->init(this->m_support_ray_tracing, this->m_has_dedicated_upload_queue, this->m_graphics_queue_family_index, this->m_upload_queue_family_index, this->m_pfn_get_instance_proc_addr, this->m_instance, this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);
    return new_unwrapped_upload_command_buffer;
}

void brx_pal_vk_device::reset_upload_command_buffer(brx_pal_upload_command_buffer *brx_pal_upload_command_buffer) const
{
    assert(NULL != brx_pal_upload_command_buffer);
    VkCommandPool upload_command_pool = static_cast<brx_pal_vk_upload_command_buffer *>(brx_pal_upload_command_buffer)->get_upload_command_pool();
    VkCommandPool graphics_command_pool = static_cast<brx_pal_vk_upload_command_buffer *>(brx_pal_upload_command_buffer)->get_graphics_command_pool();

    if (this->m_has_dedicated_upload_queue)
    {
        if (this->m_upload_queue_family_index != this->m_graphics_queue_family_index)
        {
            assert(VK_NULL_HANDLE != upload_command_pool && VK_NULL_HANDLE == graphics_command_pool);

            VkResult res_reset_upload_command_pool = this->m_pfn_reset_command_pool(this->m_device, upload_command_pool, 0U);
            assert(VK_SUCCESS == res_reset_upload_command_pool);
        }
        else
        {
            assert(VK_NULL_HANDLE != upload_command_pool && VK_NULL_HANDLE == graphics_command_pool);

            VkResult res_reset_upload_command_pool = this->m_pfn_reset_command_pool(this->m_device, upload_command_pool, 0U);
            assert(VK_SUCCESS == res_reset_upload_command_pool);
        }
    }
    else
    {
        assert(VK_NULL_HANDLE == upload_command_pool && VK_NULL_HANDLE != graphics_command_pool);

        VkResult res_reset_graphics_command_pool = this->m_pfn_reset_command_pool(this->m_device, graphics_command_pool, 0U);
        assert(VK_SUCCESS == res_reset_graphics_command_pool);
    }
}

void brx_pal_vk_device::destroy_upload_command_buffer(brx_pal_upload_command_buffer *wrapped_upload_command_buffer) const
{
    assert(NULL != wrapped_upload_command_buffer);
    brx_pal_vk_upload_command_buffer *delete_unwrapped_upload_command_buffer = static_cast<brx_pal_vk_upload_command_buffer *>(wrapped_upload_command_buffer);

    delete_unwrapped_upload_command_buffer->uninit(this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);

    delete_unwrapped_upload_command_buffer->~brx_pal_vk_upload_command_buffer();
    mcrt_free(delete_unwrapped_upload_command_buffer);
}

brx_pal_fence *brx_pal_vk_device::create_fence(bool signaled) const
{
    VkFence new_fence = VK_NULL_HANDLE;
    {
        PFN_vkCreateFence pfn_create_fence = reinterpret_cast<PFN_vkCreateFence>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateFence"));
        assert(NULL != pfn_create_fence);

        VkFenceCreateInfo fence_create_info;
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = NULL;
        fence_create_info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0U;
        VkResult res_create_fence = pfn_create_fence(this->m_device, &fence_create_info, this->m_allocation_callbacks, &new_fence);
        assert(VK_SUCCESS == res_create_fence);
    }

    void *new_brx_pal_fence_base = mcrt_malloc(sizeof(brx_pal_vk_fence), alignof(brx_pal_vk_fence));
    assert(NULL != new_brx_pal_fence_base);

    brx_pal_vk_fence *new_brx_pal_fence = new (new_brx_pal_fence_base) brx_pal_vk_fence{new_fence};
    return new_brx_pal_fence;
}

void brx_pal_vk_device::wait_for_fence(brx_pal_fence *brx_pal_fence) const
{
    assert(NULL != brx_pal_fence);
    VkFence fence = static_cast<brx_pal_vk_fence *>(brx_pal_fence)->get_fence();

    VkResult res_wait_for_fences = this->m_pfn_wait_for_fences(this->m_device, 1U, &fence, VK_TRUE, UINT64_MAX);
    assert(VK_SUCCESS == res_wait_for_fences);
}

void brx_pal_vk_device::reset_fence(brx_pal_fence *brx_pal_fence) const
{
    assert(NULL != brx_pal_fence);
    VkFence fence = static_cast<brx_pal_vk_fence *>(brx_pal_fence)->get_fence();

    VkResult res_reset_fences = this->m_pfn_reset_fences(this->m_device, 1U, &fence);
    assert(VK_SUCCESS == res_reset_fences);
}

void brx_pal_vk_device::destroy_fence(brx_pal_fence *brx_pal_fence) const
{
    assert(NULL != brx_pal_fence);
    brx_pal_vk_fence *delete_fence = static_cast<brx_pal_vk_fence *>(brx_pal_fence);

    VkFence stealed_fence = VK_NULL_HANDLE;
    delete_fence->steal(&stealed_fence);

    delete_fence->~brx_pal_vk_fence();
    mcrt_free(delete_fence);

    PFN_vkDestroyFence pfn_destroy_fence = reinterpret_cast<PFN_vkDestroyFence>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyFence"));
    assert(NULL != pfn_destroy_fence);

    pfn_destroy_fence(this->m_device, stealed_fence, this->m_allocation_callbacks);
}

brx_pal_descriptor_set_layout *brx_pal_vk_device::create_descriptor_set_layout(uint32_t descriptor_set_binding_count, BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const *descriptor_set_bindings) const
{
    void *new_unwrapped_descriptor_set_layout_base = mcrt_malloc(sizeof(brx_pal_vk_descriptor_set_layout), alignof(brx_pal_vk_descriptor_set_layout));
    assert(NULL != new_unwrapped_descriptor_set_layout_base);

    brx_pal_vk_descriptor_set_layout *new_unwrapped_descriptor_set_layout = new (new_unwrapped_descriptor_set_layout_base) brx_pal_vk_descriptor_set_layout{};
    new_unwrapped_descriptor_set_layout->init(this->m_support_ray_tracing, descriptor_set_binding_count, descriptor_set_bindings, this->m_max_per_stage_descriptor_storage_buffers, this->m_max_per_stage_descriptor_sampled_images, this->m_max_descriptor_set_storage_buffers, this->m_max_descriptor_set_sampled_images, this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);
    return new_unwrapped_descriptor_set_layout;
}

void brx_pal_vk_device::destroy_descriptor_set_layout(brx_pal_descriptor_set_layout *wrapped_descriptor_set_layout) const
{
    assert(NULL != wrapped_descriptor_set_layout);
    brx_pal_vk_descriptor_set_layout *delete_unwrapped_descriptor_set_layout = static_cast<brx_pal_vk_descriptor_set_layout *>(wrapped_descriptor_set_layout);

    delete_unwrapped_descriptor_set_layout->uninit(this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);

    delete_unwrapped_descriptor_set_layout->~brx_pal_vk_descriptor_set_layout();
    mcrt_free(delete_unwrapped_descriptor_set_layout);
}

brx_pal_pipeline_layout *brx_pal_vk_device::create_pipeline_layout(uint32_t descriptor_set_layout_count, brx_pal_descriptor_set_layout const *const *brx_pal_descriptor_set_layouts) const
{
    VkPipelineLayout new_pipeline_layout = VK_NULL_HANDLE;
    {
        PFN_vkCreatePipelineLayout pfn_create_pipeline_layout = reinterpret_cast<PFN_vkCreatePipelineLayout>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreatePipelineLayout"));
        assert(NULL != pfn_create_pipeline_layout);

        constexpr uint32_t const max_descriptor_set_layout_count = 4U;
        assert(descriptor_set_layout_count <= max_descriptor_set_layout_count);
        descriptor_set_layout_count = (descriptor_set_layout_count <= max_descriptor_set_layout_count) ? descriptor_set_layout_count : max_descriptor_set_layout_count;

        VkDescriptorSetLayout descriptor_set_layouts[max_descriptor_set_layout_count];
        for (uint32_t set_index = 0U; set_index < descriptor_set_layout_count; ++set_index)
        {
            assert(NULL != (brx_pal_descriptor_set_layouts[set_index]));
            descriptor_set_layouts[set_index] = static_cast<brx_pal_vk_descriptor_set_layout const *>(brx_pal_descriptor_set_layouts[set_index])->get_descriptor_set_layout();
        }

        VkPipelineLayoutCreateInfo pipeline_layout_create_info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, NULL, 0U, descriptor_set_layout_count, descriptor_set_layouts, 0U, NULL};

        VkResult res_create_pipeline_layout = pfn_create_pipeline_layout(this->m_device, &pipeline_layout_create_info, this->m_allocation_callbacks, &new_pipeline_layout);
        assert(VK_SUCCESS == res_create_pipeline_layout);
    }

    void *new_brx_pal_pipeline_layout_base = mcrt_malloc(sizeof(brx_pal_vk_pipeline_layout), alignof(brx_pal_vk_pipeline_layout));
    assert(NULL != new_brx_pal_pipeline_layout_base);

    brx_pal_vk_pipeline_layout *new_brx_pal_pipeline_layout = new (new_brx_pal_pipeline_layout_base) brx_pal_vk_pipeline_layout{new_pipeline_layout};
    return new_brx_pal_pipeline_layout;
}

void brx_pal_vk_device::destroy_pipeline_layout(brx_pal_pipeline_layout *brx_pal_pipeline_layout) const
{
    assert(NULL != brx_pal_pipeline_layout);
    brx_pal_vk_pipeline_layout *delete_pipeline_layout = static_cast<brx_pal_vk_pipeline_layout *>(brx_pal_pipeline_layout);

    VkPipelineLayout stealed_pipeline_layout = VK_NULL_HANDLE;
    delete_pipeline_layout->steal(&stealed_pipeline_layout);

    delete_pipeline_layout->~brx_pal_vk_pipeline_layout();
    mcrt_free(delete_pipeline_layout);

    PFN_vkDestroyPipelineLayout pfn_destroy_pipeline_layout = reinterpret_cast<PFN_vkDestroyPipelineLayout>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyPipelineLayout"));
    assert(NULL != pfn_destroy_pipeline_layout);

    pfn_destroy_pipeline_layout(this->m_device, stealed_pipeline_layout, this->m_allocation_callbacks);
}

brx_pal_descriptor_set *brx_pal_vk_device::create_descriptor_set(brx_pal_descriptor_set_layout const *descriptor_set_layout, uint32_t unbounded_descriptor_count)
{
    void *new_unwrapped_descriptor_set_base = mcrt_malloc(sizeof(brx_pal_vk_descriptor_set), alignof(brx_pal_vk_descriptor_set));
    assert(NULL != new_unwrapped_descriptor_set_base);

    brx_pal_vk_descriptor_set *new_unwrapped_descriptor_set = new (new_unwrapped_descriptor_set_base) brx_pal_vk_descriptor_set{};
    new_unwrapped_descriptor_set->init(this->m_support_ray_tracing, descriptor_set_layout, unbounded_descriptor_count, this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);
    return new_unwrapped_descriptor_set;
}

void brx_pal_vk_device::write_descriptor_set(brx_pal_descriptor_set *wrapped_descriptor_set, uint32_t dst_binding, BRX_PAL_DESCRIPTOR_TYPE descriptor_type, uint32_t dst_descriptor_start_index, uint32_t src_descriptor_count, brx_pal_uniform_upload_buffer const *const *src_dynamic_uniform_buffers, uint32_t const *src_dynamic_uniform_buffer_ranges, brx_pal_read_only_storage_buffer const *const *src_read_only_storage_buffers, brx_pal_storage_buffer const *const *src_storage_buffers, brx_pal_sampled_image const *const *src_sampled_images, brx_pal_storage_image const *const *src_storage_images, brx_pal_sampler const *const *src_samplers, brx_pal_top_level_acceleration_structure const *const *src_top_level_acceleration_structures)
{
    assert(NULL != wrapped_descriptor_set);
    brx_pal_vk_descriptor_set *const unwrapped_descriptor_set = static_cast<brx_pal_vk_descriptor_set *>(wrapped_descriptor_set);

    unwrapped_descriptor_set->write_descriptor(this->m_pfn_get_device_proc_addr, this->m_device, dst_binding, descriptor_type, dst_descriptor_start_index, src_descriptor_count, src_dynamic_uniform_buffers, src_dynamic_uniform_buffer_ranges, src_read_only_storage_buffers, src_storage_buffers, src_sampled_images, src_storage_images, src_samplers, src_top_level_acceleration_structures);
}

void brx_pal_vk_device::destroy_descriptor_set(brx_pal_descriptor_set *wrapped_descriptor_set)
{
    assert(NULL != wrapped_descriptor_set);
    brx_pal_vk_descriptor_set *delete_unwrapped_descriptor_set = static_cast<brx_pal_vk_descriptor_set *>(wrapped_descriptor_set);

    delete_unwrapped_descriptor_set->uninit(this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);

    delete_unwrapped_descriptor_set->~brx_pal_vk_descriptor_set();
    mcrt_free(delete_unwrapped_descriptor_set);
}

brx_pal_render_pass *brx_pal_vk_device::create_render_pass(uint32_t color_attachment_count, BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const *color_attachments, BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT const *depth_stencil_attachment) const
{
    // NOTE: single subpass is enough
    // input attachment is NOT necessary
    // use VK_ARM_rasterization_order_attachment_access (VK_EXT_rasterization_order_attachment_access) instead

    VkRenderPass new_render_pass = VK_NULL_HANDLE;
    {
        bool require_subpass_dependency = false;

        PFN_vkCreateRenderPass pfn_create_render_pass = reinterpret_cast<PFN_vkCreateRenderPass>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateRenderPass"));
        assert(NULL != pfn_create_render_pass);

        constexpr uint32_t const max_color_attachment_count = 8U;
        assert(color_attachment_count < max_color_attachment_count);
        color_attachment_count = (color_attachment_count < max_color_attachment_count) ? color_attachment_count : max_color_attachment_count;

        VkAttachmentDescription attachments_description[max_color_attachment_count + 1U];
        VkAttachmentReference color_attachments_reference[max_color_attachment_count];
        VkAttachmentReference depth_stencil_attachment_reference;
        assert(NULL != color_attachments || 0U == color_attachment_count);
        for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
        {
            attachments_description[color_attachment_index].flags = 0U;
            attachments_description[color_attachment_index].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments_description[color_attachment_index].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments_description[color_attachment_index].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            switch (color_attachments[color_attachment_index].format)
            {
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8_UNORM:
                attachments_description[color_attachment_index].format = VK_FORMAT_R8G8_UNORM;
                break;
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_UNORM:
                attachments_description[color_attachment_index].format = VK_FORMAT_B8G8R8A8_UNORM;
                break;
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_SRGB:
                attachments_description[color_attachment_index].format = VK_FORMAT_B8G8R8A8_SRGB;
                break;
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_UNORM:
                attachments_description[color_attachment_index].format = VK_FORMAT_R8G8B8A8_UNORM;
                break;
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_SRGB:
                attachments_description[color_attachment_index].format = VK_FORMAT_R8G8B8A8_SRGB;
                break;
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2B10G10R10_UNORM_PACK32:
                attachments_description[color_attachment_index].format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
                break;
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32:
                attachments_description[color_attachment_index].format = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
                break;
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16_SNORM:
                attachments_description[color_attachment_index].format = VK_FORMAT_R16G16_SNORM;
                break;
            case BRX_PAL_COLOR_ATTACHMENT_FORMAT_R16G16B16A16_SFLOAT:
                attachments_description[color_attachment_index].format = VK_FORMAT_R16G16B16A16_SFLOAT;
                break;
            default:
                assert(false);
                attachments_description[color_attachment_index].format = VK_FORMAT_UNDEFINED;
            }

            switch (color_attachments[color_attachment_index].load_operation)
            {
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_DONT_CARE:
                attachments_description[color_attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments_description[color_attachment_index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR:
                attachments_description[color_attachment_index].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments_description[color_attachment_index].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
            default:
                assert(false);
                attachments_description[color_attachment_index].loadOp = static_cast<VkAttachmentLoadOp>(-1);
                attachments_description[color_attachment_index].initialLayout = static_cast<VkImageLayout>(-1);
            }

            switch (color_attachments[color_attachment_index].store_operation)
            {
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_DONT_CARE:
                attachments_description[color_attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments_description[color_attachment_index].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // "VK_IMAGE_LAYOUT_UNDEFINED" can NOT be used for "finalLayout"
                break;
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE:
                attachments_description[color_attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments_description[color_attachment_index].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                require_subpass_dependency = true;
                break;
            case BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_PRESENT:
                attachments_description[color_attachment_index].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments_description[color_attachment_index].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                break;
            default:
                assert(false);
                attachments_description[color_attachment_index].storeOp = static_cast<VkAttachmentStoreOp>(-1);
                attachments_description[color_attachment_index].finalLayout = static_cast<VkImageLayout>(-1);
            }

            color_attachments_reference[color_attachment_index].attachment = color_attachment_index;
            color_attachments_reference[color_attachment_index].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        if (NULL != depth_stencil_attachment)
        {
            attachments_description[color_attachment_count].flags = 0U;
            attachments_description[color_attachment_count].samples = VK_SAMPLE_COUNT_1_BIT;

            switch (depth_stencil_attachment->load_operation)
            {
            case BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_LOAD_OPERATION_DONT_CARE:
                attachments_description[color_attachment_count].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments_description[color_attachment_count].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
            case BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_LOAD_OPERATION_CLEAR:
                attachments_description[color_attachment_count].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments_description[color_attachment_count].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                break;
            default:
                assert(false);
                attachments_description[color_attachment_count].loadOp = static_cast<VkAttachmentLoadOp>(-1);
                attachments_description[color_attachment_count].initialLayout = static_cast<VkImageLayout>(-1);
            }

            switch (depth_stencil_attachment->store_operation)
            {
            case BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_STORE_OPERATION_DONT_CARE:
                attachments_description[color_attachment_count].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachments_description[color_attachment_count].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // "VK_IMAGE_LAYOUT_UNDEFINED" can NOT be used for "finalLayout"
                break;
            case BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE:
                attachments_description[color_attachment_count].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachments_description[color_attachment_count].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                require_subpass_dependency = true;
                break;
            default:
                assert(false);
                attachments_description[color_attachment_count].storeOp = static_cast<VkAttachmentStoreOp>(-1);
                attachments_description[color_attachment_count].finalLayout = static_cast<VkImageLayout>(-1);
            }

            switch (depth_stencil_attachment->format)
            {
            case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT:
                attachments_description[color_attachment_count].format = VK_FORMAT_D32_SFLOAT;
                attachments_description[color_attachment_count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments_description[color_attachment_count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                break;
            case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_X8_D24_UNORM_PACK32:
                attachments_description[color_attachment_count].format = VK_FORMAT_X8_D24_UNORM_PACK32;
                attachments_description[color_attachment_count].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachments_description[color_attachment_count].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                break;
            case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT_S8_UINT:
                attachments_description[color_attachment_count].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                attachments_description[color_attachment_count].stencilLoadOp = attachments_description[color_attachment_count].loadOp;
                attachments_description[color_attachment_count].stencilStoreOp = attachments_description[color_attachment_count].storeOp;
                break;
            case BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D24_UNORM_S8_UINT:
                attachments_description[color_attachment_count].format = VK_FORMAT_D24_UNORM_S8_UINT;
                attachments_description[color_attachment_count].stencilLoadOp = attachments_description[color_attachment_count].loadOp;
                attachments_description[color_attachment_count].stencilStoreOp = attachments_description[color_attachment_count].storeOp;
                break;
            default:
                assert(false);
                attachments_description[color_attachment_count].format = VK_FORMAT_UNDEFINED;
                attachments_description[color_attachment_count].stencilLoadOp = static_cast<VkAttachmentLoadOp>(-1);
                attachments_description[color_attachment_count].stencilStoreOp = static_cast<VkAttachmentStoreOp>(-1);
            }

            depth_stencil_attachment_reference.attachment = color_attachment_count;
            depth_stencil_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription const subpass_description =
            {0U,
             VK_PIPELINE_BIND_POINT_GRAPHICS,
             0U,
             NULL,
             color_attachment_count,
             (color_attachment_count > 0U) ? color_attachments_reference : NULL,
             NULL,
             (NULL != depth_stencil_attachment) ? &depth_stencil_attachment_reference : NULL,
             0U,
             NULL};

        VkPipelineStageFlags const graphics_queue_family_store_destination_stage = (!this->m_support_ray_tracing) ? (g_graphics_queue_family_graphics_compute_pipeline_shader_read_stages) : (g_graphics_queue_family_graphics_compute_pipeline_shader_read_stages | g_graphics_queue_family_ray_tracing_pipeline_shader_read_stages);

        VkSubpassDependency const subpass_dependency = {
            0U,
            VK_SUBPASS_EXTERNAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            graphics_queue_family_store_destination_stage,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_DEPENDENCY_BY_REGION_BIT};

        VkRenderPassCreateInfo render_pass_create_info = {
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            NULL,
            0U,
            color_attachment_count + ((NULL != depth_stencil_attachment) ? 1U : 0U),
            attachments_description,
            1U,
            &subpass_description,
            require_subpass_dependency ? 1U : 0U,
            require_subpass_dependency ? &subpass_dependency : NULL,
        };

        VkResult res_create_render_pass = pfn_create_render_pass(this->m_device, &render_pass_create_info, NULL, &new_render_pass);
        assert(VK_SUCCESS == res_create_render_pass);
    }

    void *new_brx_pal_render_pass_base = mcrt_malloc(sizeof(brx_pal_vk_render_pass), alignof(brx_pal_vk_render_pass));
    assert(NULL != new_brx_pal_render_pass_base);

    brx_pal_vk_render_pass *new_brx_pal_render_pass = new (new_brx_pal_render_pass_base) brx_pal_vk_render_pass{new_render_pass, color_attachment_count};
    return new_brx_pal_render_pass;
}

void brx_pal_vk_device::destroy_render_pass(brx_pal_render_pass *brx_pal_render_pass) const
{
    assert(NULL != brx_pal_render_pass);
    brx_pal_vk_render_pass *delete_render_pass = static_cast<brx_pal_vk_render_pass *>(brx_pal_render_pass);

    VkRenderPass stealed_render_pass = VK_NULL_HANDLE;
    delete_render_pass->steal(&stealed_render_pass);

    delete_render_pass->~brx_pal_vk_render_pass();
    mcrt_free(delete_render_pass);

    PFN_vkDestroyRenderPass pfn_destroy_render_pass = reinterpret_cast<PFN_vkDestroyRenderPass>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyRenderPass"));
    assert(NULL != pfn_destroy_render_pass);

    pfn_destroy_render_pass(this->m_device, stealed_render_pass, this->m_allocation_callbacks);
}

brx_pal_graphics_pipeline *brx_pal_vk_device::create_graphics_pipeline(brx_pal_render_pass const *render_pass, brx_pal_pipeline_layout const *pipeline_layout, size_t vertex_shader_module_code_size, void const *vertex_shader_module_code, size_t fragment_shader_module_code_size, void const *fragment_shader_module_code, bool enable_back_face_cull, bool front_ccw, bool enable_depth_clip, uint32_t forced_sample_count, BRX_PAL_GRAPHICS_PIPELINE_DEPTH_COMPARE_OPERATION depth_compare_operation, BRX_PAL_GRAPHICS_PIPELINE_BLEND_OPERATION blend_operation) const
{
    void *new_unwrapped_graphics_pipeline_base = mcrt_malloc(sizeof(brx_pal_vk_graphics_pipeline), alignof(brx_pal_vk_graphics_pipeline));
    assert(NULL != new_unwrapped_graphics_pipeline_base);

    brx_pal_vk_graphics_pipeline *new_unwrapped_graphics_pipeline = new (new_unwrapped_graphics_pipeline_base) brx_pal_vk_graphics_pipeline{};
    new_unwrapped_graphics_pipeline->init(this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks, render_pass, pipeline_layout, vertex_shader_module_code_size, vertex_shader_module_code, fragment_shader_module_code_size, fragment_shader_module_code, enable_back_face_cull, front_ccw, enable_depth_clip, forced_sample_count, depth_compare_operation, blend_operation);
    return new_unwrapped_graphics_pipeline;
}

void brx_pal_vk_device::destroy_graphics_pipeline(brx_pal_graphics_pipeline *wrapped_graphics_pipeline) const
{
    assert(NULL != wrapped_graphics_pipeline);
    brx_pal_vk_graphics_pipeline *delete_unwrapped_graphics_pipeline = static_cast<brx_pal_vk_graphics_pipeline *>(wrapped_graphics_pipeline);

    delete_unwrapped_graphics_pipeline->uninit(this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);

    delete_unwrapped_graphics_pipeline->~brx_pal_vk_graphics_pipeline();
    mcrt_free(delete_unwrapped_graphics_pipeline);
}

brx_pal_compute_pipeline *brx_pal_vk_device::create_compute_pipeline(brx_pal_pipeline_layout const *pipeline_layout, size_t compute_shader_module_code_size, void const *compute_shader_module_code) const
{
    void *new_unwrapped_compute_pipeline_base = mcrt_malloc(sizeof(brx_pal_vk_compute_pipeline), alignof(brx_pal_vk_compute_pipeline));
    assert(NULL != new_unwrapped_compute_pipeline_base);

    brx_pal_vk_compute_pipeline *new_unwrapped_compute_pipeline = new (new_unwrapped_compute_pipeline_base) brx_pal_vk_compute_pipeline{};
    new_unwrapped_compute_pipeline->init(this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks, pipeline_layout, compute_shader_module_code_size, compute_shader_module_code);
    return new_unwrapped_compute_pipeline;
}

void brx_pal_vk_device::destroy_compute_pipeline(brx_pal_compute_pipeline *wrapped_compute_pipeline) const
{
    assert(NULL != wrapped_compute_pipeline);
    brx_pal_vk_compute_pipeline *delete_unwrapped_compute_pipeline = static_cast<brx_pal_vk_compute_pipeline *>(wrapped_compute_pipeline);

    delete_unwrapped_compute_pipeline->uninit(this->m_pfn_get_device_proc_addr, this->m_device, this->m_allocation_callbacks);

    delete_unwrapped_compute_pipeline->~brx_pal_vk_compute_pipeline();
    mcrt_free(delete_unwrapped_compute_pipeline);
}

brx_pal_frame_buffer *brx_pal_vk_device::create_frame_buffer(brx_pal_render_pass const *brx_pal_render_pass, uint32_t width, uint32_t height, uint32_t color_attachment_count, brx_pal_color_attachment_image const *const *color_attachments, brx_pal_depth_stencil_attachment_image const *depth_stencil_attachment) const
{
    assert(NULL != brx_pal_render_pass);
    VkRenderPass render_pass = static_cast<brx_pal_vk_render_pass const *>(brx_pal_render_pass)->get_render_pass();

    VkFramebuffer new_frame_buffer = VK_NULL_HANDLE;
    {
        PFN_vkCreateFramebuffer pfn_create_frame_buffer = reinterpret_cast<PFN_vkCreateFramebuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateFramebuffer"));

        constexpr uint32_t const max_color_attachment_count = 8U;
        assert(color_attachment_count < max_color_attachment_count);
        color_attachment_count = (color_attachment_count < max_color_attachment_count) ? color_attachment_count : max_color_attachment_count;

        VkImageView attachments[max_color_attachment_count + 1U];
        for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
        {
            assert(NULL != color_attachments[color_attachment_index]);
            attachments[color_attachment_index] = static_cast<brx_pal_vk_color_attachment_image const *>(color_attachments[color_attachment_index])->get_attachment_image_view();
        }

        if (NULL != depth_stencil_attachment)
        {
            attachments[color_attachment_count] = static_cast<brx_pal_vk_depth_stencil_attachment_image const *>(depth_stencil_attachment)->get_attachment_image_view();
        }

        uint32_t attachment_count = color_attachment_count + ((NULL != depth_stencil_attachment) ? 1U : 0U);

        VkFramebufferCreateInfo frame_buffer_create_info = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            NULL,
            0U,
            render_pass,
            attachment_count,
            attachments,
            width,
            height,
            1U};

        VkResult res_create_framebuffer = pfn_create_frame_buffer(this->m_device, &frame_buffer_create_info, this->m_allocation_callbacks, &new_frame_buffer);
        assert(VK_SUCCESS == res_create_framebuffer);
    }

    void *new_brx_pal_frame_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_frame_buffer), alignof(brx_pal_vk_frame_buffer));
    assert(NULL != new_brx_pal_frame_buffer_base);

    brx_pal_vk_frame_buffer *new_brx_pal_frame_buffer = new (new_brx_pal_frame_buffer_base) brx_pal_vk_frame_buffer{new_frame_buffer};
    return new_brx_pal_frame_buffer;
}

void brx_pal_vk_device::destroy_frame_buffer(brx_pal_frame_buffer *brx_pal_frame_buffer) const
{
    assert(NULL != brx_pal_frame_buffer);
    brx_pal_vk_frame_buffer *delete_frame_buffer = static_cast<brx_pal_vk_frame_buffer *>(brx_pal_frame_buffer);

    VkFramebuffer stealed_frame_buffer = VK_NULL_HANDLE;
    delete_frame_buffer->steal(&stealed_frame_buffer);

    delete_frame_buffer->~brx_pal_vk_frame_buffer();
    mcrt_free(delete_frame_buffer);

    PFN_vkDestroyFramebuffer pfn_destroy_framebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyFramebuffer"));
    assert(NULL != pfn_destroy_framebuffer);

    pfn_destroy_framebuffer(this->m_device, stealed_frame_buffer, this->m_allocation_callbacks);
}

uint32_t brx_pal_vk_device::get_uniform_upload_buffer_offset_alignment() const
{
    return this->m_min_uniform_buffer_offset_alignment;
}

brx_pal_uniform_upload_buffer *brx_pal_vk_device::create_uniform_upload_buffer(uint32_t size) const
{
    void *new_unwrapped_uniform_upload_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_uniform_upload_buffer), alignof(brx_pal_vk_uniform_upload_buffer));
    assert(NULL != new_unwrapped_uniform_upload_buffer_base);

    brx_pal_vk_uniform_upload_buffer *new_unwrapped_uniform_upload_buffer = new (new_unwrapped_uniform_upload_buffer_base) brx_pal_vk_uniform_upload_buffer{};
    new_unwrapped_uniform_upload_buffer->init(this->m_memory_allocator, this->m_uniform_upload_buffer_memory_pool, size);
    return new_unwrapped_uniform_upload_buffer;
}

void brx_pal_vk_device::destroy_uniform_upload_buffer(brx_pal_uniform_upload_buffer *wrapped_uniform_upload_buffer) const
{
    assert(NULL != wrapped_uniform_upload_buffer);
    brx_pal_vk_uniform_upload_buffer *delete_unwrapped_uniform_upload_buffer = static_cast<brx_pal_vk_uniform_upload_buffer *>(wrapped_uniform_upload_buffer);

    delete_unwrapped_uniform_upload_buffer->uninit(this->m_memory_allocator);

    delete_unwrapped_uniform_upload_buffer->~brx_pal_vk_uniform_upload_buffer();
    mcrt_free(delete_unwrapped_uniform_upload_buffer);
}

uint32_t brx_pal_vk_device::get_staging_upload_buffer_offset_alignment() const
{
    return this->m_optimal_buffer_copy_offset_alignment;
}

uint32_t brx_pal_vk_device::get_staging_upload_buffer_row_pitch_alignment() const
{
    return this->m_optimal_buffer_copy_row_pitch_alignment;
}

brx_pal_staging_upload_buffer *brx_pal_vk_device::create_staging_upload_buffer(uint32_t size) const
{
    void *new_unwrapped_staging_upload_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_staging_upload_buffer), alignof(brx_pal_vk_staging_upload_buffer));
    assert(NULL != new_unwrapped_staging_upload_buffer_base);

    brx_pal_vk_staging_upload_buffer *new_unwrapped_staging_upload_buffer = new (new_unwrapped_staging_upload_buffer_base) brx_pal_vk_staging_upload_buffer{};
    new_unwrapped_staging_upload_buffer->init(this->m_memory_allocator, this->m_staging_upload_buffer_memory_pool, size);
    return new_unwrapped_staging_upload_buffer;
}

void brx_pal_vk_device::destroy_staging_upload_buffer(brx_pal_staging_upload_buffer *wrapped_staging_upload_buffer) const
{
    assert(NULL != wrapped_staging_upload_buffer);
    brx_pal_vk_staging_upload_buffer *delete_unwrapped_staging_upload_buffer = static_cast<brx_pal_vk_staging_upload_buffer *>(wrapped_staging_upload_buffer);

    delete_unwrapped_staging_upload_buffer->uninit(this->m_memory_allocator);

    delete_unwrapped_staging_upload_buffer->~brx_pal_vk_staging_upload_buffer();
    mcrt_free(delete_unwrapped_staging_upload_buffer);
}

brx_pal_storage_intermediate_buffer *brx_pal_vk_device::create_storage_intermediate_buffer(uint32_t size) const
{
    void *new_unwrapped_storage_intermediate_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_storage_intermediate_buffer), alignof(brx_pal_vk_storage_intermediate_buffer));
    assert(NULL != new_unwrapped_storage_intermediate_buffer_base);

    brx_pal_vk_storage_intermediate_buffer *new_unwrapped_storage_intermediate_buffer = new (new_unwrapped_storage_intermediate_buffer_base) brx_pal_vk_storage_intermediate_buffer{};
    new_unwrapped_storage_intermediate_buffer->init(this->m_support_ray_tracing, this->m_device, this->m_pfn_get_buffer_device_address, this->m_memory_allocator, this->m_storage_intermediate_buffer_memory_pool, size);
    return new_unwrapped_storage_intermediate_buffer;
}

void brx_pal_vk_device::destroy_storage_intermediate_buffer(brx_pal_storage_intermediate_buffer *wrapped_storage_intermediate_buffer) const
{
    assert(NULL != wrapped_storage_intermediate_buffer);
    brx_pal_vk_storage_intermediate_buffer *delete_wrapped_storage_intermediate_buffer = static_cast<brx_pal_vk_storage_intermediate_buffer *>(wrapped_storage_intermediate_buffer);

    delete_wrapped_storage_intermediate_buffer->uninit(this->m_memory_allocator);

    delete_wrapped_storage_intermediate_buffer->~brx_pal_vk_storage_intermediate_buffer();
    mcrt_free(delete_wrapped_storage_intermediate_buffer);
}

brx_pal_storage_asset_buffer *brx_pal_vk_device::create_storage_asset_buffer(uint32_t size) const
{
    void *new_unwrapped_storage_asset_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_storage_asset_buffer), alignof(brx_pal_vk_storage_asset_buffer));
    assert(NULL != new_unwrapped_storage_asset_buffer_base);

    brx_pal_vk_storage_asset_buffer *new_unwrapped_storage_asset_buffer = new (new_unwrapped_storage_asset_buffer_base) brx_pal_vk_storage_asset_buffer{};
    new_unwrapped_storage_asset_buffer->init(this->m_support_ray_tracing, this->m_device, this->m_pfn_get_buffer_device_address, this->m_memory_allocator, this->m_storage_asset_buffer_memory_pool, size);
    return new_unwrapped_storage_asset_buffer;
}

void brx_pal_vk_device::destroy_storage_asset_buffer(brx_pal_storage_asset_buffer *wrapped_asset_storage_buffer) const
{
    assert(NULL != wrapped_asset_storage_buffer);
    brx_pal_vk_storage_asset_buffer *delete_wrapped_storage_asset_buffer = static_cast<brx_pal_vk_storage_asset_buffer *>(wrapped_asset_storage_buffer);

    delete_wrapped_storage_asset_buffer->uninit(this->m_memory_allocator);

    delete_wrapped_storage_asset_buffer->~brx_pal_vk_storage_asset_buffer();
    mcrt_free(delete_wrapped_storage_asset_buffer);
}

brx_pal_color_attachment_image *brx_pal_vk_device::create_color_attachment_image(BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT wrapped_color_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image) const
{
    void *new_unwrapped_color_attachment_image_base = mcrt_malloc(sizeof(brx_pal_vk_color_attachment_intermediate_image), alignof(brx_pal_vk_color_attachment_intermediate_image));
    assert(NULL != new_unwrapped_color_attachment_image_base);

    brx_pal_vk_color_attachment_intermediate_image *new_unwrapped_color_attachment_image = new (new_unwrapped_color_attachment_image_base) brx_pal_vk_color_attachment_intermediate_image{};
    new_unwrapped_color_attachment_image->init(this->m_device, this->m_pfn_create_image_view, this->m_allocation_callbacks, this->m_memory_allocator, this->m_color_transient_attachment_image_memory_pool, this->m_color_attachment_sampled_image_memory_pool, wrapped_color_attachment_image_format, width, height, allow_sampled_image);
    return new_unwrapped_color_attachment_image;
}

void brx_pal_vk_device::destroy_color_attachment_image(brx_pal_color_attachment_image *wrapped_color_attachment_image) const
{
    assert(NULL != wrapped_color_attachment_image);
    brx_pal_vk_color_attachment_intermediate_image *delete_unwrapped_color_attachment_image = static_cast<brx_pal_vk_color_attachment_intermediate_image *>(wrapped_color_attachment_image);

    delete_unwrapped_color_attachment_image->uninit(this->m_device, this->m_pfn_destroy_image_view, this->m_allocation_callbacks, this->m_memory_allocator);

    delete_unwrapped_color_attachment_image->~brx_pal_vk_color_attachment_intermediate_image();
    mcrt_free(delete_unwrapped_color_attachment_image);
}

BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT brx_pal_vk_device::get_depth_attachment_image_format() const
{
    BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT brx_pal_depth_stencil_attachment_image_format;

    switch (this->m_depth_attachment_image_format)
    {
    case VK_FORMAT_D32_SFLOAT:
        brx_pal_depth_stencil_attachment_image_format = BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT;
        break;
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        brx_pal_depth_stencil_attachment_image_format = BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_X8_D24_UNORM_PACK32;
        break;
    default:
        assert(false);
        brx_pal_depth_stencil_attachment_image_format = static_cast<BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT>(-1);
    }

    return brx_pal_depth_stencil_attachment_image_format;
}

BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT brx_pal_vk_device::get_depth_stencil_attachment_image_format() const
{
    BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT brx_pal_depth_stencil_attachment_image_format;

    switch (this->m_depth_attachment_image_format)
    {
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        brx_pal_depth_stencil_attachment_image_format = BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D32_SFLOAT_S8_UINT;
        break;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        brx_pal_depth_stencil_attachment_image_format = BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_D24_UNORM_S8_UINT;
        break;
    default:
        assert(false);
        brx_pal_depth_stencil_attachment_image_format = static_cast<BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT>(-1);
    }

    return brx_pal_depth_stencil_attachment_image_format;
}

brx_pal_depth_stencil_attachment_image *brx_pal_vk_device::create_depth_stencil_attachment_image(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT wrapped_depth_stencil_attachment_image_format, uint32_t width, uint32_t height, bool allow_sampled_image) const
{
    void *new_unwrapped_depth_stencil_attachment_image_base = mcrt_malloc(sizeof(brx_pal_vk_depth_stencil_attachment_intermediate_image), alignof(brx_pal_vk_depth_stencil_attachment_intermediate_image));
    assert(NULL != new_unwrapped_depth_stencil_attachment_image_base);

    brx_pal_vk_depth_stencil_attachment_intermediate_image *new_unwrapped_depth_stencil_attachment_image = new (new_unwrapped_depth_stencil_attachment_image_base) brx_pal_vk_depth_stencil_attachment_intermediate_image{};
    new_unwrapped_depth_stencil_attachment_image->init(this->m_device, this->m_pfn_create_image_view, this->m_allocation_callbacks, this->m_memory_allocator, this->m_depth_transient_attachment_image_memory_pool, this->m_depth_attachment_sampled_image_memory_pool, this->m_depth_stencil_transient_attachment_image_memory_pool, this->m_depth_stencil_attachment_sampled_image_memory_pool, wrapped_depth_stencil_attachment_image_format, width, height, allow_sampled_image);
    return new_unwrapped_depth_stencil_attachment_image;
}

void brx_pal_vk_device::destroy_depth_stencil_attachment_image(brx_pal_depth_stencil_attachment_image *wrapped_depth_stencil_attachment_image) const
{
    assert(NULL != wrapped_depth_stencil_attachment_image);
    brx_pal_vk_depth_stencil_attachment_intermediate_image *delete_unwrapped_depth_stencil_attachment_image = static_cast<brx_pal_vk_depth_stencil_attachment_intermediate_image *>(wrapped_depth_stencil_attachment_image);

    delete_unwrapped_depth_stencil_attachment_image->uninit(this->m_device, this->m_pfn_destroy_image_view, this->m_allocation_callbacks, this->m_memory_allocator);

    delete_unwrapped_depth_stencil_attachment_image->~brx_pal_vk_depth_stencil_attachment_intermediate_image();
    mcrt_free(delete_unwrapped_depth_stencil_attachment_image);
}

brx_pal_storage_image *brx_pal_vk_device::create_storage_image(BRX_PAL_STORAGE_IMAGE_FORMAT wrapped_storage_image_format, uint32_t width, uint32_t height, bool volume, uint32_t depth, bool allow_sampled_image) const
{
    void *new_unwrapped_storage_image_base = mcrt_malloc(sizeof(brx_pal_vk_storage_intermediate_image), alignof(brx_pal_vk_storage_intermediate_image));
    assert(NULL != new_unwrapped_storage_image_base);

    brx_pal_vk_storage_intermediate_image *new_unwrapped_storage_image = new (new_unwrapped_storage_image_base) brx_pal_vk_storage_intermediate_image{};
    new_unwrapped_storage_image->init(this->m_device, this->m_pfn_create_image_view, this->m_allocation_callbacks, this->m_memory_allocator, this->m_storage_intermediate_image_memory_pool, wrapped_storage_image_format, width, height, volume, depth, allow_sampled_image);
    return new_unwrapped_storage_image;
}

void brx_pal_vk_device::destroy_storage_image(brx_pal_storage_image *wrapped_storage_image) const
{
    assert(NULL != wrapped_storage_image);
    brx_pal_vk_storage_intermediate_image *delete_unwrapped_storage_image = static_cast<brx_pal_vk_storage_intermediate_image *>(wrapped_storage_image);

    delete_unwrapped_storage_image->uninit(this->m_device, this->m_pfn_destroy_image_view, this->m_allocation_callbacks, this->m_memory_allocator);

    delete_unwrapped_storage_image->~brx_pal_vk_storage_intermediate_image();
    mcrt_free(delete_unwrapped_storage_image);
}

bool brx_pal_vk_device::is_sampled_asset_image_compression_bc_supported() const
{
    return this->m_physical_device_feature_texture_compression_BC;
}

bool brx_pal_vk_device::is_sampled_asset_image_compression_astc_supported() const
{
    return this->m_physical_device_feature_texture_compression_ASTC_LDR;
}

brx_pal_sampled_asset_image *brx_pal_vk_device::create_sampled_asset_image(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT wrapped_sampled_asset_image_format, uint32_t width, uint32_t height, bool array, uint32_t array_layers, uint32_t mip_levels) const
{
    VkFormat unwrapped_sampled_asset_image_format;
    switch (wrapped_sampled_asset_image_format)
    {
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8_UNORM:
        unwrapped_sampled_asset_image_format = VK_FORMAT_R8_UNORM;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM:
        unwrapped_sampled_asset_image_format = VK_FORMAT_R8G8B8A8_UNORM;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_SRGB:
        unwrapped_sampled_asset_image_format = VK_FORMAT_R8G8B8A8_SRGB;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT:
        unwrapped_sampled_asset_image_format = VK_FORMAT_R16_SFLOAT;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT:
        unwrapped_sampled_asset_image_format = VK_FORMAT_R16G16_SFLOAT;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
        unwrapped_sampled_asset_image_format = VK_FORMAT_R16G16B16A16_SFLOAT;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_UNORM_BLOCK:
        unwrapped_sampled_asset_image_format = VK_FORMAT_BC7_UNORM_BLOCK;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_SRGB_BLOCK:
        unwrapped_sampled_asset_image_format = VK_FORMAT_BC7_SRGB_BLOCK;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_UNORM_BLOCK:
        unwrapped_sampled_asset_image_format = VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        break;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_SRGB_BLOCK:
        unwrapped_sampled_asset_image_format = VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        break;
    default:
        assert(false);
        unwrapped_sampled_asset_image_format = VK_FORMAT_UNDEFINED;
    }

    void *new_brx_pal_sampled_asset_image_base = mcrt_malloc(sizeof(brx_pal_vk_sampled_asset_image), alignof(brx_pal_vk_sampled_asset_image));
    assert(NULL != new_brx_pal_sampled_asset_image_base);

    brx_pal_vk_sampled_asset_image *new_brx_pal_sampled_asset_image = new (new_brx_pal_sampled_asset_image_base) brx_pal_vk_sampled_asset_image{};

    new_brx_pal_sampled_asset_image->init(this->m_device, this->m_pfn_create_image_view, this->m_allocation_callbacks, this->m_memory_allocator, this->m_sampled_asset_image_memory_pool, unwrapped_sampled_asset_image_format, width, height, array, array_layers, mip_levels);

    return new_brx_pal_sampled_asset_image;
}

void brx_pal_vk_device::destroy_sampled_asset_image(brx_pal_sampled_asset_image *wrapped_sampled_asset_image) const
{
    assert(NULL != wrapped_sampled_asset_image);
    brx_pal_vk_sampled_asset_image *delete_unwrapped_sampled_asset_image = static_cast<brx_pal_vk_sampled_asset_image *>(wrapped_sampled_asset_image);

    delete_unwrapped_sampled_asset_image->uninit(this->m_device, this->m_pfn_destroy_image_view, this->m_allocation_callbacks, this->m_memory_allocator);

    delete_unwrapped_sampled_asset_image->~brx_pal_vk_sampled_asset_image();
    mcrt_free(delete_unwrapped_sampled_asset_image);
}

brx_pal_sampler *brx_pal_vk_device::create_sampler(BRX_PAL_SAMPLER_FILTER wrapped_filter, BRX_PAL_SAMPLER_ADDRESS_MODE wrapped_address_mode) const
{
    VkFilter unwrapped_filter;
    VkSamplerMipmapMode unwrapped_mipmap_mode;
    switch (wrapped_filter)
    {
    case BRX_PAL_SAMPLER_FILTER_NEAREST:
        unwrapped_filter = VK_FILTER_NEAREST;
        unwrapped_mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        break;
    case BRX_PAL_SAMPLER_FILTER_LINEAR:
        unwrapped_filter = VK_FILTER_LINEAR;
        unwrapped_mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        break;
    default:
        assert(false);
        unwrapped_filter = static_cast<VkFilter>(-1);
        unwrapped_mipmap_mode = static_cast<VkSamplerMipmapMode>(-1);
    }

    VkSamplerAddressMode unwrapped_address_mode;
    switch (wrapped_address_mode)
    {
    case BRX_PAL_SAMPLER_ADDRESS_MODE_WRAP:
        unwrapped_address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    case BRX_PAL_SAMPLER_ADDRESS_MODE_CLAMP:
        unwrapped_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        break;
    default:
        assert(false);
        unwrapped_address_mode = static_cast<VkSamplerAddressMode>(-1);
    }

    VkSampler new_sampler = VK_NULL_HANDLE;
    {
        PFN_vkCreateSampler pfn_create_sampler = reinterpret_cast<PFN_vkCreateSampler>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateSampler"));
        assert(NULL != pfn_create_sampler);

        VkSamplerCreateInfo sampler_create_info;
        sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_create_info.pNext = NULL;
        sampler_create_info.flags = 0U;
        sampler_create_info.magFilter = unwrapped_filter;
        sampler_create_info.minFilter = unwrapped_filter;
        sampler_create_info.mipmapMode = unwrapped_mipmap_mode;
        sampler_create_info.addressModeU = unwrapped_address_mode;
        sampler_create_info.addressModeV = unwrapped_address_mode;
        sampler_create_info.addressModeW = unwrapped_address_mode;
        sampler_create_info.mipLodBias = 0.0F;
        sampler_create_info.anisotropyEnable = VK_FALSE;
        sampler_create_info.maxAnisotropy = 1U;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
        sampler_create_info.minLod = 0.0F;
        sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
        sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;
        VkResult res_create_sampler = pfn_create_sampler(this->m_device, &sampler_create_info, this->m_allocation_callbacks, &new_sampler);
        assert(VK_SUCCESS == res_create_sampler);
    }

    void *new_brx_pal_sampler_base = mcrt_malloc(sizeof(brx_pal_vk_sampler), alignof(brx_pal_vk_sampler));
    assert(NULL != new_brx_pal_sampler_base);

    brx_pal_vk_sampler *new_brx_pal_sampler = new (new_brx_pal_sampler_base) brx_pal_vk_sampler{new_sampler};
    return new_brx_pal_sampler;
}

void brx_pal_vk_device::destroy_sampler(brx_pal_sampler *brx_pal_sampler) const
{
    assert(NULL != brx_pal_sampler);
    brx_pal_vk_sampler *delete_sampler = static_cast<brx_pal_vk_sampler *>(brx_pal_sampler);

    VkSampler stealed_sampler = VK_NULL_HANDLE;
    delete_sampler->steal(&stealed_sampler);

    delete_sampler->~brx_pal_vk_sampler();
    mcrt_free(delete_sampler);

    PFN_vkDestroySampler pfn_destroy_sampler = reinterpret_cast<PFN_vkDestroySampler>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroySampler"));
    assert(NULL != pfn_destroy_sampler);

    pfn_destroy_sampler(this->m_device, stealed_sampler, this->m_allocation_callbacks);
}

brx_pal_surface *brx_pal_vk_device::create_surface(void *wsi_window) const
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    PFN_vkCreateAndroidSurfaceKHR const pfn_create_android_surface = reinterpret_cast<PFN_vkCreateAndroidSurfaceKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateAndroidSurfaceKHR"));
    assert(NULL != pfn_create_android_surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    PFN_vkCreateXcbSurfaceKHR const pfn_create_xcb_surface = reinterpret_cast<PFN_vkCreateXcbSurfaceKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateXcbSurfaceKHR"));
    assert(NULL != pfn_create_xcb_surface);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    PFN_vkCreateMetalSurfaceEXT const pfn_create_metal_surface = reinterpret_cast<PFN_vkCreateMetalSurfaceEXT>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateMetalSurfaceEXT"));
    assert(NULL != pfn_create_metal_surface);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    PFN_vkCreateWin32SurfaceKHR const pfn_create_win32_surface = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkCreateWin32SurfaceKHR"));
    assert(NULL != pfn_create_win32_surface);
#else
#error Unknown Platform
#endif
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfn_get_physical_device_surface_support = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    assert(NULL != pfn_get_physical_device_surface_support);

    void *new_brx_pal_surface_base = mcrt_malloc(sizeof(brx_pal_vk_surface), alignof(brx_pal_vk_surface));
    assert(NULL != new_brx_pal_surface_base);

    brx_pal_vk_surface *new_brx_pal_surface = new (new_brx_pal_surface_base) brx_pal_vk_surface{};

    new_brx_pal_surface->init(
        this->m_instance,
        this->m_physical_device,
        this->m_graphics_queue_family_index,
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        pfn_create_android_surface,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        pfn_create_xcb_surface,
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        pfn_create_metal_surface,
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        pfn_create_win32_surface,
#else
#error Unknown Platform
#endif
        pfn_get_physical_device_surface_support,
        this->m_allocation_callbacks,
        wsi_window);

    return new_brx_pal_surface;
}

void brx_pal_vk_device::destroy_surface(brx_pal_surface *wrapped_surface) const
{
    PFN_vkDestroySurfaceKHR pfn_destroy_surface = reinterpret_cast<PFN_vkDestroySurfaceKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkDestroySurfaceKHR"));
    assert(NULL != pfn_destroy_surface);

    brx_pal_vk_surface *delete_unwrapped_surface = static_cast<brx_pal_vk_surface *>(wrapped_surface);

    delete_unwrapped_surface->uninit(this->m_instance, pfn_destroy_surface, this->m_allocation_callbacks);

    delete_unwrapped_surface->~brx_pal_vk_surface();
    mcrt_free(delete_unwrapped_surface);
}

brx_pal_swap_chain *brx_pal_vk_device::create_swap_chain(brx_pal_surface *wrapped_surface) const
{
    assert(NULL != wrapped_surface);
    VkSurfaceKHR surface = static_cast<brx_pal_vk_surface *>(wrapped_surface)->get_surface();

    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pfn_get_physical_device_surface_formats = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    assert(NULL != pfn_get_physical_device_surface_formats);

    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfn_get_physical_device_surface_capabilities = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    assert(NULL != pfn_get_physical_device_surface_capabilities);

    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pfn_get_physical_device_surface_present_modes = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(this->m_pfn_get_instance_proc_addr(this->m_instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));
    assert(NULL != pfn_get_physical_device_surface_present_modes);

    PFN_vkCreateSwapchainKHR pfn_create_swap_chain = reinterpret_cast<PFN_vkCreateSwapchainKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateSwapchainKHR"));
    assert(NULL != pfn_create_swap_chain);

    PFN_vkGetSwapchainImagesKHR pfn_get_swap_chain_images = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetSwapchainImagesKHR"));
    assert(NULL != pfn_get_swap_chain_images);

    void *new_brx_pal_swap_chain_base = mcrt_malloc(sizeof(brx_pal_vk_swap_chain), alignof(brx_pal_vk_swap_chain));
    assert(NULL != new_brx_pal_swap_chain_base);

    brx_pal_vk_swap_chain *new_brx_pal_swap_chain = new (new_brx_pal_swap_chain_base) brx_pal_vk_swap_chain{};

    new_brx_pal_swap_chain->init(this->m_device, this->m_physical_device, pfn_get_physical_device_surface_formats, pfn_get_physical_device_surface_capabilities, pfn_get_physical_device_surface_present_modes, pfn_create_swap_chain, pfn_get_swap_chain_images, this->m_pfn_create_image_view, m_allocation_callbacks, surface);

    return new_brx_pal_swap_chain;
}

bool brx_pal_vk_device::acquire_next_image(brx_pal_graphics_command_buffer *brx_pal_graphics_command_buffer, brx_pal_swap_chain const *brx_pal_swap_chain, uint32_t *out_swap_chain_image_index) const
{
    assert(NULL != brx_pal_graphics_command_buffer);
    assert(NULL != brx_pal_swap_chain);
    assert(NULL != out_swap_chain_image_index);
    VkSemaphore acquire_next_image_semaphore = static_cast<brx_pal_vk_graphics_command_buffer const *>(brx_pal_graphics_command_buffer)->get_acquire_next_image_semaphore();
    VkSwapchainKHR swap_chain = static_cast<brx_pal_vk_swap_chain const *>(brx_pal_swap_chain)->get_swap_chain();

    VkResult res_acquire_next_image = this->m_pfn_acquire_next_image(this->m_device, swap_chain, UINT64_MAX, acquire_next_image_semaphore, VK_NULL_HANDLE, out_swap_chain_image_index);
    switch (res_acquire_next_image)
    {
    case VK_SUCCESS:
        return true;
    case VK_SUBOPTIMAL_KHR:
        // TODO:
        return true;
    case VK_ERROR_OUT_OF_DATE_KHR:
        return false;
    default:
        assert(false);
        return false;
    }
}

void brx_pal_vk_device::destroy_swap_chain(brx_pal_swap_chain *wrapped_swap_chain) const
{
    PFN_vkDestroySwapchainKHR pfn_destroy_swapchain = reinterpret_cast<PFN_vkDestroySwapchainKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroySwapchainKHR"));
    assert(NULL != pfn_destroy_swapchain);

    assert(NULL != wrapped_swap_chain);
    brx_pal_vk_swap_chain *delete_unwrapped_swap_chain = static_cast<brx_pal_vk_swap_chain *>(wrapped_swap_chain);

    delete_unwrapped_swap_chain->uninit(this->m_device, pfn_destroy_swapchain, this->m_pfn_destroy_image_view, this->m_allocation_callbacks);

    delete_unwrapped_swap_chain->~brx_pal_vk_swap_chain();
    mcrt_free(delete_unwrapped_swap_chain);
}

brx_pal_scratch_buffer *brx_pal_vk_device::create_scratch_buffer(uint32_t size) const
{
    void *new_unwrapped_scratch_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_scratch_buffer), alignof(brx_pal_vk_scratch_buffer));
    assert(NULL != new_unwrapped_scratch_buffer_base);

    brx_pal_vk_scratch_buffer *new_unwrapped_scratch_buffer = new (new_unwrapped_scratch_buffer_base) brx_pal_vk_scratch_buffer{};
    new_unwrapped_scratch_buffer->init(this->m_device, this->m_pfn_get_buffer_device_address, this->m_memory_allocator, this->m_scratch_buffer_memory_pool, size);
    return new_unwrapped_scratch_buffer;
}

void brx_pal_vk_device::destroy_scratch_buffer(brx_pal_scratch_buffer *wrapped_scratch_buffer) const
{
    assert(NULL != wrapped_scratch_buffer);
    brx_pal_vk_scratch_buffer *delete_unwrapped_scratch_buffer = static_cast<brx_pal_vk_scratch_buffer *>(wrapped_scratch_buffer);

    delete_unwrapped_scratch_buffer->uninit(this->m_memory_allocator);

    delete_unwrapped_scratch_buffer->~brx_pal_vk_scratch_buffer();
    mcrt_free(delete_unwrapped_scratch_buffer);
}

void brx_pal_vk_device::get_intermediate_bottom_level_acceleration_structure_size(uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *wrapped_bottom_level_acceleration_structure_geometries, uint32_t *intermediate_bottom_level_acceleration_structure_size, uint32_t *build_scratch_size, uint32_t *update_scratch_size) const
{
    assert(NULL != wrapped_bottom_level_acceleration_structure_geometries);
    assert(NULL != intermediate_bottom_level_acceleration_structure_size);
    assert(NULL != build_scratch_size);

    PFN_vkGetAccelerationStructureBuildSizesKHR const pfn_get_acceleration_structure_build_sizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetAccelerationStructureBuildSizesKHR"));
    assert(NULL != pfn_get_acceleration_structure_build_sizes);

    mcrt_vector<VkAccelerationStructureGeometryKHR> acceleration_structure_geometries(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));
    mcrt_vector<uint32_t> max_primitive_counts(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));
    for (uint32_t acceleration_structure_geometry_index = 0U; acceleration_structure_geometry_index < bottom_level_acceleration_structure_geometry_count; ++acceleration_structure_geometry_index)
    {
        BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const &wrapped_bottom_level_acceleration_structure_geometry = wrapped_bottom_level_acceleration_structure_geometries[acceleration_structure_geometry_index];

        VkFormat vertex_position_attribute_format;
        switch (wrapped_bottom_level_acceleration_structure_geometry.vertex_position_attribute_format)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_VERTEX_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT:
            vertex_position_attribute_format = VK_FORMAT_R32G32B32_SFLOAT;
            break;
        default:
            // VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR
            assert(false);
            vertex_position_attribute_format = static_cast<VkFormat>(-1);
            break;
        }

        VkDeviceAddress const vertex_position_buffer_device_memory_range_base = static_cast<brx_pal_vk_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.vertex_position_buffer)->get_device_memory_range_base();

        VkIndexType index_type;
        switch (wrapped_bottom_level_acceleration_structure_geometry.index_type)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT32:
            index_type = VK_INDEX_TYPE_UINT32;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT16:
            index_type = VK_INDEX_TYPE_UINT16;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE:
            index_type = VK_INDEX_TYPE_NONE_KHR;
            break;
        default:
            assert(false);
            index_type = static_cast<VkIndexType>(-1);
        }

        VkDeviceAddress const index_buffer_device_memory_range_base = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? static_cast<brx_pal_vk_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.index_buffer)->get_device_memory_range_base() : 0U;

        acceleration_structure_geometries[acceleration_structure_geometry_index] = VkAccelerationStructureGeometryKHR{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            NULL,
            VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            {.triangles =
                 {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                  NULL,
                  vertex_position_attribute_format,
                  {.deviceAddress = vertex_position_buffer_device_memory_range_base},
                  wrapped_bottom_level_acceleration_structure_geometry.vertex_position_binding_stride,
                  (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? (wrapped_bottom_level_acceleration_structure_geometry.index_count - 1U) : (wrapped_bottom_level_acceleration_structure_geometry.vertex_count - 1U),
                  index_type,
                  {.deviceAddress = index_buffer_device_memory_range_base},
                  {.deviceAddress = 0U}}},
            wrapped_bottom_level_acceleration_structure_geometry.force_closest_hit ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0U};

        assert(0U == ((BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? (wrapped_bottom_level_acceleration_structure_geometry.index_count % 3U) : (wrapped_bottom_level_acceleration_structure_geometry.vertex_count % 3U)));
        max_primitive_counts[acceleration_structure_geometry_index] = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? (wrapped_bottom_level_acceleration_structure_geometry.index_count / 3U) : (wrapped_bottom_level_acceleration_structure_geometry.vertex_count / 3U);
    }

    VkAccelerationStructureBuildGeometryInfoKHR const acceleration_structure_build_geometry_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        NULL,
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        static_cast<uint32_t>(acceleration_structure_geometries.size()),
        &acceleration_structure_geometries[0],
        NULL,
        {.deviceAddress = 0U}};

    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_size_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        NULL,
        static_cast<VkDeviceSize>(-1),
        static_cast<VkDeviceSize>(-1),
        static_cast<VkDeviceSize>(-1)};

    pfn_get_acceleration_structure_build_sizes(this->m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &acceleration_structure_build_geometry_info, &max_primitive_counts[0], &acceleration_structure_build_size_info);

    (*intermediate_bottom_level_acceleration_structure_size) = static_cast<uint32_t>(acceleration_structure_build_size_info.accelerationStructureSize);
    (*build_scratch_size) = static_cast<uint32_t>(acceleration_structure_build_size_info.buildScratchSize);
    (*update_scratch_size) = static_cast<uint32_t>(acceleration_structure_build_size_info.updateScratchSize);
}

brx_pal_intermediate_bottom_level_acceleration_structure *brx_pal_vk_device::create_intermediate_bottom_level_acceleration_structure(uint32_t size) const
{
    void *new_unwrapped_intermediate_bottom_level_acceleration_structure_base = mcrt_malloc(sizeof(brx_pal_vk_intermediate_bottom_level_acceleration_structure), alignof(brx_pal_vk_intermediate_bottom_level_acceleration_structure));
    assert(NULL != new_unwrapped_intermediate_bottom_level_acceleration_structure_base);

    brx_pal_vk_intermediate_bottom_level_acceleration_structure *new_unwrapped_intermediate_bottom_level_acceleration_structure = new (new_unwrapped_intermediate_bottom_level_acceleration_structure_base) brx_pal_vk_intermediate_bottom_level_acceleration_structure{};
    new_unwrapped_intermediate_bottom_level_acceleration_structure->init(this->m_device, this->m_pfn_create_acceleration_structure, this->m_pfn_get_acceleration_structure_device_address, this->m_allocation_callbacks, this->m_memory_allocator, this->m_intermediate_bottom_level_acceleration_structure_memory_pool, size);
    return new_unwrapped_intermediate_bottom_level_acceleration_structure;
}

void brx_pal_vk_device::destroy_intermediate_bottom_level_acceleration_structure(brx_pal_intermediate_bottom_level_acceleration_structure *wrapped_intermediate_bottom_level_acceleration_structure) const
{
    assert(NULL != wrapped_intermediate_bottom_level_acceleration_structure);
    brx_pal_vk_intermediate_bottom_level_acceleration_structure *delete_unwrapped_intermediate_bottom_level_acceleration_structure = static_cast<brx_pal_vk_intermediate_bottom_level_acceleration_structure *>(wrapped_intermediate_bottom_level_acceleration_structure);

    delete_unwrapped_intermediate_bottom_level_acceleration_structure->uninit(this->m_device, this->m_pfn_destroy_acceleration_structure, this->m_allocation_callbacks, this->m_memory_allocator);

    delete_unwrapped_intermediate_bottom_level_acceleration_structure->~brx_pal_vk_intermediate_bottom_level_acceleration_structure();
    mcrt_free(delete_unwrapped_intermediate_bottom_level_acceleration_structure);
}

void brx_pal_vk_device::get_non_compacted_bottom_level_acceleration_structure_size(uint32_t bottom_level_acceleration_structure_geometry_count, BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const *wrapped_bottom_level_acceleration_structure_geometries, uint32_t *non_compacted_bottom_level_acceleration_structure_size, uint32_t *build_scratch_size) const
{
    assert(NULL != wrapped_bottom_level_acceleration_structure_geometries);
    assert(NULL != non_compacted_bottom_level_acceleration_structure_size);
    assert(NULL != build_scratch_size);

    PFN_vkGetAccelerationStructureBuildSizesKHR const pfn_get_acceleration_structure_build_sizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetAccelerationStructureBuildSizesKHR"));
    assert(NULL != pfn_get_acceleration_structure_build_sizes);

    mcrt_vector<VkAccelerationStructureGeometryKHR> acceleration_structure_geometries(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));
    mcrt_vector<uint32_t> max_primitive_counts(static_cast<size_t>(bottom_level_acceleration_structure_geometry_count));
    for (uint32_t acceleration_structure_geometry_index = 0U; acceleration_structure_geometry_index < bottom_level_acceleration_structure_geometry_count; ++acceleration_structure_geometry_index)
    {
        BRX_PAL_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_GEOMETRY const &wrapped_bottom_level_acceleration_structure_geometry = wrapped_bottom_level_acceleration_structure_geometries[acceleration_structure_geometry_index];

        VkFormat vertex_position_attribute_format;
        switch (wrapped_bottom_level_acceleration_structure_geometry.vertex_position_attribute_format)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_VERTEX_ATTRIBUTE_FORMAT_R32G32B32_SFLOAT:
            vertex_position_attribute_format = VK_FORMAT_R32G32B32_SFLOAT;
            break;
        default:
            // VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR
            assert(false);
            vertex_position_attribute_format = static_cast<VkFormat>(-1);
            break;
        }

        VkDeviceAddress const vertex_position_buffer_device_memory_range_base = static_cast<brx_pal_vk_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.vertex_position_buffer)->get_device_memory_range_base();

        VkIndexType index_type;
        switch (wrapped_bottom_level_acceleration_structure_geometry.index_type)
        {
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT32:
            index_type = VK_INDEX_TYPE_UINT32;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_UINT16:
            index_type = VK_INDEX_TYPE_UINT16;
            break;
        case BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE:
            index_type = VK_INDEX_TYPE_NONE_KHR;
            break;
        default:
            assert(false);
            index_type = static_cast<VkIndexType>(-1);
        }

        VkDeviceAddress const index_buffer_device_memory_range_base = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? static_cast<brx_pal_vk_acceleration_structure_build_input_read_only_buffer const *>(wrapped_bottom_level_acceleration_structure_geometry.index_buffer)->get_device_memory_range_base() : 0U;

        acceleration_structure_geometries[acceleration_structure_geometry_index] = VkAccelerationStructureGeometryKHR{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            NULL,
            VK_GEOMETRY_TYPE_TRIANGLES_KHR,
            {.triangles =
                 {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                  NULL,
                  vertex_position_attribute_format,
                  {.deviceAddress = vertex_position_buffer_device_memory_range_base},
                  wrapped_bottom_level_acceleration_structure_geometry.vertex_position_binding_stride,
                  (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? (wrapped_bottom_level_acceleration_structure_geometry.index_count - 1U) : (wrapped_bottom_level_acceleration_structure_geometry.vertex_count - 1U),
                  index_type,
                  {.deviceAddress = index_buffer_device_memory_range_base},
                  {.deviceAddress = 0U}}},
            wrapped_bottom_level_acceleration_structure_geometry.force_closest_hit ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0U};

        assert(0U == ((BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? (wrapped_bottom_level_acceleration_structure_geometry.index_count % 3U) : (wrapped_bottom_level_acceleration_structure_geometry.vertex_count % 3U)));
        max_primitive_counts[acceleration_structure_geometry_index] = (BRX_PAL_GRAPHICS_PIPELINE_INDEX_TYPE_NONE != wrapped_bottom_level_acceleration_structure_geometry.index_type) ? (wrapped_bottom_level_acceleration_structure_geometry.index_count / 3U) : (wrapped_bottom_level_acceleration_structure_geometry.vertex_count / 3U);
    }

    VkAccelerationStructureBuildGeometryInfoKHR const acceleration_structure_build_geometry_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        NULL,
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        static_cast<uint32_t>(acceleration_structure_geometries.size()),
        &acceleration_structure_geometries[0],
        NULL,
        {.deviceAddress = 0U}};

    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_size_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        NULL,
        static_cast<VkDeviceSize>(-1),
        static_cast<VkDeviceSize>(-1),
        static_cast<VkDeviceSize>(-1)};

    pfn_get_acceleration_structure_build_sizes(this->m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &acceleration_structure_build_geometry_info, &max_primitive_counts[0], &acceleration_structure_build_size_info);

    (*non_compacted_bottom_level_acceleration_structure_size) = static_cast<uint32_t>(acceleration_structure_build_size_info.accelerationStructureSize);
    (*build_scratch_size) = static_cast<uint32_t>(acceleration_structure_build_size_info.buildScratchSize);
}

brx_pal_non_compacted_bottom_level_acceleration_structure *brx_pal_vk_device::create_non_compacted_bottom_level_acceleration_structure(uint32_t size) const
{
    void *new_unwrapped_non_compacted_bottom_level_acceleration_structure_base = mcrt_malloc(sizeof(brx_pal_vk_non_compacted_bottom_level_acceleration_structure), alignof(brx_pal_vk_non_compacted_bottom_level_acceleration_structure));
    assert(NULL != new_unwrapped_non_compacted_bottom_level_acceleration_structure_base);

    brx_pal_vk_non_compacted_bottom_level_acceleration_structure *new_unwrapped_non_compacted_bottom_level_acceleration_structure = new (new_unwrapped_non_compacted_bottom_level_acceleration_structure_base) brx_pal_vk_non_compacted_bottom_level_acceleration_structure{};
    new_unwrapped_non_compacted_bottom_level_acceleration_structure->init(this->m_device, this->m_pfn_create_acceleration_structure, this->m_allocation_callbacks, this->m_memory_allocator, this->m_non_compacted_bottom_level_acceleration_structure_memory_pool, size);
    return new_unwrapped_non_compacted_bottom_level_acceleration_structure;
}

void brx_pal_vk_device::destroy_non_compacted_bottom_level_acceleration_structure(brx_pal_non_compacted_bottom_level_acceleration_structure *wrapped_non_compacted_bottom_level_acceleration_structure) const
{
    assert(NULL != wrapped_non_compacted_bottom_level_acceleration_structure);
    brx_pal_vk_non_compacted_bottom_level_acceleration_structure *delete_unwrapped_non_compacted_bottom_level_acceleration_structure = static_cast<brx_pal_vk_non_compacted_bottom_level_acceleration_structure *>(wrapped_non_compacted_bottom_level_acceleration_structure);

    delete_unwrapped_non_compacted_bottom_level_acceleration_structure->uninit(this->m_device, this->m_pfn_destroy_acceleration_structure, this->m_allocation_callbacks, this->m_memory_allocator);

    delete_unwrapped_non_compacted_bottom_level_acceleration_structure->~brx_pal_vk_non_compacted_bottom_level_acceleration_structure();
    mcrt_free(delete_unwrapped_non_compacted_bottom_level_acceleration_structure);
}

brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool *brx_pal_vk_device::create_compacted_bottom_level_acceleration_structure_size_query_pool(uint32_t query_count) const
{
    PFN_vkCreateQueryPool const pfn_create_query_pool = reinterpret_cast<PFN_vkCreateQueryPool>(this->m_pfn_get_device_proc_addr(this->m_device, "vkCreateQueryPool"));
    assert(NULL != pfn_create_query_pool);

    void *new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool_base = mcrt_malloc(sizeof(brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool), alignof(brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool));
    assert(NULL != new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool_base);

    brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool *new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool = new (new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool_base) brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool{};
    new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool->init(this->m_device, pfn_create_query_pool, this->m_allocation_callbacks, query_count);
    return new_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool;
}

uint32_t brx_pal_vk_device::get_compacted_bottom_level_acceleration_structure_size_query_pool_result(brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool const *wrapped_compacted_bottom_level_acceleration_structure_size_query_pool, uint32_t query_index) const
{
    assert(NULL != wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);
    brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool const *const unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool = static_cast<brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool const *>(wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);

    VkQueryPool const query_pool = unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool->get_query_pool();

    VkDeviceSize compacted_bottom_level_acceleration_structure_size = static_cast<VkDeviceSize>(-1);
    VkResult res_get_query_pool_results;
    while (VK_NOT_READY == (res_get_query_pool_results = this->m_pfn_get_query_pool_results(this->m_device, query_pool, query_index, 1U, sizeof(VkDeviceSize), &compacted_bottom_level_acceleration_structure_size, sizeof(VkDeviceSize), 0U)))
    {
        _internal_pause();
    }
    assert(VK_SUCCESS == res_get_query_pool_results);

    return static_cast<uint32_t>(compacted_bottom_level_acceleration_structure_size);
}

void brx_pal_vk_device::destroy_compacted_bottom_level_acceleration_structure_size_query_pool(brx_pal_compacted_bottom_level_acceleration_structure_size_query_pool *wrapped_compacted_bottom_level_acceleration_structure_size_query_pool) const
{
    PFN_vkDestroyQueryPool const pfn_destroy_query_pool = reinterpret_cast<PFN_vkDestroyQueryPool>(this->m_pfn_get_device_proc_addr(this->m_device, "vkDestroyQueryPool"));
    assert(NULL != pfn_destroy_query_pool);

    assert(NULL != wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);
    brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool *delete_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool = static_cast<brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool *>(wrapped_compacted_bottom_level_acceleration_structure_size_query_pool);

    delete_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool->uninit(this->m_device, pfn_destroy_query_pool, this->m_allocation_callbacks);

    delete_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool->~brx_pal_vk_compacted_bottom_level_acceleration_structure_size_query_pool();
    mcrt_free(delete_unwrapped_compacted_bottom_level_acceleration_structure_size_query_pool);
}

brx_pal_compacted_bottom_level_acceleration_structure *brx_pal_vk_device::create_compacted_bottom_level_acceleration_structure(uint32_t size) const
{
    void *new_unwrapped_compacted_bottom_level_acceleration_structure_base = mcrt_malloc(sizeof(brx_pal_vk_compacted_bottom_level_acceleration_structure), alignof(brx_pal_vk_compacted_bottom_level_acceleration_structure));
    assert(NULL != new_unwrapped_compacted_bottom_level_acceleration_structure_base);

    brx_pal_vk_compacted_bottom_level_acceleration_structure *new_unwrapped_compacted_bottom_level_acceleration_structure = new (new_unwrapped_compacted_bottom_level_acceleration_structure_base) brx_pal_vk_compacted_bottom_level_acceleration_structure{};
    new_unwrapped_compacted_bottom_level_acceleration_structure->init(this->m_device, this->m_pfn_create_acceleration_structure, this->m_pfn_get_acceleration_structure_device_address, this->m_allocation_callbacks, this->m_memory_allocator, this->m_compacted_bottom_level_acceleration_structure_memory_pool, size);
    return new_unwrapped_compacted_bottom_level_acceleration_structure;
}

void brx_pal_vk_device::destroy_compacted_bottom_level_acceleration_structure(brx_pal_compacted_bottom_level_acceleration_structure *wrapped_compacted_bottom_level_acceleration_structure) const
{
    assert(NULL != wrapped_compacted_bottom_level_acceleration_structure);
    brx_pal_vk_compacted_bottom_level_acceleration_structure *delete_unwrapped_compacted_bottom_level_acceleration_structure = static_cast<brx_pal_vk_compacted_bottom_level_acceleration_structure *>(wrapped_compacted_bottom_level_acceleration_structure);

    delete_unwrapped_compacted_bottom_level_acceleration_structure->uninit(this->m_device, this->m_pfn_destroy_acceleration_structure, this->m_allocation_callbacks, this->m_memory_allocator);

    delete_unwrapped_compacted_bottom_level_acceleration_structure->~brx_pal_vk_compacted_bottom_level_acceleration_structure();
    mcrt_free(delete_unwrapped_compacted_bottom_level_acceleration_structure);
}

brx_pal_top_level_acceleration_structure_instance_upload_buffer *brx_pal_vk_device::create_top_level_acceleration_structure_instance_upload_buffer(uint32_t instance_count) const
{
    void *new_unwrapped_top_level_acceleration_structure_instance_upload_buffer_base = mcrt_malloc(sizeof(brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer), alignof(brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer));
    assert(NULL != new_unwrapped_top_level_acceleration_structure_instance_upload_buffer_base);

    brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer *new_unwrapped_top_level_acceleration_structure_instance_upload_buffer = new (new_unwrapped_top_level_acceleration_structure_instance_upload_buffer_base) brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer{};
    new_unwrapped_top_level_acceleration_structure_instance_upload_buffer->init(this->m_device, this->m_pfn_get_buffer_device_address, this->m_memory_allocator, this->m_top_level_acceleration_structure_instance_upload_buffer_memory_pool, instance_count);
    return new_unwrapped_top_level_acceleration_structure_instance_upload_buffer;
}

void brx_pal_vk_device::destroy_top_level_acceleration_structure_instance_upload_buffer(brx_pal_top_level_acceleration_structure_instance_upload_buffer *wrapped_top_level_acceleration_structure_instance_upload_buffer) const
{
    assert(NULL != wrapped_top_level_acceleration_structure_instance_upload_buffer);
    brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer *delete_unwrapped_top_level_acceleration_structure_instance_upload_buffer = static_cast<brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer *>(wrapped_top_level_acceleration_structure_instance_upload_buffer);

    delete_unwrapped_top_level_acceleration_structure_instance_upload_buffer->uninit(this->m_memory_allocator);

    delete_unwrapped_top_level_acceleration_structure_instance_upload_buffer->~brx_pal_vk_top_level_acceleration_structure_instance_upload_buffer();
    mcrt_free(delete_unwrapped_top_level_acceleration_structure_instance_upload_buffer);
}

void brx_pal_vk_device::get_top_level_acceleration_structure_size(uint32_t top_level_acceleration_structure_instance_count, uint32_t *top_level_acceleration_structure_size, uint32_t *build_scratch_size, uint32_t *update_scratch_size) const
{
    assert(NULL != top_level_acceleration_structure_size);
    assert(NULL != build_scratch_size);
    assert(NULL != update_scratch_size);

    PFN_vkGetAccelerationStructureBuildSizesKHR const pfn_get_acceleration_structure_build_sizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(this->m_pfn_get_device_proc_addr(this->m_device, "vkGetAccelerationStructureBuildSizesKHR"));
    assert(NULL != pfn_get_acceleration_structure_build_sizes);

    VkAccelerationStructureGeometryKHR const acceleration_structure_geometry = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        NULL,
        VK_GEOMETRY_TYPE_INSTANCES_KHR,
        {.instances =
             {
                 VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                 NULL,
                 VK_FALSE,
                 {0U}}}};

    VkAccelerationStructureBuildGeometryInfoKHR const acceleration_structure_build_geometry_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        NULL,
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        1U,
        &acceleration_structure_geometry,
        NULL,
        {.deviceAddress = 0U}};

    VkAccelerationStructureBuildSizesInfoKHR acceleration_structure_build_size_info = {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        NULL,
        static_cast<VkDeviceSize>(-1),
        static_cast<VkDeviceSize>(-1),
        static_cast<VkDeviceSize>(-1)};

    pfn_get_acceleration_structure_build_sizes(this->m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &acceleration_structure_build_geometry_info, &top_level_acceleration_structure_instance_count, &acceleration_structure_build_size_info);

    (*top_level_acceleration_structure_size) = static_cast<uint32_t>(acceleration_structure_build_size_info.accelerationStructureSize);
    (*build_scratch_size) = static_cast<uint32_t>(acceleration_structure_build_size_info.buildScratchSize);
    (*update_scratch_size) = static_cast<uint32_t>(acceleration_structure_build_size_info.updateScratchSize);
}

brx_pal_top_level_acceleration_structure *brx_pal_vk_device::create_top_level_acceleration_structure(uint32_t size) const
{
    void *new_unwrapped_top_level_acceleration_structure_base = mcrt_malloc(sizeof(brx_pal_vk_top_level_acceleration_structure), alignof(brx_pal_vk_top_level_acceleration_structure));
    assert(NULL != new_unwrapped_top_level_acceleration_structure_base);

    brx_pal_vk_top_level_acceleration_structure *new_unwrapped_top_level_acceleration_structure = new (new_unwrapped_top_level_acceleration_structure_base) brx_pal_vk_top_level_acceleration_structure{};
    new_unwrapped_top_level_acceleration_structure->init(this->m_device, this->m_pfn_create_acceleration_structure, this->m_pfn_get_acceleration_structure_device_address, this->m_allocation_callbacks, this->m_memory_allocator, this->m_top_level_acceleration_structure_memory_pool, size);
    return new_unwrapped_top_level_acceleration_structure;
}

void brx_pal_vk_device::destroy_top_level_acceleration_structure(brx_pal_top_level_acceleration_structure *wrapped_top_level_acceleration_structure) const
{
    assert(NULL != wrapped_top_level_acceleration_structure);
    brx_pal_vk_top_level_acceleration_structure *delete_unwrapped_top_level_acceleration_structure = static_cast<brx_pal_vk_top_level_acceleration_structure *>(wrapped_top_level_acceleration_structure);

    delete_unwrapped_top_level_acceleration_structure->uninit(this->m_device, this->m_pfn_destroy_acceleration_structure, this->m_allocation_callbacks, this->m_memory_allocator);

    delete_unwrapped_top_level_acceleration_structure->~brx_pal_vk_top_level_acceleration_structure();
    mcrt_free(delete_unwrapped_top_level_acceleration_structure);
}

#if defined(ENABLE_VULKAN_VALIDATION_LAYER)
#if ENABLE_VULKAN_VALIDATION_LAYER
static VkBool32 VKAPI_PTR _internal_debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
{
#if defined(__GNUC__)
#if defined(__linux__)
#if defined(__ANDROID__)
    if (0 == strcmp("UNASSIGNED-CoreValidation-SwapchainPreTransform", pCallbackData->pMessageIdName))
    {
        // Known Issue
    }
    else
    {
        __android_log_write(ANDROID_LOG_DEBUG, "Vulkan-Demo", pCallbackData->pMessage);
    }
#else
    std::puts(pCallbackData->pMessage);
    std::puts("\n");
#endif
#elif defined(__MACH__)
    std::puts(pCallbackData->pMessage);
    std::puts("\n");
#else
#error Unknown Platform
#endif
#elif defined(_MSC_VER)
    OutputDebugStringA(pCallbackData->pMessage);
    OutputDebugStringA("\n");
#else
#error Unknown Compiler
#endif
    return VK_FALSE;
}
#else
// No Validation Layer
#endif
#else
#error "0 or 1"
#endif

static inline uint32_t _internal_find_lowest_memory_type_index(struct VkPhysicalDeviceMemoryProperties const *physical_device_memory_properties, VkDeviceSize memory_requirements_size, uint32_t memory_requirements_memory_type_bits, VkMemoryPropertyFlags required_property_flags)
{
    uint32_t memory_type_count = physical_device_memory_properties->memoryTypeCount;
    assert(VK_MAX_MEMORY_TYPES >= memory_type_count);

    // The lower memory_type_index indicates the more performance
    for (uint32_t memory_type_index = 0; memory_type_index < memory_type_count; ++memory_type_index)
    {
        uint32_t memory_type_bits = (1U << memory_type_index);
        bool is_required_memory_type = ((memory_requirements_memory_type_bits & memory_type_bits) != 0) ? true : false;

        VkMemoryPropertyFlags property_flags = physical_device_memory_properties->memoryTypes[memory_type_index].propertyFlags;
        bool has_required_property_flags = ((property_flags & required_property_flags) == required_property_flags) ? true : false;

        uint32_t heap_index = physical_device_memory_properties->memoryTypes[memory_type_index].heapIndex;
        VkDeviceSize heap_budget = physical_device_memory_properties->memoryHeaps[heap_index].size;
        // The application is not alone and there may be other applications which interact with the Vulkan as well.
        // The allocation may success even if the budget has been exceeded. However, this may result in performance issue.
        bool is_within_budget = (memory_requirements_size <= heap_budget) ? true : false;

        if (is_required_memory_type && has_required_property_flags && is_within_budget)
        {
            return memory_type_index;
        }
    }

    return VK_MAX_MEMORY_TYPES;
}

static inline uint32_t _internal_find_lowest_memory_type_index(struct VkPhysicalDeviceMemoryProperties const *physical_device_memory_properties, VkDeviceSize memory_requirements_size, uint32_t memory_requirements_memory_type_bits, VkMemoryPropertyFlags required_property_flags, VkMemoryPropertyFlags preferred_property_flags)
{
    VkMemoryPropertyFlags optimal_property_flags = (required_property_flags | preferred_property_flags);
    uint32_t memory_type_index = _internal_find_lowest_memory_type_index(physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, optimal_property_flags);
    if (VK_MAX_MEMORY_TYPES != memory_type_index)
    {
        assert(VK_MAX_MEMORY_TYPES > memory_type_index);
        return memory_type_index;
    }
    else
    {
        return _internal_find_lowest_memory_type_index(physical_device_memory_properties, memory_requirements_size, memory_requirements_memory_type_bits, required_property_flags);
    }
}

#if defined(__GNUC__)
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__) || defined(__arm__)
#include <arm_acle.h>
#else
#error Unknown Architecture
#endif
#elif defined(_MSC_VER)
#if defined(_M_X64) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(_M_ARM64) || defined(_M_ARM)
#include <intrin.h>
#else
#error Unknown Architecture
#endif
#else
#error Unknown Compiler
#endif

static inline void _internal_pause()
{
#if defined(__GNUC__)
#if defined(__x86_64__) || defined(__i386__)
    _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
    __yield();
#else
#error Unknown Architecture
#endif
#elif defined(_MSC_VER)
#if defined(_M_X64) || defined(_M_IX86)
    _mm_pause();
#elif defined(_M_ARM64) || defined(_M_ARM)
    __yield();
#else
#error Unknown Architecture
#endif
#else
#error Unknown Compiler
#endif
}
