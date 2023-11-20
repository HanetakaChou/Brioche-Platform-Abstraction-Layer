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
#include <algorithm>
#include <assert.h>

static constexpr uint32_t const g_preferred_swap_chain_image_count = 2U;
static constexpr uint32_t const g_preferred_swap_chain_image_width = 1280U;
static constexpr uint32_t const g_preferred_swap_chain_image_height = 720U;

brx_pal_vk_surface::brx_pal_vk_surface() : m_surface(VK_NULL_HANDLE)
{
}

void brx_pal_vk_surface::init(
    VkInstance instance,
    VkPhysicalDevice physical_device,
    uint32_t graphics_queue_family_index,
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    PFN_vkCreateAndroidSurfaceKHR pfn_create_android_surface,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    PFN_vkCreateXcbSurfaceKHR pfn_create_xcb_surface,
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    PFN_vkCreateWin32SurfaceKHR pfn_create_win32_surface,
#else
#error Unknown Platform
#endif
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfn_get_physical_device_surface_support,
    VkAllocationCallbacks const *allocation_callbacks,
    void *wsi_window)
{
    assert(VK_NULL_HANDLE == this->m_surface);
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    {
        ANativeWindow *native_window = static_cast<ANativeWindow *>(wsi_window);

        VkAndroidSurfaceCreateInfoKHR const android_surface_create_info = {
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            NULL,
            0U,
            native_window};
        VkResult res_create_android_surface = pfn_create_android_surface(instance, &android_surface_create_info, allocation_callbacks, &this->m_surface);
        assert(VK_SUCCESS == res_create_android_surface);
    }
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    {
        struct brx_pal_xcb_window_T
        {
            xcb_connection_t *m_connection;
            xcb_window_t m_window;
        };
        brx_pal_xcb_window_T *const brx_pal_xcb_window = static_cast<brx_pal_xcb_window_T *>(wsi_window);

        VkXcbSurfaceCreateInfoKHR const xcb_surface_create_info = {
            VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
            NULL,
            0U,
            brx_pal_xcb_window->m_connection,
            brx_pal_xcb_window->m_window};
        VkResult res_create_xcb_surface = pfn_create_xcb_surface(instance, &xcb_surface_create_info, allocation_callbacks, &this->m_surface);
        assert(VK_SUCCESS == res_create_xcb_surface);
    }
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    {
        HWND hWnd = static_cast<HWND>(wsi_window);

        VkWin32SurfaceCreateInfoKHR const win32_surface_create_info = {
            VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            NULL,
            0U,
            reinterpret_cast<HINSTANCE>(GetClassLongPtrW(hWnd, GCLP_HMODULE)),
            hWnd};
        VkResult res_create_win32_surface = pfn_create_win32_surface(instance, &win32_surface_create_info, allocation_callbacks, &this->m_surface);
        assert(VK_SUCCESS == res_create_win32_surface);
    }
#else
#error Unknown Platform
#endif
    {
        VkBool32 supported;
        VkResult res_get_physical_device_surface_support = pfn_get_physical_device_surface_support(physical_device, graphics_queue_family_index, this->m_surface, &supported);
        assert(VK_SUCCESS == res_get_physical_device_surface_support);
        assert(VK_FALSE != supported);
    }
}

void brx_pal_vk_surface::uninit(VkInstance instance, PFN_vkDestroySurfaceKHR pfn_destroy_surface, VkAllocationCallbacks const *allocation_callbacks)
{
    pfn_destroy_surface(instance, this->m_surface, allocation_callbacks);

    this->m_surface = VK_NULL_HANDLE;
}

brx_pal_vk_surface::~brx_pal_vk_surface()
{
    assert(VK_NULL_HANDLE == this->m_surface);
}

VkSurfaceKHR brx_pal_vk_surface::get_surface() const
{
    return this->m_surface;
}

brx_pal_vk_swap_chain_image_view::brx_pal_vk_swap_chain_image_view() : m_image_view(VK_NULL_HANDLE)
{
}

