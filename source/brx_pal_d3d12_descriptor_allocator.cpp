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
#include <algorithm>
#include <assert.h>

brx_pal_d3d12_descriptor_allocator::brx_pal_d3d12_descriptor_allocator()
	: m_device(NULL),
	  m_shader_visible_cbv_srv_uav_descriptor_heap(NULL),
	  m_non_shader_visible_cbv_srv_uav_descriptor_heap(NULL),
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	  m_statistic_get_shader_visible_cbv_srv_uav_cpu_descriptor_handle(0U),
	  m_statistic_get_non_shader_visible_cbv_srv_uav_cpu_descriptor_handle(0U),
#endif
	  m_cbv_srv_uav_descriptor_heap_num_alloced(0U),
	  m_cbv_srv_uav_descriptor_heap_next_free_index(0U),
	  m_shader_visible_sampler_descriptor_heap(NULL),
	  m_non_shader_visible_sampler_descriptor_heap(NULL),
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	  m_statistic_get_shader_visible_sampler_cpu_descriptor_handle(0U),
	  m_statistic_get_non_shader_visible_sampler_cpu_descriptor_handle(0U),
#endif
	  m_sampler_descriptor_heap_num_alloced(0U),
	  m_sampler_descriptor_heap_next_free_index(0U)

{
}

void brx_pal_d3d12_descriptor_allocator::init(ID3D12Device *device)
{
	assert(NULL == this->m_device);
	this->m_device = device;

	this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	assert(NULL == this->m_shader_visible_cbv_srv_uav_descriptor_heap);
	assert(NULL == this->m_non_shader_visible_cbv_srv_uav_descriptor_heap);
	assert(0U == this->m_cbv_srv_uav_descriptor_heap_free_list.size());
	assert(0U == this->m_cbv_srv_uav_descriptor_heap_num_alloced);
	assert(0U == this->m_cbv_srv_uav_descriptor_heap_next_free_index);

	this->m_sampler_descriptor_heap_descriptor_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	assert(NULL == this->m_shader_visible_sampler_descriptor_heap);
	assert(NULL == this->m_non_shader_visible_sampler_descriptor_heap);
	assert(0U == this->m_sampler_descriptor_heap_free_list.size());
	assert(0U == this->m_sampler_descriptor_heap_num_alloced);
	assert(0U == this->m_sampler_descriptor_heap_next_free_index);
}

void brx_pal_d3d12_descriptor_allocator::uninit()
{
	if (0U < this->m_cbv_srv_uav_descriptor_heap_num_alloced)
	{
		assert(1U == this->m_cbv_srv_uav_descriptor_heap_free_list.size());
		assert(0U == m_cbv_srv_uav_descriptor_heap_free_list.begin()->first);
		assert(this->m_cbv_srv_uav_descriptor_heap_next_free_index == m_cbv_srv_uav_descriptor_heap_free_list.begin()->second);

#if defined(ENABLE_BUSY_LIST_VALIDATION) && ENABLE_BUSY_LIST_VALIDATION
		assert(this->m_cbv_srv_uav_descriptor_heap_busy_list.empty());
#endif

		assert(NULL != this->m_shader_visible_cbv_srv_uav_descriptor_heap);
		assert(NULL != this->m_non_shader_visible_cbv_srv_uav_descriptor_heap);

		this->m_shader_visible_cbv_srv_uav_descriptor_heap->Release();
		this->m_non_shader_visible_cbv_srv_uav_descriptor_heap->Release();

		this->m_shader_visible_cbv_srv_uav_descriptor_heap = NULL;
		this->m_non_shader_visible_cbv_srv_uav_descriptor_heap = NULL;
	}
	else
	{
		assert(NULL == this->m_shader_visible_cbv_srv_uav_descriptor_heap);
		assert(NULL == this->m_non_shader_visible_cbv_srv_uav_descriptor_heap);
	}

	if (0U < this->m_sampler_descriptor_heap_num_alloced)
	{
		assert(1U == this->m_sampler_descriptor_heap_free_list.size());
		assert(0U == m_sampler_descriptor_heap_free_list.begin()->first);
		assert(this->m_sampler_descriptor_heap_next_free_index == m_sampler_descriptor_heap_free_list.begin()->second);

#if defined(ENABLE_BUSY_LIST_VALIDATION) && ENABLE_BUSY_LIST_VALIDATION
		assert(this->m_sampler_descriptor_heap_busy_list.empty());
#endif

		assert(NULL != this->m_shader_visible_sampler_descriptor_heap);
		assert(NULL != this->m_non_shader_visible_sampler_descriptor_heap);

		this->m_shader_visible_sampler_descriptor_heap->Release();
		this->m_non_shader_visible_sampler_descriptor_heap->Release();

		this->m_shader_visible_sampler_descriptor_heap = NULL;
		this->m_non_shader_visible_sampler_descriptor_heap = NULL;
	}
	else
	{
		assert(NULL == this->m_shader_visible_sampler_descriptor_heap);
		assert(NULL == this->m_non_shader_visible_sampler_descriptor_heap);
	}

	this->m_device = NULL;
}

