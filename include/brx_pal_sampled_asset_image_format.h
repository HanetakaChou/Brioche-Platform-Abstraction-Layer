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

#ifndef _BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_H_
#define _BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_H_ 1

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <algorithm>

enum BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT
{
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_UNDEFINED = 0,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8_UNORM = 1,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM = 2,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_SRGB = 3,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT = 4,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT = 5,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT = 6,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_UNORM_BLOCK = 7,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_SRGB_BLOCK = 8,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_UNORM_BLOCK = 9,
    BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_SRGB_BLOCK = 10
};

struct BRX_PAL_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST
{
    size_t staging_upload_buffer_offset;
    uint32_t output_row_pitch;
    uint32_t output_row_size;
    uint32_t output_row_count;
    uint32_t output_slice_pitch;
    uint32_t output_slice_count;
};

static inline constexpr uint32_t brx_pal_sampled_asset_image_format_get_aspect_count(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT format)
{
    switch (format)
    {
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8_UNORM:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_SRGB:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_UNORM_BLOCK:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_SRGB_BLOCK:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_SRGB_BLOCK:
        return 1U;
    default:
        return -1;
    }
}

static inline constexpr uint32_t brx_pal_sampled_asset_image_format_get_block_size(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT format)
{
    switch (format)
    {
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8_UNORM:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_SRGB:
        return 4U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT:
        return 2U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT:
        return 4U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
        return 8U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_UNORM_BLOCK:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_SRGB_BLOCK:
        return (128U / 8U);
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_SRGB_BLOCK:
        return (128U / 8U);
    default:
        return -1;
    }
}

static inline constexpr uint32_t brx_pal_sampled_asset_image_format_get_block_width(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT format)
{
    switch (format)
    {
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8_UNORM:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_SRGB:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_UNORM_BLOCK:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_SRGB_BLOCK:
        return 4U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_SRGB_BLOCK:
        return 4U;
    default:
        return -1;
    }
}

static inline constexpr uint32_t brx_pal_sampled_asset_image_format_get_block_height(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT format)
{
    switch (format)
    {
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8_UNORM:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_UNORM:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R8G8B8A8_SRGB:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_R16G16B16A16_SFLOAT:
        return 1U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_UNORM_BLOCK:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_BC7_SRGB_BLOCK:
        return 4U;
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT_ASTC_4x4_SRGB_BLOCK:
        return 4U;
    default:
        return -1;
    }
}

static inline constexpr uint32_t brx_pal_sampled_asset_image_import_calculate_subresource_index(uint32_t mip_level, uint32_t array_layer, uint32_t aspect_index, uint32_t mip_levels, uint32_t array_layers)
{
    // [D3D12CalcSubresource](https://github.com/microsoft/DirectX-Headers/blob/48f23952bc08a6dce0727339c07cedbc4797356c/include/directx/d3dx12_core.h#L1166)
    return mip_level + array_layer * mip_levels + aspect_index * mip_levels * array_layers;
}

