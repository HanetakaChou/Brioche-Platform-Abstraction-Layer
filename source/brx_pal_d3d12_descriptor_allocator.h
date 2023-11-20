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

#ifndef _BRX_PAL_D3D12_DESCRIPTOR_ALLOCATOR_H_
#define _BRX_PAL_D3D12_DESCRIPTOR_ALLOCATOR_H_ 1

#include "../../McRT-Malloc/include/mcrt_map.h"
#include "../../McRT-Malloc/include/mcrt_unordered_map.h"
#include "../../McRT-Malloc/include/mcrt_set.h"
#include <cassert>

class brx_pal_d3d12_descriptor_set;

static constexpr uint32_t const INVALID_BASE_DESCRIPTOR_HEAP_INDEX = 0XFFFFFFFFU;

static constexpr D3D12_GPU_DESCRIPTOR_HANDLE const INVALID_BASE_DESCRIPTOR_HANDLE = D3D12_GPU_DESCRIPTOR_HANDLE{0XFFFFFFFFFFFFFFFFU};

struct brx_pal_d3d12_pipeline_set
{
	mcrt_vector<brx_pal_d3d12_descriptor_set const *> m_descriptor_sets;
};

struct brx_pal_d3d12_pipeline_set_compare
{
	inline bool operator()(brx_pal_d3d12_pipeline_set const &lhs, brx_pal_d3d12_pipeline_set const &rhs) const
	{
		size_t const min_size = sizeof(brx_pal_d3d12_descriptor_set const *) * std::min(lhs.m_descriptor_sets.size(), rhs.m_descriptor_sets.size());
		if (min_size > 0U)
		{
			assert((!lhs.m_descriptor_sets.empty()) && (!rhs.m_descriptor_sets.empty()));

			int const mem_cmp = std::memcmp(lhs.m_descriptor_sets.data(), rhs.m_descriptor_sets.data(), min_size);

			if (mem_cmp < 0)
			{
				return true;
			}
			else if (mem_cmp > 0)
			{
				return false;
			}
			else
			{
				assert(0 == mem_cmp);
				return lhs.m_descriptor_sets.size() < rhs.m_descriptor_sets.size();
			}
		}
		else
		{
			return lhs.m_descriptor_sets.size() < rhs.m_descriptor_sets.size();
		}
	}
};

struct brx_pal_d3d12_pipeline_set_descriptor_heap
{
	mcrt_vector<mcrt_vector<D3D12_GPU_DESCRIPTOR_HANDLE>> m_descriptor_sets_base_descriptor_handle;
	ID3D12DescriptorHeap *m_cbv_srv_uav_descriptor_heap;
	ID3D12DescriptorHeap *m_sampler_descriptor_heap;
};

class brx_pal_d3d12_descriptor_allocator
{
	ID3D12Device *m_device;
	UINT m_cbv_srv_uav_descriptor_heap_descriptor_increment_size;
	UINT m_sampler_descriptor_heap_descriptor_increment_size;
	mcrt_map<brx_pal_d3d12_pipeline_set, brx_pal_d3d12_pipeline_set_descriptor_heap, brx_pal_d3d12_pipeline_set_compare> m_pipeline_sets_descriptor_heap;

public:
	brx_pal_d3d12_descriptor_allocator();
	void init(ID3D12Device *new_device);
	void uninit();
	~brx_pal_d3d12_descriptor_allocator();

	brx_pal_d3d12_pipeline_set_descriptor_heap const *create_or_get_pipeline_set_descriptor_heap(uint32_t descriptor_set_count, brx_pal_descriptor_set const *const *descriptor_sets);
	void invalidate_descriptor_set(brx_pal_descriptor_set const *descriptor_set);
};

#endif