brx_pal_d3d12_descriptor_allocator::~brx_pal_d3d12_descriptor_allocator()
{
	assert(NULL == this->m_device);
	assert(NULL == this->m_shader_visible_cbv_srv_uav_descriptor_heap);
	assert(NULL == this->m_non_shader_visible_cbv_srv_uav_descriptor_heap);
	assert(NULL == this->m_shader_visible_sampler_descriptor_heap);
	assert(NULL == this->m_non_shader_visible_sampler_descriptor_heap);
}

uint32_t brx_pal_d3d12_descriptor_allocator::alloc_cbv_srv_uav_descriptor(uint32_t num_descriptors)
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	assert(this->m_statistic_get_shader_visible_cbv_srv_uav_cpu_descriptor_handle == this->m_statistic_get_non_shader_visible_cbv_srv_uav_cpu_descriptor_handle);
#endif

	assert(this->m_cbv_srv_uav_descriptor_heap_next_free_index <= this->m_cbv_srv_uav_descriptor_heap_num_alloced);

	for (mcrt_map<uint32_t, uint32_t>::iterator current = this->m_cbv_srv_uav_descriptor_heap_free_list.begin(); current != this->m_cbv_srv_uav_descriptor_heap_free_list.end(); current = std::next(current))
	{
		uint32_t const base_descriptor_heap_index = current->first;
		uint32_t const num_alloced_descriptor = current->second;

		if (num_alloced_descriptor >= num_descriptors)
		{
			this->m_cbv_srv_uav_descriptor_heap_free_list.erase(current);

			if (num_alloced_descriptor > num_descriptors)
			{
				this->m_cbv_srv_uav_descriptor_heap_free_list.emplace(base_descriptor_heap_index + num_descriptors, num_alloced_descriptor - num_descriptors);
			}

#if defined(ENABLE_BUSY_LIST_VALIDATION) && ENABLE_BUSY_LIST_VALIDATION
			this->m_cbv_srv_uav_descriptor_heap_busy_list.emplace(base_descriptor_heap_index, num_descriptors);
#endif
			return base_descriptor_heap_index;
		}
	}

	if ((this->m_cbv_srv_uav_descriptor_heap_next_free_index + num_descriptors) > this->m_cbv_srv_uav_descriptor_heap_num_alloced)
	{
		uint32_t aligned_minimum_num_alloced;
		{
			uint32_t const minimum_num_alloced = (this->m_cbv_srv_uav_descriptor_heap_num_alloced + num_descriptors);
			if (minimum_num_alloced > 1U)
			{
#if defined(__GNUC__)
				unsigned int index = (31U - static_cast<unsigned int>(__builtin_clz(static_cast<unsigned int>(minimum_num_alloced - 1U))));
				aligned_minimum_num_alloced = (static_cast<uint32_t>(1U) << (static_cast<uint32_t>(index) + static_cast<uint32_t>(1U)));
#elif defined(_MSC_VER)
#if defined(_M_X64) || defined(_M_IX86)
				unsigned long index;
				_BitScanReverse(&index, static_cast<unsigned long>(minimum_num_alloced - 1U));
				aligned_minimum_num_alloced = (static_cast<uint32_t>(1U) << (static_cast<uint32_t>(index) + static_cast<uint32_t>(1U)));
#elif defined(_M_ARM64) || defined(_M_ARM)
				unsigned int index = _CountLeadingZeros(static_cast<unsigned long>(minimum_num_alloced - 1U));
				aligned_minimum_num_alloced = (static_cast<uint32_t>(1U) << (static_cast<uint32_t>(index) + static_cast<uint32_t>(1U)));
#else
#error Unknown Architecture
#endif

#else
#error Unknown Compiler
#endif
			}
			else
			{
				aligned_minimum_num_alloced = 1U;
			}
		}

		uint32_t const new_num_alloced = std::max(2U * this->m_cbv_srv_uav_descriptor_heap_num_alloced, aligned_minimum_num_alloced);

		ID3D12DescriptorHeap *new_shader_visible_cbv_srv_uav_descriptor_heap = NULL;
		{
			D3D12_DESCRIPTOR_HEAP_DESC const shader_visible_descriptor_heap_desc = {
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				new_num_alloced,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				0U};
			HRESULT hr_create_descriptor_heap = this->m_device->CreateDescriptorHeap(&shader_visible_descriptor_heap_desc, IID_PPV_ARGS(&new_shader_visible_cbv_srv_uav_descriptor_heap));
			assert(SUCCEEDED(hr_create_descriptor_heap));
		}

		ID3D12DescriptorHeap *new_non_shader_visible_cbv_srv_uav_descriptor_heap = NULL;
		{
			D3D12_DESCRIPTOR_HEAP_DESC const non_shader_visible_descriptor_heap_desc = {
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				new_num_alloced,
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				0U};
			HRESULT hr_create_descriptor_heap = this->m_device->CreateDescriptorHeap(&non_shader_visible_descriptor_heap_desc, IID_PPV_ARGS(&new_non_shader_visible_cbv_srv_uav_descriptor_heap));
			assert(SUCCEEDED(hr_create_descriptor_heap));
		}

		if (0U < this->m_cbv_srv_uav_descriptor_heap_num_alloced)
		{
			assert(NULL != this->m_shader_visible_cbv_srv_uav_descriptor_heap);
			this->m_shader_visible_cbv_srv_uav_descriptor_heap->Release();
			this->m_shader_visible_cbv_srv_uav_descriptor_heap = NULL;

			assert(NULL != this->m_non_shader_visible_cbv_srv_uav_descriptor_heap);
			this->m_device->CopyDescriptorsSimple(this->m_cbv_srv_uav_descriptor_heap_num_alloced, new_shader_visible_cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), this->m_non_shader_visible_cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			this->m_device->CopyDescriptorsSimple(this->m_cbv_srv_uav_descriptor_heap_num_alloced, new_non_shader_visible_cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), this->m_non_shader_visible_cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			this->m_non_shader_visible_cbv_srv_uav_descriptor_heap->Release();
			this->m_non_shader_visible_cbv_srv_uav_descriptor_heap = NULL;
		}
		else
		{
			assert(NULL == this->m_shader_visible_cbv_srv_uav_descriptor_heap);
			assert(NULL == this->m_non_shader_visible_cbv_srv_uav_descriptor_heap);
		}

		this->m_cbv_srv_uav_descriptor_heap_num_alloced = new_num_alloced;
		this->m_shader_visible_cbv_srv_uav_descriptor_heap = new_shader_visible_cbv_srv_uav_descriptor_heap;
		this->m_non_shader_visible_cbv_srv_uav_descriptor_heap = new_non_shader_visible_cbv_srv_uav_descriptor_heap;
	}

	uint32_t const base_descriptor_heap_index = this->m_cbv_srv_uav_descriptor_heap_next_free_index;
	this->m_cbv_srv_uav_descriptor_heap_next_free_index += num_descriptors;
	assert(this->m_cbv_srv_uav_descriptor_heap_next_free_index <= this->m_cbv_srv_uav_descriptor_heap_num_alloced);