void brx_pal_vk_swap_chain_image_view::init(VkDevice device, PFN_vkCreateImageView pfn_create_image_view, VkAllocationCallbacks const *allocation_callbacks, VkImage image, VkFormat image_format)
{
    assert(VK_NULL_HANDLE == this->m_image_view);

    VkImageViewCreateInfo const image_view_create_info = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        NULL,
        0U,
        image,
        VK_IMAGE_VIEW_TYPE_2D,
        image_format,
        {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
        {VK_IMAGE_ASPECT_COLOR_BIT, 0U, 1U, 0U, 1U}};

    VkResult res_create_image_view = pfn_create_image_view(device, &image_view_create_info, allocation_callbacks, &this->m_image_view);
    assert(VK_SUCCESS == res_create_image_view);
}

void brx_pal_vk_swap_chain_image_view::uninit(VkDevice device, PFN_vkDestroyImageView pfn_destroy_image_view, VkAllocationCallbacks const *allocation_callbacks)
{
    pfn_destroy_image_view(device, this->m_image_view, allocation_callbacks);

    this->m_image_view = VK_NULL_HANDLE;
}

brx_pal_vk_swap_chain_image_view::~brx_pal_vk_swap_chain_image_view()
{
    assert(VK_NULL_HANDLE == this->m_image_view);
}

VkImageView brx_pal_vk_swap_chain_image_view::get_attachment_image_view() const
{
    assert(VK_NULL_HANDLE != this->m_image_view);
    return this->m_image_view;
}

brx_pal_sampled_image const *brx_pal_vk_swap_chain_image_view::get_sampled_image() const
{
    return NULL;
}

brx_pal_vk_swap_chain::brx_pal_vk_swap_chain() : m_swap_chain(VK_NULL_HANDLE), m_image_format(VK_FORMAT_UNDEFINED), m_image_width(0), m_image_height(0)
{
}

