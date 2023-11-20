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
	: m_device(NULL)
{
}

void brx_pal_d3d12_descriptor_allocator::init(ID3D12Device *device)
{
	assert(NULL == this->m_device);
	this->m_device = device;

	this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	this->m_sampler_descriptor_heap_descriptor_increment_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	assert(this->m_pipeline_sets_descriptor_heap.empty());
}

void brx_pal_d3d12_descriptor_allocator::uninit()
{
	assert(this->m_pipeline_sets_descriptor_heap.empty());

	this->m_device = NULL;
}

brx_pal_d3d12_descriptor_allocator::~brx_pal_d3d12_descriptor_allocator()
{
	assert(NULL == this->m_device);

	assert(this->m_pipeline_sets_descriptor_heap.empty());
}

brx_pal_d3d12_pipeline_set_descriptor_heap const *brx_pal_d3d12_descriptor_allocator::create_or_get_pipeline_set_descriptor_heap(uint32_t descriptor_set_count, brx_pal_descriptor_set const *const *wrapped_descriptor_sets)
{
	brx_pal_d3d12_pipeline_set pipeline_set;
	pipeline_set.m_descriptor_sets.resize(descriptor_set_count);
	for (uint32_t descriptor_set_index = 0U; descriptor_set_index < descriptor_set_count; ++descriptor_set_index)
	{
		pipeline_set.m_descriptor_sets[descriptor_set_index] = static_cast<brx_pal_d3d12_descriptor_set const *>(wrapped_descriptor_sets[descriptor_set_index]);
	}

	auto const &found_pipeline_set_descriptor_heap = this->m_pipeline_sets_descriptor_heap.find(pipeline_set);
	if (this->m_pipeline_sets_descriptor_heap.end() != found_pipeline_set_descriptor_heap)
	{
		return &found_pipeline_set_descriptor_heap->second;
	}
	else
	{
		mcrt_vector<mcrt_vector<uint32_t>> descriptor_sets_base_descriptor_heap_indices(static_cast<size_t>(descriptor_set_count));
		ID3D12DescriptorHeap *cbv_srv_uav_descriptor_heap = NULL;
		ID3D12DescriptorHeap *sampler_descriptor_heap = NULL;
		{
			uint32_t cbv_srv_uav_descriptor_count = 0U;
			uint32_t sampler_descriptor_count = 0U;

			for (uint32_t descriptor_set_index = 0U; descriptor_set_index < descriptor_set_count; ++descriptor_set_index)
			{
				brx_pal_d3d12_descriptor_set const *const descriptor_set = static_cast<brx_pal_d3d12_descriptor_set const *>(wrapped_descriptor_sets[descriptor_set_index]);
				assert(NULL != descriptor_set);

				uint32_t const binding_count = descriptor_set->get_descriptor_binding_count();
				brx_pal_d3d12_descriptor_binding const *const descriptor_bindings = descriptor_set->get_descriptor_bindings();

				for (uint32_t binding_index = 0U; binding_index < binding_count; ++binding_index)
				{
					brx_pal_d3d12_descriptor_binding const &descriptor_binding = descriptor_bindings[binding_index];

					switch (descriptor_binding.m_root_parameter_type)
					{
					case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
					{
						assert(descriptor_binding.m_root_descriptor_table_resources.empty());
						assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_samplers.empty());
						descriptor_sets_base_descriptor_heap_indices[descriptor_set_index].push_back(INVALID_BASE_DESCRIPTOR_HEAP_INDEX);
					}
					break;
					case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
					{
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
						assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_samplers.empty());
						descriptor_sets_base_descriptor_heap_indices[descriptor_set_index].push_back(cbv_srv_uav_descriptor_count);
						cbv_srv_uav_descriptor_count += descriptor_binding.m_root_descriptor_table_num_descriptors;
					}
					break;
					case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					{
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
						assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_unordered_access_views.size());
						assert(descriptor_binding.m_root_descriptor_table_samplers.empty());
						descriptor_sets_base_descriptor_heap_indices[descriptor_set_index].push_back(cbv_srv_uav_descriptor_count);
						cbv_srv_uav_descriptor_count += descriptor_binding.m_root_descriptor_table_num_descriptors;
					}
					break;
					case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					{
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
						assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_samplers.empty());
						descriptor_sets_base_descriptor_heap_indices[descriptor_set_index].push_back(cbv_srv_uav_descriptor_count);
						cbv_srv_uav_descriptor_count += descriptor_binding.m_root_descriptor_table_num_descriptors;
					}
					break;
					case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					{
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
						assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_unordered_access_views.size());
						assert(descriptor_binding.m_root_descriptor_table_samplers.empty());
						descriptor_sets_base_descriptor_heap_indices[descriptor_set_index].push_back(cbv_srv_uav_descriptor_count);
						cbv_srv_uav_descriptor_count += descriptor_binding.m_root_descriptor_table_num_descriptors;
					}
					break;
					case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
					{
						assert(descriptor_binding.m_root_descriptor_table_resources.empty());
						assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_samplers.size());
						descriptor_sets_base_descriptor_heap_indices[descriptor_set_index].push_back(sampler_descriptor_count);
						sampler_descriptor_count += descriptor_binding.m_root_descriptor_table_num_descriptors;
					}
					break;
					case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
					{
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
						assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
						assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
						assert(descriptor_binding.m_root_descriptor_table_samplers.empty());
						descriptor_sets_base_descriptor_heap_indices[descriptor_set_index].push_back(cbv_srv_uav_descriptor_count);
						cbv_srv_uav_descriptor_count += descriptor_binding.m_root_descriptor_table_num_descriptors;
					}
					break;
					default:
					{
						assert(false);
					}
					}
				}

				assert(descriptor_sets_base_descriptor_heap_indices[descriptor_set_index].size() == binding_count);
			}

			if (cbv_srv_uav_descriptor_count > 0U)
			{
				D3D12_DESCRIPTOR_HEAP_DESC const descriptor_heap_desc = {
					D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
					cbv_srv_uav_descriptor_count,
					D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
					0U};
				HRESULT hr_create_descriptor_heap = this->m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&cbv_srv_uav_descriptor_heap));
				assert(SUCCEEDED(hr_create_descriptor_heap));
			}
			else
			{
				assert(NULL == cbv_srv_uav_descriptor_heap);
			}

			if (sampler_descriptor_count > 0U)
			{
				D3D12_DESCRIPTOR_HEAP_DESC const descriptor_heap_desc = {
					D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
					sampler_descriptor_count,
					D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
					0U};
				HRESULT hr_create_descriptor_heap = this->m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&sampler_descriptor_heap));
				assert(SUCCEEDED(hr_create_descriptor_heap));
			}
			else
			{
				assert(NULL == sampler_descriptor_heap);
			}
		}

		mcrt_vector<mcrt_vector<D3D12_GPU_DESCRIPTOR_HANDLE>> descriptor_sets_base_descriptors(static_cast<size_t>(descriptor_set_count));

		for (uint32_t descriptor_set_index = 0U; descriptor_set_index < descriptor_set_count; ++descriptor_set_index)
		{
			brx_pal_d3d12_descriptor_set const *const descriptor_set = static_cast<brx_pal_d3d12_descriptor_set const *>(wrapped_descriptor_sets[descriptor_set_index]);
			assert(NULL != descriptor_set);

			uint32_t const binding_count = descriptor_set->get_descriptor_binding_count();
			brx_pal_d3d12_descriptor_binding const *const descriptor_bindings = descriptor_set->get_descriptor_bindings();

			for (uint32_t binding_index = 0U; binding_index < binding_count; ++binding_index)
			{
				brx_pal_d3d12_descriptor_binding const &descriptor_binding = descriptor_bindings[binding_index];

				uint32_t const base_descriptor_heap_index = descriptor_sets_base_descriptor_heap_indices[descriptor_set_index][binding_index];

				switch (descriptor_binding.m_root_parameter_type)
				{
				case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
				{
					assert(descriptor_binding.m_root_descriptor_table_resources.empty());
					assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_samplers.empty());
					assert(INVALID_BASE_DESCRIPTOR_HEAP_INDEX == base_descriptor_heap_index);
					descriptor_sets_base_descriptors[descriptor_set_index].push_back(INVALID_BASE_DESCRIPTOR_HANDLE);
				}
				break;
				case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
				{
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
					assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

					assert(NULL != cbv_srv_uav_descriptor_heap);

					for (uint32_t descriptor_index = 0U; descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors; ++descriptor_index)
					{
						ID3D12Resource *const resource = descriptor_binding.m_root_descriptor_table_resources[descriptor_index];
						D3D12_SHADER_RESOURCE_VIEW_DESC const &shader_resource_view = descriptor_binding.m_root_descriptor_table_shader_resource_views[descriptor_index];
						this->m_device->CreateShaderResourceView(resource, &shader_resource_view, D3D12_CPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * (base_descriptor_heap_index + descriptor_index)});
					}

					descriptor_sets_base_descriptors[descriptor_set_index].push_back(D3D12_GPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * base_descriptor_heap_index});
				}
				break;
				case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				{
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
					assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_unordered_access_views.size());
					assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

					assert(NULL != cbv_srv_uav_descriptor_heap);

					for (uint32_t descriptor_index = 0U; descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors; ++descriptor_index)
					{
						ID3D12Resource *const resource = descriptor_binding.m_root_descriptor_table_resources[descriptor_index];
						D3D12_UNORDERED_ACCESS_VIEW_DESC const &unordered_access_view = descriptor_binding.m_root_descriptor_table_unordered_access_views[descriptor_index];
						this->m_device->CreateUnorderedAccessView(resource, NULL, &unordered_access_view, D3D12_CPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * (base_descriptor_heap_index + descriptor_index)});
					}

					descriptor_sets_base_descriptors[descriptor_set_index].push_back(D3D12_GPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * base_descriptor_heap_index});
				}
				break;
				case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				{
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
					assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

					assert(NULL != cbv_srv_uav_descriptor_heap);

					for (uint32_t descriptor_index = 0U; descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors; ++descriptor_index)
					{
						ID3D12Resource *const resource = descriptor_binding.m_root_descriptor_table_resources[descriptor_index];
						D3D12_SHADER_RESOURCE_VIEW_DESC const &shader_resource_view = descriptor_binding.m_root_descriptor_table_shader_resource_views[descriptor_index];
						this->m_device->CreateShaderResourceView(resource, &shader_resource_view, D3D12_CPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * (base_descriptor_heap_index + descriptor_index)});
					}

					descriptor_sets_base_descriptors[descriptor_set_index].push_back(D3D12_GPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * base_descriptor_heap_index});
				}
				break;
				case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				{
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
					assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_unordered_access_views.size());
					assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

					assert(NULL != cbv_srv_uav_descriptor_heap);

					for (uint32_t descriptor_index = 0U; descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors; ++descriptor_index)
					{
						ID3D12Resource *const resource = descriptor_binding.m_root_descriptor_table_resources[descriptor_index];
						D3D12_UNORDERED_ACCESS_VIEW_DESC const &unordered_access_view = descriptor_binding.m_root_descriptor_table_unordered_access_views[descriptor_index];
						this->m_device->CreateUnorderedAccessView(resource, NULL, &unordered_access_view, D3D12_CPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * (base_descriptor_heap_index + descriptor_index)});
					}

					descriptor_sets_base_descriptors[descriptor_set_index].push_back(D3D12_GPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * base_descriptor_heap_index});
				}
				break;
				case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
				{
					assert(descriptor_binding.m_root_descriptor_table_resources.empty());
					assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_samplers.size());

					assert(NULL != sampler_descriptor_heap);

					for (uint32_t descriptor_index = 0U; descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors; ++descriptor_index)
					{
						D3D12_SAMPLER_DESC const &sample = descriptor_binding.m_root_descriptor_table_samplers[descriptor_index];
						this->m_device->CreateSampler(&sample, D3D12_CPU_DESCRIPTOR_HANDLE{sampler_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_sampler_descriptor_heap_descriptor_increment_size * (base_descriptor_heap_index + descriptor_index)});
					}

					descriptor_sets_base_descriptors[descriptor_set_index].push_back(D3D12_GPU_DESCRIPTOR_HANDLE{sampler_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr + this->m_sampler_descriptor_heap_descriptor_increment_size * base_descriptor_heap_index});
				}
				break;
				case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
				{
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
					assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
					assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
					assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

					assert(NULL != cbv_srv_uav_descriptor_heap);

					for (uint32_t descriptor_index = 0U; descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors; ++descriptor_index)
					{
						D3D12_SHADER_RESOURCE_VIEW_DESC const &shader_resource_view = descriptor_binding.m_root_descriptor_table_shader_resource_views[descriptor_index];
						this->m_device->CreateShaderResourceView(NULL, &shader_resource_view, D3D12_CPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetCPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * (base_descriptor_heap_index + descriptor_index)});
					}

					descriptor_sets_base_descriptors[descriptor_set_index].push_back(D3D12_GPU_DESCRIPTOR_HANDLE{cbv_srv_uav_descriptor_heap->GetGPUDescriptorHandleForHeapStart().ptr + this->m_cbv_srv_uav_descriptor_heap_descriptor_increment_size * base_descriptor_heap_index});
				}
				break;
				default:
				{
					assert(false);
				}
				}
			}

			assert(descriptor_sets_base_descriptors[descriptor_set_index].size() == binding_count);
		}

		return &this->m_pipeline_sets_descriptor_heap.insert(found_pipeline_set_descriptor_heap, std::pair<brx_pal_d3d12_pipeline_set, brx_pal_d3d12_pipeline_set_descriptor_heap>{pipeline_set, brx_pal_d3d12_pipeline_set_descriptor_heap{std::move(descriptor_sets_base_descriptors), cbv_srv_uav_descriptor_heap, sampler_descriptor_heap}})->second;
	}
}