#if defined(ENABLE_BUSY_LIST_VALIDATION) && ENABLE_BUSY_LIST_VALIDATION
	this->m_cbv_srv_uav_descriptor_heap_busy_list.emplace(base_descriptor_heap_index, num_descriptors);
#endif
	return base_descriptor_heap_index;
}

void brx_pal_d3d12_descriptor_allocator::free_cbv_srv_uav_descriptor(uint32_t base_descriptor_heap_index, uint32_t num_descriptors)
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	assert(this->m_statistic_get_shader_visible_cbv_srv_uav_cpu_descriptor_handle == this->m_statistic_get_non_shader_visible_cbv_srv_uav_cpu_descriptor_handle);
#endif

#if defined(ENABLE_BUSY_LIST_VALIDATION) && ENABLE_BUSY_LIST_VALIDATION
	{
		mcrt_map<uint32_t, uint32_t>::iterator const current = this->m_cbv_srv_uav_descriptor_heap_busy_list.find(base_descriptor_heap_index);
		assert(this->m_cbv_srv_uav_descriptor_heap_busy_list.end() != current);
		uint32_t validate_num_descriptors = current->second;
		assert(validate_num_descriptors == num_descriptors);
		this->m_cbv_srv_uav_descriptor_heap_busy_list.erase(current);
	}
