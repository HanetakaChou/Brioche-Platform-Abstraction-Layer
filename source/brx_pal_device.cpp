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

#include "../include/brx_pal_device.h"

#if defined(__GNUC__)

#if defined(__linux__)

extern brx_pal_device *brx_pal_create_vk_device(void *wsi_connection, bool support_ray_tracing);

extern void brx_pal_destroy_vk_device(brx_pal_device *wrapped_device);

extern "C" brx_pal_device *brx_pal_create_device(void *wsi_connection, bool support_ray_tracing)
{
    return brx_pal_create_vk_device(wsi_connection, support_ray_tracing);
}

extern "C" void brx_pal_destroy_device(brx_pal_device *wrapped_device)
{
    return brx_pal_destroy_vk_device(wrapped_device);
}

#else
#error Unknown Platform
#endif

#elif defined(_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <sdkddkver.h>
#include <windows.h>

extern brx_pal_device *brx_pal_create_d3d12_device(void *wsi_connection, bool support_ray_tracing);

extern void brx_pal_destroy_d3d12_device(brx_pal_device *device);

extern brx_pal_device *brx_pal_create_vk_device(void *wsi_connection, bool support_ray_tracing);

extern void brx_pal_destroy_vk_device(brx_pal_device *wrapped_device);

extern "C" brx_pal_device *brx_pal_create_device(void *wsi_connection, bool support_ray_tracing)
{
    // we can always assume Direct3D is supported better than vulkan on windows
    if ((NULL != LoadLibraryW(L"D3D12.dll")) && (NULL != LoadLibraryW(L"DXGI.dll")))
    {
        return brx_pal_create_d3d12_device(wsi_connection, support_ray_tracing);
    }
    else if (NULL != LoadLibraryW(L"vulkan-1.dll"))
    {
        return brx_pal_create_vk_device(wsi_connection, support_ray_tracing);
    }
    else
    {
        assert(ERROR_MOD_NOT_FOUND == GetLastError());
        return NULL;
    }
}

extern "C" void brx_pal_destroy_device(brx_pal_device *wrapped_device)
{
    switch (wrapped_device->get_backend_name())
    {
    case BRX_PAL_BACKEND_NAME_D3D12:
    {
        return brx_pal_destroy_d3d12_device(wrapped_device);
    }
    break;
    case BRX_PAL_BACKEND_NAME_VK:
    {
        return brx_pal_destroy_vk_device(wrapped_device);
    }
    break;
    default:
    {
        assert(false);
    }
    }
}

#else
#error Unknown Compiler
#endif