void brx_pal_d3d12_descriptor_allocator::invalidate_descriptor_set(brx_pal_descriptor_set const *wrapped_descriptor_set)
{
	brx_pal_d3d12_descriptor_set const *const descriptor_set = static_cast<brx_pal_d3d12_descriptor_set const *>(wrapped_descriptor_set);

	auto pipeline_set_descriptor_heap = this->m_pipeline_sets_descriptor_heap.begin();
	while (this->m_pipeline_sets_descriptor_heap.end() != pipeline_set_descriptor_heap)
	{
		bool found_descriptor_set = false;
		for (brx_pal_d3d12_descriptor_set const *const pipeline_set_descriptor_set : pipeline_set_descriptor_heap->first.m_descriptor_sets)
		{
			if (pipeline_set_descriptor_set == descriptor_set)
			{
				found_descriptor_set = true;
				break;
			}
		}

		if (found_descriptor_set)
		{
			ID3D12DescriptorHeap *const cbv_srv_uav_descriptor_heap = pipeline_set_descriptor_heap->second.m_cbv_srv_uav_descriptor_heap;
			ID3D12DescriptorHeap *const sampler_descriptor_heap = pipeline_set_descriptor_heap->second.m_sampler_descriptor_heap;

			if (NULL != cbv_srv_uav_descriptor_heap)
			{
				cbv_srv_uav_descriptor_heap->Release();
			}

			if (NULL != sampler_descriptor_heap)
			{
				sampler_descriptor_heap->Release();
			}

			pipeline_set_descriptor_heap = this->m_pipeline_sets_descriptor_heap.erase(pipeline_set_descriptor_heap);
		}
		else
		{
			++pipeline_set_descriptor_heap;
		}
	}
}