#endif

	mcrt_map<uint32_t, uint32_t>::iterator const next = this->m_cbv_srv_uav_descriptor_heap_free_list.lower_bound(base_descriptor_heap_index);
	assert(this->m_cbv_srv_uav_descriptor_heap_free_list.end() == next || next->first >= (base_descriptor_heap_index + num_descriptors));

	mcrt_map<uint32_t, uint32_t>::iterator const previous = ((this->m_cbv_srv_uav_descriptor_heap_free_list.end() != next) || (!this->m_cbv_srv_uav_descriptor_heap_free_list.empty())) ? std::prev(next) : this->m_cbv_srv_uav_descriptor_heap_free_list.end();
	assert(this->m_cbv_srv_uav_descriptor_heap_free_list.end() == previous || (previous->first + previous->second) <= base_descriptor_heap_index);

	bool merge_previous = (this->m_cbv_srv_uav_descriptor_heap_free_list.end() != previous) && ((previous->first + previous->second) == base_descriptor_heap_index);
	bool merge_next = (this->m_cbv_srv_uav_descriptor_heap_free_list.end() != next) && ((base_descriptor_heap_index + num_descriptors) == next->first);
	if (merge_previous && merge_next)
	{
		previous->second += (num_descriptors + next->second);
		this->m_cbv_srv_uav_descriptor_heap_free_list.erase(next);
	}
	else if (merge_previous)
	{
		previous->second += num_descriptors;
	}
	else if (merge_next)
	{
		uint32_t const new_num_descriptors = num_descriptors + next->second;
		this->m_cbv_srv_uav_descriptor_heap_free_list.erase(next);
		this->m_cbv_srv_uav_descriptor_heap_free_list.insert(previous, {base_descriptor_heap_index, new_num_descriptors});
	}
	else
	{
		this->m_cbv_srv_uav_descriptor_heap_free_list.insert(previous, {base_descriptor_heap_index, num_descriptors});
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE brx_pal_d3d12_descriptor_allocator::get_shader_visible_cbv_srv_uav_cpu_descriptor_handle(uint32_t descriptor_heap_index) const
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	++this->m_statistic_get_shader_visible_cbv_srv_uav_cpu_descriptor_handle;
#endif
	D3D12_CPU_DESCRIPTOR_HANDLE cbv_srv_uav_cpu_descriptor_handle{this->m_shader_visible_cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * descriptor_heap_index};
	return cbv_srv_uav_cpu_descriptor_handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE brx_pal_d3d12_descriptor_allocator::get_non_shader_visible_cbv_srv_uav_cpu_descriptor_handle(uint32_t descriptor_heap_index) const
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	++this->m_statistic_get_non_shader_visible_cbv_srv_uav_cpu_descriptor_handle;
#endif

	D3D12_CPU_DESCRIPTOR_HANDLE cbv_srv_uav_cpu_descriptor_handle{this->m_non_shader_visible_cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * descriptor_heap_index};
	return cbv_srv_uav_cpu_descriptor_handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE brx_pal_d3d12_descriptor_allocator::get_cbv_srv_uav_gpu_descriptor_handle(uint32_t descriptor_heap_index) const
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	assert(this->m_statistic_get_shader_visible_cbv_srv_uav_cpu_descriptor_handle == this->m_statistic_get_non_shader_visible_cbv_srv_uav_cpu_descriptor_handle);
#endif

	D3D12_GPU_DESCRIPTOR_HANDLE cbv_srv_uav_gpu_descriptor_handle{this->m_shader_visible_cbv_srv_uav_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * descriptor_heap_index};
	return cbv_srv_uav_gpu_descriptor_handle;
}