void brx_pal_vk_swap_chain::init(VkDevice device, VkPhysicalDevice physical_device, PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pfn_get_physical_device_surface_formats, PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfn_get_physical_device_surface_capabilities, PFN_vkGetPhysicalDeviceSurfacePresentModesKHR pfn_get_physical_device_surface_present_modes, PFN_vkCreateSwapchainKHR pfn_create_swap_chain, PFN_vkGetSwapchainImagesKHR pfn_get_swap_chain_images, PFN_vkCreateImageView pfn_create_image_view, VkAllocationCallbacks const *allocation_callbacks, VkSurfaceKHR surface)
{
    assert(VK_FORMAT_UNDEFINED == this->m_image_format);
    assert(0 == this->m_image_width);
    assert(0 == this->m_image_height);
    // Get Information from Surface
    VkCompositeAlphaFlagBitsKHR swap_chain_composite_alpha = static_cast<VkCompositeAlphaFlagBitsKHR>(-1);
    uint32_t request_swap_chain_image_count = static_cast<uint32_t>(-1);
    {
        // Format
        {
            uint32_t surface_format_count_1 = uint32_t(-1);
            pfn_get_physical_device_surface_formats(physical_device, surface, &surface_format_count_1, NULL);

            mcrt_vector<VkSurfaceFormatKHR> surface_formats(static_cast<size_t>(surface_format_count_1));

            uint32_t surface_format_count_2 = surface_format_count_1;
            pfn_get_physical_device_surface_formats(physical_device, surface, &surface_format_count_2, surface_formats.data());
            assert(surface_format_count_1 == surface_format_count_2);

            assert(surface_format_count_2 >= 1U);
            if (VK_FORMAT_UNDEFINED != surface_formats[0].format)
            {
                this->m_image_format = surface_formats[0].format;
            }
            else
            {
                this->m_image_format = VK_FORMAT_B8G8R8A8_UNORM;
            }

            assert(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == surface_formats[0].colorSpace);
        }

        // Capabilities
        {
            VkSurfaceCapabilitiesKHR surface_capabilities;
            VkResult res_get_physical_device_surface_capablilities = pfn_get_physical_device_surface_capabilities(physical_device, surface, &surface_capabilities);
            assert(VK_SUCCESS == res_get_physical_device_surface_capablilities);

            request_swap_chain_image_count = std::min(std::max(surface_capabilities.minImageCount, g_preferred_swap_chain_image_count), surface_capabilities.maxImageCount);

            constexpr uint32_t const min_max_image_dimension_2d = 8192U;

            this->m_image_width = std::min(std::max(std::max(surface_capabilities.minImageExtent.width, 1U), (surface_capabilities.currentExtent.width != 0XFFFFFFFFU) ? surface_capabilities.currentExtent.width : g_preferred_swap_chain_image_width), std::min(std::max(surface_capabilities.maxImageExtent.width, 1U), min_max_image_dimension_2d));

            this->m_image_height = std::min(std::max(std::max(surface_capabilities.minImageExtent.height, 1U), (surface_capabilities.currentExtent.height != 0XFFFFFFFFU) ? surface_capabilities.currentExtent.height : g_preferred_swap_chain_image_height), std::min(std::max(surface_capabilities.maxImageExtent.height, 1U), min_max_image_dimension_2d));

            // Test on Android
            // We should use identity even if the surface is rotation 90
            assert(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR == (VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR & surface_capabilities.supportedTransforms));

            VkCompositeAlphaFlagBitsKHR composite_alphas[] = {
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR};

            for (VkCompositeAlphaFlagBitsKHR composite_alpha : composite_alphas)
            {
                if (0 != (composite_alpha & surface_capabilities.supportedCompositeAlpha))
                {
                    swap_chain_composite_alpha = composite_alpha;
                    break;
                }
            }
        }
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    {
        uint32_t present_mode_count = static_cast<uint32_t>(-1);
        pfn_get_physical_device_surface_present_modes(physical_device, surface, &present_mode_count, NULL);

        mcrt_vector<VkPresentModeKHR> present_modes(static_cast<size_t>(present_mode_count));

        pfn_get_physical_device_surface_present_modes(physical_device, surface, &present_mode_count, &present_modes[0]);
        assert(present_mode_count == present_modes.size());

        for (uint32_t present_mode_index = 0U; present_mode_index < present_mode_count; ++present_mode_index)
        {
            if (VK_PRESENT_MODE_MAILBOX_KHR == present_modes[present_mode_index])
            {
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    // Create Swap Chain
    assert(VK_NULL_HANDLE == this->m_swap_chain);
    {
        VkSwapchainCreateInfoKHR const swap_chain_create_info = {
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            NULL,
            0U,
            surface,
            request_swap_chain_image_count,
            this->m_image_format,
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            {this->m_image_width, this->m_image_height},
            1U,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0U,
            NULL,
            // Test on Android
            // We should use identity even if the surface is rotation 90
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            swap_chain_composite_alpha,
            present_mode,
            VK_FALSE,
            VK_NULL_HANDLE};

        VkResult res_create_swap_chain = pfn_create_swap_chain(device, &swap_chain_create_info, allocation_callbacks, &this->m_swap_chain);
        assert(VK_SUCCESS == res_create_swap_chain);
    }

    uint32_t new_swap_chain_image_count = static_cast<uint32_t>(-1);
    mcrt_vector<VkImage> new_swap_chain_images;
    {

        uint32_t swap_chain_image_count_1 = uint32_t(-1);
        VkResult res_get_swap_chain_images_1 = pfn_get_swap_chain_images(device, this->m_swap_chain, &swap_chain_image_count_1, NULL);
        assert(VK_SUCCESS == res_get_swap_chain_images_1);

        new_swap_chain_image_count = swap_chain_image_count_1;

        new_swap_chain_images.resize(new_swap_chain_image_count);

        uint32_t swap_chain_image_count_2 = swap_chain_image_count_1;
        VkResult res_get_swap_chain_images_2 = pfn_get_swap_chain_images(device, this->m_swap_chain, &swap_chain_image_count_2, new_swap_chain_images.data());
        assert(VK_SUCCESS == res_get_swap_chain_images_2 && swap_chain_image_count_2 == swap_chain_image_count_1);

        assert(swap_chain_image_count_2 == new_swap_chain_image_count);
    }

    // Create Swap Chain Image View
    assert(this->m_image_views.empty());
    {
        this->m_image_views.resize(new_swap_chain_image_count);

        for (uint32_t swap_chain_image_index = 0U; swap_chain_image_index < new_swap_chain_image_count; ++swap_chain_image_index)
        {
            this->m_image_views[swap_chain_image_index].init(device, pfn_create_image_view, allocation_callbacks, new_swap_chain_images[swap_chain_image_index], this->m_image_format);
        }
    }
}

void brx_pal_vk_swap_chain::uninit(VkDevice device, PFN_vkDestroySwapchainKHR pfn_destroy_swapchain, PFN_vkDestroyImageView pfn_destroy_image_view, VkAllocationCallbacks const *allocation_callbacks)
{
    for (size_t swap_chain_image_index = 0U; swap_chain_image_index < this->m_image_views.size(); ++swap_chain_image_index)
    {
        this->m_image_views[swap_chain_image_index].uninit(device, pfn_destroy_image_view, allocation_callbacks);
    }
    this->m_image_views.clear();

    pfn_destroy_swapchain(device, this->m_swap_chain, allocation_callbacks);
    this->m_swap_chain = VK_NULL_HANDLE;
}

brx_pal_vk_swap_chain::~brx_pal_vk_swap_chain()
{
    assert(this->m_image_views.empty());
    assert(VK_NULL_HANDLE == this->m_swap_chain);
}

VkSwapchainKHR brx_pal_vk_swap_chain::get_swap_chain() const
{
    return this->m_swap_chain;
}

BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT brx_pal_vk_swap_chain::get_image_format() const
{
    BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT brx_pal_color_attachment_image_format;
    switch (this->m_image_format)
    {
    case VK_FORMAT_B8G8R8A8_UNORM:
        brx_pal_color_attachment_image_format = BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_UNORM;
        break;
    case VK_FORMAT_B8G8R8A8_SRGB:
        brx_pal_color_attachment_image_format = BRX_PAL_COLOR_ATTACHMENT_FORMAT_B8G8R8A8_SRGB;
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
        brx_pal_color_attachment_image_format = BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_UNORM;
        break;
    case VK_FORMAT_R8G8B8A8_SRGB:
        brx_pal_color_attachment_image_format = BRX_PAL_COLOR_ATTACHMENT_FORMAT_R8G8B8A8_SRGB;
        break;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        brx_pal_color_attachment_image_format = BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2B10G10R10_UNORM_PACK32;
        break;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        brx_pal_color_attachment_image_format = BRX_PAL_COLOR_ATTACHMENT_FORMAT_A2R10G10B10_UNORM_PACK32;
        break;
    default:
        assert(false);
        brx_pal_color_attachment_image_format = static_cast<BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT>(-1);
    }
    return brx_pal_color_attachment_image_format;
}

uint32_t brx_pal_vk_swap_chain::get_image_width() const
{
    return this->m_image_width;
}

uint32_t brx_pal_vk_swap_chain::get_image_height() const
{
    return this->m_image_height;
}

uint32_t brx_pal_vk_swap_chain::get_image_count() const
{
    return static_cast<uint32_t>(this->m_image_views.size());
}

brx_pal_color_attachment_image const *brx_pal_vk_swap_chain::get_image(uint32_t swap_chain_image_index) const
{
    assert(swap_chain_image_index < this->m_image_views.size());
    return &this->m_image_views[swap_chain_image_index];
}
