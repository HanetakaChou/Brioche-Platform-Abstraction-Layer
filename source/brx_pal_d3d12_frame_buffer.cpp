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
#include <assert.h>

brx_pal_d3d12_frame_buffer::brx_pal_d3d12_frame_buffer() : m_depth_stencil_attachment_image(NULL)
{
}

void brx_pal_d3d12_frame_buffer::init(brx_pal_render_pass const *wrapped_render_pass, uint32_t width, uint32_t height, uint32_t color_attachment_count, brx_pal_color_attachment_image const *const *color_attachments, brx_pal_depth_stencil_attachment_image const *depth_stencil_attachment)
{
	assert(NULL != wrapped_render_pass);
	brx_pal_d3d12_render_pass const *const unwrapped_render_pass = static_cast<brx_pal_d3d12_render_pass const *>(wrapped_render_pass);
	assert(unwrapped_render_pass->get_color_attachment_count() == color_attachment_count);
	assert((BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED == unwrapped_render_pass->get_depth_stencil_attachment_format()) == (NULL == depth_stencil_attachment));

	assert(this->m_color_attachment_images.empty());
	this->m_color_attachment_images.resize(color_attachment_count);
	for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
	{
		assert(NULL != color_attachments[color_attachment_index]);
		this->m_color_attachment_images[color_attachment_index] = static_cast<brx_pal_d3d12_color_attachment_image const *>(color_attachments[color_attachment_index]);
	}

	assert(NULL == this->m_depth_stencil_attachment_image);
	if (NULL != depth_stencil_attachment)
	{
		this->m_depth_stencil_attachment_image = static_cast<brx_pal_d3d12_depth_stencil_attachment_image const *>(depth_stencil_attachment);
	}
}

void brx_pal_d3d12_frame_buffer::uninit()
{
}

uint32_t brx_pal_d3d12_frame_buffer::get_color_attachment_image_count() const
{
	return this->m_color_attachment_images.size();
}

brx_pal_d3d12_color_attachment_image const *const *brx_pal_d3d12_frame_buffer::get_color_attachment_images() const
{
	return this->m_color_attachment_images.data();
}

brx_pal_d3d12_depth_stencil_attachment_image const *brx_pal_d3d12_frame_buffer::get_depth_stencil_attachment_image() const
{
	return this->m_depth_stencil_attachment_image;
}