uint32_t brx_pal_d3d12_descriptor_allocator::alloc_sampler_descriptor(uint32_t num_descriptors)
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	assert(this->m_statistic_get_shader_visible_sampler_cpu_descriptor_handle == this->m_statistic_get_non_shader_visible_sampler_cpu_descriptor_handle);
#endif

	assert(this->m_sampler_descriptor_heap_next_free_index <= this->m_sampler_descriptor_heap_num_alloced);

	for (mcrt_map<uint32_t, uint32_t>::iterator current = this->m_sampler_descriptor_heap_free_list.begin(); current != this->m_sampler_descriptor_heap_free_list.end(); current = std::next(current))
	{
		uint32_t const base_descriptor_heap_index = current->first;
		uint32_t const num_alloced_descriptor = current->second;

		if (num_alloced_descriptor >= num_descriptors)
		{
			this->m_sampler_descriptor_heap_free_list.erase(current);

			if (num_alloced_descriptor > num_descriptors)
			{
				this->m_sampler_descriptor_heap_free_list.emplace(base_descriptor_heap_index + num_descriptors, num_alloced_descriptor - num_descriptors);
			}

#if defined(ENABLE_BUSY_LIST_VALIDATION) && ENABLE_BUSY_LIST_VALIDATION
			this->m_sampler_descriptor_heap_busy_list.emplace(base_descriptor_heap_index, num_descriptors);
#endif
			return base_descriptor_heap_index;
		}
	}

	if ((this->m_sampler_descriptor_heap_next_free_index + num_descriptors) > this->m_sampler_descriptor_heap_num_alloced)
	{
		uint32_t aligned_minimum_num_alloced;
		{
#if defined(__GNUC__)
			aligned_num_descriptors = (1U << (31U - __builtin_clz(static_cast<uint32_t>(this->m_sampler_descriptor_heap_next_free_index + num_descriptors))));
#elif defined(_MSC_VER)
			unsigned long index;
			_BitScanReverse(&index, (this->m_sampler_descriptor_heap_next_free_index + num_descriptors));
			aligned_minimum_num_alloced = (1U << index);
#else
#error Unknown Compiler
#endif
		}

		uint32_t const new_num_alloced = std::max(2U * this->m_sampler_descriptor_heap_num_alloced, aligned_minimum_num_alloced);

		ID3D12DescriptorHeap *new_shader_visible_sampler_descriptor_heap = NULL;
		{
			D3D12_DESCRIPTOR_HEAP_DESC const shader_visible_descriptor_heap_desc = {
				D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
				new_num_alloced,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				0U};
			HRESULT hr_create_descriptor_heap = this->m_device->CreateDescriptorHeap(&shader_visible_descriptor_heap_desc, IID_PPV_ARGS(&new_shader_visible_sampler_descriptor_heap));
			assert(SUCCEEDED(hr_create_descriptor_heap));
		}

		ID3D12DescriptorHeap *new_non_shader_visible_sampler_descriptor_heap = NULL;
		{
			D3D12_DESCRIPTOR_HEAP_DESC const non_shader_visible_descriptor_heap_desc = {
				D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
				new_num_alloced,
				D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
				0U};
			HRESULT hr_create_descriptor_heap = this->m_device->CreateDescriptorHeap(&non_shader_visible_descriptor_heap_desc, IID_PPV_ARGS(&new_non_shader_visible_sampler_descriptor_heap));
			assert(SUCCEEDED(hr_create_descriptor_heap));
		}

		if (0U < this->m_sampler_descriptor_heap_num_alloced)
		{
			assert(NULL != this->m_shader_visible_sampler_descriptor_heap);
			this->m_shader_visible_sampler_descriptor_heap->Release();
			this->m_shader_visible_sampler_descriptor_heap = NULL;

			assert(NULL != this->m_non_shader_visible_sampler_descriptor_heap);
			this->m_device->CopyDescriptorsSimple(this->m_sampler_descriptor_heap_num_alloced, new_shader_visible_sampler_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), this->m_non_shader_visible_sampler_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			this->m_device->CopyDescriptorsSimple(this->m_sampler_descriptor_heap_num_alloced, new_non_shader_visible_sampler_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), this->m_non_shader_visible_sampler_descriptor_heap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			this->m_non_shader_visible_sampler_descriptor_heap->Release();
			this->m_non_shader_visible_sampler_descriptor_heap = NULL;
		}
		else
		{
			assert(NULL == this->m_shader_visible_sampler_descriptor_heap);
			assert(NULL == this->m_non_shader_visible_sampler_descriptor_heap);
		}

		this->m_sampler_descriptor_heap_num_alloced = new_num_alloced;
		this->m_shader_visible_sampler_descriptor_heap = new_shader_visible_sampler_descriptor_heap;
		this->m_non_shader_visible_sampler_descriptor_heap = new_non_shader_visible_sampler_descriptor_heap;
	}

	uint32_t const base_descriptor_heap_index = this->m_sampler_descriptor_heap_next_free_index;
	this->m_sampler_descriptor_heap_next_free_index += num_descriptors;
	assert(this->m_sampler_descriptor_heap_next_free_index <= this->m_sampler_descriptor_heap_num_alloced);
