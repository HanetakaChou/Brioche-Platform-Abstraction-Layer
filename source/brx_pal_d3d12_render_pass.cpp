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

brx_pal_d3d12_render_pass::brx_pal_d3d12_render_pass() : m_depth_stencil_attachment_format(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED)
{
}

void brx_pal_d3d12_render_pass::init(uint32_t color_attachment_count, BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT const *color_attachments, BRX_PAL_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT const *depth_stencil_attachment)
{
	assert(this->m_color_attachment_formats.empty());
	this->m_color_attachment_formats.resize(color_attachment_count);

	assert(this->m_color_attachment_load_clear_indices.empty());
	assert(this->m_color_attachment_store_flush_for_present_indices.empty());
	for (uint32_t color_attachment_index = 0U; color_attachment_index < color_attachment_count; ++color_attachment_index)
	{
		this->m_color_attachment_formats[color_attachment_index] = color_attachments[color_attachment_index].format;

		if (BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_CLEAR == color_attachments[color_attachment_index].load_operation)
		{
			this->m_color_attachment_load_clear_indices.push_back(color_attachment_index);
		}
		else
		{
			assert((BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_LOAD_OPERATION_DONT_CARE == color_attachments[color_attachment_index].load_operation));
		}

		if (BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_PRESENT == color_attachments[color_attachment_index].store_operation)
		{
			this->m_color_attachment_store_flush_for_present_indices.emplace(color_attachment_index);
		}
		else
		{
			assert((BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_DONT_CARE == color_attachments[color_attachment_index].store_operation) || (BRX_PAL_RENDER_PASS_COLOR_ATTACHMENT_STORE_OPERATION_FLUSH_FOR_SAMPLED_IMAGE == color_attachments[color_attachment_index].store_operation));
		}
	}
	assert(this->m_color_attachment_formats.size() == color_attachment_count);
	assert(this->m_color_attachment_load_clear_indices.size() <= color_attachment_count);
	assert(this->m_color_attachment_store_flush_for_present_indices.size() <= color_attachment_count);

	assert(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED == this->m_depth_stencil_attachment_format);
	if (NULL != depth_stencil_attachment)
	{
		this->m_depth_stencil_attachment_format = depth_stencil_attachment->format;
	}
	else
	{
		assert(BRX_PAL_DEPTH_STENCIL_ATTACHMENT_FORMAT_UNDEFINED == this->m_depth_stencil_attachment_format);
	}
}

void brx_pal_d3d12_render_pass::uninit()
{
}

uint32_t brx_pal_d3d12_render_pass::get_color_attachment_count() const
{
	return this->m_color_attachment_formats.size();
}

BRX_PAL_COLOR_ATTACHMENT_IMAGE_FORMAT const *brx_pal_d3d12_render_pass::get_color_attachment_formats() const
{
	return this->m_color_attachment_formats.data();
}

uint32_t brx_pal_d3d12_render_pass::get_color_attachment_load_clear_count() const
{
	return this->m_color_attachment_load_clear_indices.size();
}

uint32_t const *brx_pal_d3d12_render_pass::get_color_attachment_load_clear_indices() const
{
	return this->m_color_attachment_load_clear_indices.data();
}

bool brx_pal_d3d12_render_pass::is_color_attachment_store_flush_for_present(uint32_t color_attachment_index) const
{
	return (this->m_color_attachment_store_flush_for_present_indices.end() != this->m_color_attachment_store_flush_for_present_indices.find(color_attachment_index));
}

BRX_PAL_DEPTH_STENCIL_ATTACHMENT_IMAGE_FORMAT brx_pal_d3d12_render_pass::get_depth_stencil_attachment_format() const
{
	return this->m_depth_stencil_attachment_format;
}