static inline uint32_t brx_pal_sampled_asset_image_import_calculate_subresource_memcpy_dests(BRX_PAL_SAMPLED_ASSET_IMAGE_FORMAT format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, uint32_t array_layers, size_t staging_upload_buffer_base_offset, uint32_t staging_upload_buffer_offset_alignment, uint32_t staging_upload_buffer_row_pitch_alignment, uint32_t subresource_count, BRX_PAL_SAMPLED_ASSET_IMAGE_IMPORT_SUBRESOURCE_MEMCPY_DEST *subresource_memcpy_dests)
{
    uint32_t (*const align_up_uint32)(uint32_t, uint32_t) = [](uint32_t value, uint32_t alignment) -> uint32_t
    {
        //
        //  Copyright (c) 2005-2019 Intel Corporation
        //
        //  Licensed under the Apache License, Version 2.0 (the "License");
        //  you may not use this file except in compliance with the License.
        //  You may obtain a copy of the License at
        //
        //      http://www.apache.org/licenses/LICENSE-2.0
        //
        //  Unless required by applicable law or agreed to in writing, software
        //  distributed under the License is distributed on an "AS IS" BASIS,
        //  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        //  See the License for the specific language governing permissions and
        //  limitations under the License.
        //

        // [alignUp](https://github.com/oneapi-src/oneTBB/blob/tbb_2019/src/tbbmalloc/shared_utils.h#L42)

        assert(alignment != static_cast<uint32_t>(0U));

        // power-of-2 alignment
        assert((alignment & (alignment - static_cast<uint32_t>(1))) == static_cast<uint32_t>(0U));

        return (((value - static_cast<uint32_t>(1U)) | (alignment - static_cast<uint32_t>(1U))) + static_cast<uint32_t>(1U));
    };

    size_t (*const align_up_uintptr)(size_t, size_t) = [](size_t value, size_t alignment) -> size_t
    {
        //
        //  Copyright (c) 2005-2019 Intel Corporation
        //
        //  Licensed under the Apache License, Version 2.0 (the "License");
        //  you may not use this file except in compliance with the License.
        //  You may obtain a copy of the License at
        //
        //      http://www.apache.org/licenses/LICENSE-2.0
        //
        //  Unless required by applicable law or agreed to in writing, software
        //  distributed under the License is distributed on an "AS IS" BASIS,
        //  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        //  See the License for the specific language governing permissions and
        //  limitations under the License.
        //

        // [alignUp](https://github.com/oneapi-src/oneTBB/blob/tbb_2019/src/tbbmalloc/shared_utils.h#L42)

        assert(alignment != static_cast<size_t>(0U));

        // power-of-2 alignment
        assert((alignment & (alignment - static_cast<size_t>(1))) == static_cast<size_t>(0U));

        return (((value - static_cast<size_t>(1U)) | (alignment - static_cast<size_t>(1U))) + static_cast<size_t>(1U));
    };

    // https://source.winehq.org/git/vkd3d.git/
    // libs/vkd3d/device.c
    // d3d12_device_GetCopyableFootprints
    // libs/vkd3d/utils.c
    // vkd3d_formats

    // https://github.com/ValveSoftware/dxvk/blob/master/src/dxvk/dxvk_context.cpp
    // DxvkContext::uploadImage

    // Context::texSubImage2D libANGLE/Context.cpp
    // Texture::setSubImage libANGLE/Texture.cpp
    // TextureVk::setSubImage libANGLE/renderer/vulkan/TextureVk.cpp
    // TextureVk::setSubImageImpl libANGLE/renderer/vulkan/TextureVk.cpp
    // ImageHelper::stageSubresourceUpdate libANGLE/renderer/vulkan/vk_helpers.cpp
    // ImageHelper::CalculateBufferInfo libANGLE/renderer/vulkan/vk_helpers.cpp
    // ImageHelper::stageSubresourceUpdateImpl libANGLE/renderer/vulkan/vk_helpers.cpp

    // [D3D12_PROPERTY_LAYOUT_FORMAT_TABLE::CalculateResourceSize](https://github.com/microsoft/DirectX-Headers/blob/v1.610.2/src/d3dx12_property_format_table.cpp#L1014)

    uint32_t const aspect_count = brx_pal_sampled_asset_image_format_get_aspect_count(format);
    uint32_t const block_size = brx_pal_sampled_asset_image_format_get_block_size(format);
    uint32_t const block_width = brx_pal_sampled_asset_image_format_get_block_width(format);
    uint32_t const block_height = brx_pal_sampled_asset_image_format_get_block_height(format);

    // vkspec: bufferOffset must be a multiple of 4
    staging_upload_buffer_offset_alignment = align_up_uint32(align_up_uint32(staging_upload_buffer_offset_alignment, 4U), block_size);

    uint32_t total_bytes = 0U;
    size_t staging_upload_buffer_offset = staging_upload_buffer_base_offset;

    // TODO: support more than one aspect
    assert(1U == aspect_count);
    for (uint32_t aspect_index = 0U; aspect_index < aspect_count; ++aspect_index)
    {
        for (uint32_t array_layer = 0U; array_layer < array_layers; ++array_layer)
        {
            uint32_t w = width;
            uint32_t h = height;
            uint32_t d = depth;

            for (uint32_t mipLevel = 0U; mipLevel < mip_levels; ++mipLevel)
            {

                uint32_t const output_row_size = ((w + (block_width - 1U)) / block_width) * block_size;
                uint32_t const output_row_count = (h + (block_height - 1U)) / block_height;
                uint32_t const output_slice_count = d;

                uint32_t const output_row_pitch = align_up_uint32(output_row_size, staging_upload_buffer_row_pitch_alignment);
                uint32_t const output_slice_pitch = output_row_pitch * output_row_count;

                size_t const new_staging_upload_buffer_offset = align_up_uintptr(staging_upload_buffer_offset, static_cast<size_t>(staging_upload_buffer_offset_alignment));
                total_bytes += static_cast<uint32_t>(new_staging_upload_buffer_offset - staging_upload_buffer_offset);
                staging_upload_buffer_offset = new_staging_upload_buffer_offset;

                uint32_t const subresource_index = brx_pal_sampled_asset_image_import_calculate_subresource_index(mipLevel, array_layer, aspect_index, mip_levels, array_layers);
                assert(subresource_index < subresource_count);

                subresource_memcpy_dests[subresource_index].staging_upload_buffer_offset = staging_upload_buffer_offset;
                subresource_memcpy_dests[subresource_index].output_row_pitch = output_row_pitch;
                subresource_memcpy_dests[subresource_index].output_row_size = output_row_size;
                subresource_memcpy_dests[subresource_index].output_slice_pitch = output_slice_pitch;
                subresource_memcpy_dests[subresource_index].output_row_count = output_row_count;
                subresource_memcpy_dests[subresource_index].output_slice_count = output_slice_count;

                uint32_t const surface_size = (output_slice_pitch * output_slice_count);
                total_bytes += surface_size;
                staging_upload_buffer_offset += surface_size;
                assert((staging_upload_buffer_base_offset + total_bytes) == staging_upload_buffer_offset);

                w = std::max(1U, w >> 1U);
                h = std::max(1U, h >> 1U);
                d = std::max(1U, d >> 1U);
            }
        }
    }

    return total_bytes;
}

#endif