#if defined(ENABLE_BUSY_LIST_VALIDATION) && ENABLE_BUSY_LIST_VALIDATION
	this->m_sampler_descriptor_heap_busy_list.emplace(base_descriptor_heap_index, num_descriptors);
#endif
	return base_descriptor_heap_index;
}

void brx_pal_d3d12_descriptor_allocator::free_sampler_descriptor(uint32_t base_descriptor_heap_index, uint32_t num_descriptors)
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	assert(this->m_statistic_get_shader_visible_sampler_cpu_descriptor_handle == this->m_statistic_get_non_shader_visible_sampler_cpu_descriptor_handle);
#endif

#if defined(ENABLE_BUSY_LIST_VALIDATION) && ENABLE_BUSY_LIST_VALIDATION
	{
		mcrt_map<uint32_t, uint32_t>::iterator const current = this->m_sampler_descriptor_heap_busy_list.find(base_descriptor_heap_index);
		assert(this->m_sampler_descriptor_heap_busy_list.end() != current);
		uint32_t validate_num_descriptors = current->second;
		assert(validate_num_descriptors == num_descriptors);
		this->m_sampler_descriptor_heap_busy_list.erase(current);
	}
#endif

	mcrt_map<uint32_t, uint32_t>::iterator const next = this->m_sampler_descriptor_heap_free_list.lower_bound(base_descriptor_heap_index);
	assert(this->m_sampler_descriptor_heap_free_list.end() == next || next->first >= (base_descriptor_heap_index + num_descriptors));

	mcrt_map<uint32_t, uint32_t>::iterator const previous = ((this->m_sampler_descriptor_heap_free_list.end() != next) || (!this->m_sampler_descriptor_heap_free_list.empty())) ? std::prev(next) : this->m_sampler_descriptor_heap_free_list.end();
	assert(this->m_sampler_descriptor_heap_free_list.end() == previous || (previous->first + previous->second) <= base_descriptor_heap_index);

	bool merge_previous = (this->m_sampler_descriptor_heap_free_list.end() != previous) && ((previous->first + previous->second) == base_descriptor_heap_index);
	bool merge_next = (this->m_sampler_descriptor_heap_free_list.end() != next) && ((base_descriptor_heap_index + num_descriptors) == next->first);
	if (merge_previous && merge_next)
	{
		previous->second += (num_descriptors + next->second);
		this->m_sampler_descriptor_heap_free_list.erase(next);
	}
	else if (merge_previous)
	{
		previous->second += num_descriptors;
	}
	else if (merge_next)
	{
		uint32_t const new_num_descriptors = num_descriptors + next->second;
		this->m_sampler_descriptor_heap_free_list.erase(next);
		this->m_sampler_descriptor_heap_free_list.insert(previous, {base_descriptor_heap_index, new_num_descriptors});
	}
	else
	{
		this->m_sampler_descriptor_heap_free_list.insert(previous, {base_descriptor_heap_index, num_descriptors});
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE brx_pal_d3d12_descriptor_allocator::get_shader_visible_sampler_cpu_descriptor_handle(uint32_t descriptor_heap_index) const
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	++this->m_statistic_get_shader_visible_sampler_cpu_descriptor_handle;
#endif

	D3D12_CPU_DESCRIPTOR_HANDLE sample_cpu_descriptor_handle{this->m_shader_visible_sampler_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_sampler_descriptor_heap_descriptor_increment_size * descriptor_heap_index};
	return sample_cpu_descriptor_handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE brx_pal_d3d12_descriptor_allocator::get_non_shader_visible_sampler_cpu_descriptor_handle(uint32_t descriptor_heap_index) const
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	++this->m_statistic_get_non_shader_visible_sampler_cpu_descriptor_handle;
#endif

	D3D12_CPU_DESCRIPTOR_HANDLE sample_cpu_descriptor_handle{this->m_non_shader_visible_sampler_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_sampler_descriptor_heap_descriptor_increment_size * descriptor_heap_index};
	return sample_cpu_descriptor_handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE brx_pal_d3d12_descriptor_allocator::get_sampler_gpu_descriptor_handle(uint32_t descriptor_heap_index) const
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	assert(this->m_statistic_get_shader_visible_sampler_cpu_descriptor_handle == this->m_statistic_get_non_shader_visible_sampler_cpu_descriptor_handle);
#endif

	D3D12_GPU_DESCRIPTOR_HANDLE sample_gpu_descriptor_handle{this->m_shader_visible_sampler_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr + this->m_sampler_descriptor_heap_descriptor_increment_size * descriptor_heap_index};
	return sample_gpu_descriptor_handle;
}

void brx_pal_d3d12_descriptor_allocator::bind_command_list(ID3D12GraphicsCommandList *command_list) const
{
#if defined(ENABLE_STAGING_HEAP_VALIDATION) && ENABLE_STAGING_HEAP_VALIDATION
	assert(this->m_statistic_get_shader_visible_cbv_srv_uav_cpu_descriptor_handle == this->m_statistic_get_non_shader_visible_cbv_srv_uav_cpu_descriptor_handle);
	assert(this->m_statistic_get_shader_visible_sampler_cpu_descriptor_handle == this->m_statistic_get_non_shader_visible_sampler_cpu_descriptor_handle);
#endif

	if (NULL != this->m_shader_visible_cbv_srv_uav_descriptor_heap && NULL != this->m_shader_visible_sampler_descriptor_heap)
	{
		ID3D12DescriptorHeap *const descriptor_heaps[] = {this->m_shader_visible_cbv_srv_uav_descriptor_heap, this->m_shader_visible_sampler_descriptor_heap};
		command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
	}
	else if (NULL != this->m_shader_visible_cbv_srv_uav_descriptor_heap)
	{
		ID3D12DescriptorHeap *const descriptor_heaps[] = {this->m_shader_visible_cbv_srv_uav_descriptor_heap};
		command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
	}
	else if (NULL != this->m_shader_visible_sampler_descriptor_heap)
	{
		ID3D12DescriptorHeap *const descriptor_heaps[] = {this->m_shader_visible_sampler_descriptor_heap};
		command_list->SetDescriptorHeaps(sizeof(descriptor_heaps) / sizeof(descriptor_heaps[0]), descriptor_heaps);
	}
	else
	{
		// Do Nothing
	}
}