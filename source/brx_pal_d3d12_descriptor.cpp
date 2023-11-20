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

brx_pal_d3d12_descriptor_set_layout::brx_pal_d3d12_descriptor_set_layout()
{
}

void brx_pal_d3d12_descriptor_set_layout::init(uint32_t descriptor_set_binding_count, BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING const *wrapped_descriptor_set_bindings)
{
	assert(0U == this->m_descriptor_layouts.size());
	this->m_descriptor_layouts.resize(descriptor_set_binding_count);
	for (uint32_t binding_index = 0U; binding_index < descriptor_set_binding_count; ++binding_index)
	{
		switch (wrapped_descriptor_set_bindings[binding_index].descriptor_type)
		{
		case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
		{
			this->m_descriptor_layouts[binding_index].m_root_parameter_type = BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER;
			this->m_descriptor_layouts[binding_index].m_root_parameter_shader_register = wrapped_descriptor_set_bindings[binding_index].binding;
			this->m_descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors = -1;
			assert(1U == wrapped_descriptor_set_bindings[binding_index].descriptor_count);
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
		{
			this->m_descriptor_layouts[binding_index].m_root_parameter_type = BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER;
			this->m_descriptor_layouts[binding_index].m_root_parameter_shader_register = wrapped_descriptor_set_bindings[binding_index].binding;
			this->m_descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		{
			this->m_descriptor_layouts[binding_index].m_root_parameter_type = BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			this->m_descriptor_layouts[binding_index].m_root_parameter_shader_register = wrapped_descriptor_set_bindings[binding_index].binding;
			this->m_descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		{
			this->m_descriptor_layouts[binding_index].m_root_parameter_type = BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			this->m_descriptor_layouts[binding_index].m_root_parameter_shader_register = wrapped_descriptor_set_bindings[binding_index].binding;
			this->m_descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
		{
			this->m_descriptor_layouts[binding_index].m_root_parameter_type = BRX_PAL_DESCRIPTOR_TYPE_SAMPLER;
			this->m_descriptor_layouts[binding_index].m_root_parameter_shader_register = wrapped_descriptor_set_bindings[binding_index].binding;
			this->m_descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		{
			this->m_descriptor_layouts[binding_index].m_root_parameter_type = BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			this->m_descriptor_layouts[binding_index].m_root_parameter_shader_register = wrapped_descriptor_set_bindings[binding_index].binding;
			this->m_descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
		{
			this->m_descriptor_layouts[binding_index].m_root_parameter_type = BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE;
			this->m_descriptor_layouts[binding_index].m_root_parameter_shader_register = wrapped_descriptor_set_bindings[binding_index].binding;
			this->m_descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors = wrapped_descriptor_set_bindings[binding_index].descriptor_count;
		}
		break;
		default:
		{
			assert(false);
			this->m_descriptor_layouts[binding_index].m_root_parameter_type = static_cast<BRX_PAL_DESCRIPTOR_TYPE>(-1);
		}
		}
	}
}

void brx_pal_d3d12_descriptor_set_layout::uninit()
{
}

uint32_t brx_pal_d3d12_descriptor_set_layout::get_descriptor_layout_count() const
{
	return static_cast<uint32_t>(this->m_descriptor_layouts.size());
}

brx_pal_d3d12_descriptor_layout const *brx_pal_d3d12_descriptor_set_layout::get_descriptor_layouts() const
{
	brx_pal_d3d12_descriptor_layout const *descriptor_layouts;

	if (this->m_descriptor_layouts.size() > 0U)
	{
		descriptor_layouts = &this->m_descriptor_layouts[0];
	}
	else
	{
		descriptor_layouts = NULL;
	}

	return descriptor_layouts;
}

brx_pal_d3d12_pipeline_layout::brx_pal_d3d12_pipeline_layout() : m_root_signature(NULL)
{
}

void brx_pal_d3d12_pipeline_layout::init(ID3D12Device *device, decltype(D3D12SerializeRootSignature) *pfn_d3d12_serialize_root_signature, uint32_t descriptor_set_layout_count, brx_pal_descriptor_set_layout const *const *descriptor_set_layouts)
{
	mcrt_vector<D3D12_ROOT_PARAMETER> root_parameters;
	mcrt_vector<D3D12_DESCRIPTOR_RANGE> root_descriptor_table_ranges;
	for (uint32_t set_index = 0U; set_index < descriptor_set_layout_count; ++set_index)
	{
		uint32_t binding_count = static_cast<brx_pal_d3d12_descriptor_set_layout const *>(descriptor_set_layouts[set_index])->get_descriptor_layout_count();
		brx_pal_d3d12_descriptor_layout const *descriptor_layouts = static_cast<brx_pal_d3d12_descriptor_set_layout const *>(descriptor_set_layouts[set_index])->get_descriptor_layouts();
		for (uint32_t binding_index = 0U; binding_index < binding_count; ++binding_index)
		{
			switch (descriptor_layouts[binding_index].m_root_parameter_type)
			{
			case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
			{
				uint32_t const new_root_parameter_index = static_cast<uint32_t>(root_parameters.size());
				root_parameters.emplace_back();

				root_parameters[new_root_parameter_index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				root_parameters[new_root_parameter_index].Descriptor.ShaderRegister = descriptor_layouts[binding_index].m_root_parameter_shader_register;
				root_parameters[new_root_parameter_index].Descriptor.RegisterSpace = set_index;
				root_parameters[new_root_parameter_index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			break;
			case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
			{
				uint32_t const new_root_descriptor_table_range_index = static_cast<uint32_t>(root_descriptor_table_ranges.size());
				root_descriptor_table_ranges.emplace_back();

				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				if (BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors)
				{
					root_descriptor_table_ranges[new_root_descriptor_table_range_index].NumDescriptors = descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors;
				}
				else
				{
					assert(binding_count == (binding_index + 1U));
					root_descriptor_table_ranges[new_root_descriptor_table_range_index].NumDescriptors = UINT32_MAX;
				}
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].BaseShaderRegister = descriptor_layouts[binding_index].m_root_parameter_shader_register;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RegisterSpace = set_index;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].OffsetInDescriptorsFromTableStart = 0U;

				uint32_t const new_root_parameter_index = static_cast<uint32_t>(root_parameters.size());
				root_parameters.emplace_back();

				root_parameters[new_root_parameter_index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				root_parameters[new_root_parameter_index].DescriptorTable.NumDescriptorRanges = 1U;
				root_parameters[new_root_parameter_index].DescriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE *>(static_cast<uintptr_t>(new_root_descriptor_table_range_index));
				root_parameters[new_root_parameter_index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			break;
			case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			{
				uint32_t const new_root_descriptor_table_range_index = static_cast<uint32_t>(root_descriptor_table_ranges.size());
				root_descriptor_table_ranges.emplace_back();

				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors);
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].NumDescriptors = descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].BaseShaderRegister = descriptor_layouts[binding_index].m_root_parameter_shader_register;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RegisterSpace = set_index;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].OffsetInDescriptorsFromTableStart = 0U;

				uint32_t const new_root_parameter_index = static_cast<uint32_t>(root_parameters.size());
				root_parameters.emplace_back();

				root_parameters[new_root_parameter_index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				root_parameters[new_root_parameter_index].DescriptorTable.NumDescriptorRanges = 1U;
				root_parameters[new_root_parameter_index].DescriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE *>(static_cast<uintptr_t>(new_root_descriptor_table_range_index));
				root_parameters[new_root_parameter_index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			break;
			case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			{
				uint32_t const new_root_descriptor_table_range_index = static_cast<uint32_t>(root_descriptor_table_ranges.size());
				root_descriptor_table_ranges.emplace_back();

				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				if (BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors)
				{
					root_descriptor_table_ranges[new_root_descriptor_table_range_index].NumDescriptors = descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors;
				}
				else
				{
					assert(binding_count == (binding_index + 1U));
					root_descriptor_table_ranges[new_root_descriptor_table_range_index].NumDescriptors = UINT32_MAX;
				}
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].BaseShaderRegister = descriptor_layouts[binding_index].m_root_parameter_shader_register;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RegisterSpace = set_index;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].OffsetInDescriptorsFromTableStart = 0U;

				uint32_t const new_root_parameter_index = static_cast<uint32_t>(root_parameters.size());
				root_parameters.emplace_back();

				root_parameters[new_root_parameter_index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				root_parameters[new_root_parameter_index].DescriptorTable.NumDescriptorRanges = 1U;
				root_parameters[new_root_parameter_index].DescriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE *>(static_cast<uintptr_t>(new_root_descriptor_table_range_index));
				root_parameters[new_root_parameter_index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			break;
			case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
			{
				uint32_t const new_root_descriptor_table_range_index = static_cast<uint32_t>(root_descriptor_table_ranges.size());
				root_descriptor_table_ranges.emplace_back();

				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
				assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors);
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].NumDescriptors = descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].BaseShaderRegister = descriptor_layouts[binding_index].m_root_parameter_shader_register;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RegisterSpace = set_index;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].OffsetInDescriptorsFromTableStart = 0U;

				uint32_t const new_root_parameter_index = static_cast<uint32_t>(root_parameters.size());
				root_parameters.emplace_back();

				root_parameters[new_root_parameter_index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				root_parameters[new_root_parameter_index].DescriptorTable.NumDescriptorRanges = 1U;
				root_parameters[new_root_parameter_index].DescriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE *>(static_cast<uintptr_t>(new_root_descriptor_table_range_index));
				root_parameters[new_root_parameter_index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			break;
			case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			{
				uint32_t const new_root_descriptor_table_range_index = static_cast<uint32_t>(root_descriptor_table_ranges.size());
				root_descriptor_table_ranges.emplace_back();

				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].NumDescriptors = descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].BaseShaderRegister = descriptor_layouts[binding_index].m_root_parameter_shader_register;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RegisterSpace = set_index;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].OffsetInDescriptorsFromTableStart = 0U;

				uint32_t const new_root_parameter_index = static_cast<uint32_t>(root_parameters.size());
				root_parameters.emplace_back();

				root_parameters[new_root_parameter_index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				root_parameters[new_root_parameter_index].DescriptorTable.NumDescriptorRanges = 1U;
				root_parameters[new_root_parameter_index].DescriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE *>(static_cast<uintptr_t>(new_root_descriptor_table_range_index));
				root_parameters[new_root_parameter_index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			break;
			case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
			{
				uint32_t const new_root_descriptor_table_range_index = static_cast<uint32_t>(root_descriptor_table_ranges.size());
				root_descriptor_table_ranges.emplace_back();

				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors);
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].NumDescriptors = descriptor_layouts[binding_index].m_root_descriptor_table_num_descriptors;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].BaseShaderRegister = descriptor_layouts[binding_index].m_root_parameter_shader_register;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].RegisterSpace = set_index;
				root_descriptor_table_ranges[new_root_descriptor_table_range_index].OffsetInDescriptorsFromTableStart = 0U;

				uint32_t const new_root_parameter_index = static_cast<uint32_t>(root_parameters.size());
				root_parameters.emplace_back();

				root_parameters[new_root_parameter_index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
				root_parameters[new_root_parameter_index].DescriptorTable.NumDescriptorRanges = 1U;
				root_parameters[new_root_parameter_index].DescriptorTable.pDescriptorRanges = reinterpret_cast<D3D12_DESCRIPTOR_RANGE *>(static_cast<uintptr_t>(new_root_descriptor_table_range_index));
				root_parameters[new_root_parameter_index].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			}
			break;
			default:
			{
				assert(false);
			}
			}
		}
	}

	// The memory address within "root_descriptor_table_ranges" may change when "emplace_back"
	uint32_t const num_root_parameters = static_cast<uint32_t>(root_parameters.size());
	for (uint32_t root_parameter_index = 0U; root_parameter_index < num_root_parameters; ++root_parameter_index)
	{
		if (D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE == root_parameters[root_parameter_index].ParameterType)
		{
			uint32_t const new_root_descriptor_table_range_index = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(root_parameters[root_parameter_index].DescriptorTable.pDescriptorRanges));
			root_parameters[root_parameter_index].DescriptorTable.pDescriptorRanges = &root_descriptor_table_ranges[new_root_descriptor_table_range_index];
		}
	}

	D3D12_ROOT_SIGNATURE_DESC const root_signature_desc = {
		num_root_parameters,
		&root_parameters[0],
		0U,
		NULL,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};

	ID3DBlob *new_blob_with_root_signature = NULL;
	HRESULT const hr_serialize_root_signature = pfn_d3d12_serialize_root_signature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &new_blob_with_root_signature, NULL);
	assert(SUCCEEDED(hr_serialize_root_signature));

	assert(NULL == this->m_root_signature);
	HRESULT const hr_create_root_signature = device->CreateRootSignature(0U, new_blob_with_root_signature->GetBufferPointer(), new_blob_with_root_signature->GetBufferSize(), IID_PPV_ARGS(&this->m_root_signature));
	assert(SUCCEEDED(hr_create_root_signature));

	new_blob_with_root_signature->Release();
}

void brx_pal_d3d12_pipeline_layout::uninit()
{
	assert(NULL != this->m_root_signature);

	this->m_root_signature->Release();

	this->m_root_signature = NULL;
}

brx_pal_d3d12_pipeline_layout::~brx_pal_d3d12_pipeline_layout()
{
	assert(NULL == this->m_root_signature);
}

ID3D12RootSignature *brx_pal_d3d12_pipeline_layout::get_root_signature() const
{
	return this->m_root_signature;
}

brx_pal_d3d12_descriptor_set::brx_pal_d3d12_descriptor_set()
{
}

void brx_pal_d3d12_descriptor_set::init(brx_pal_d3d12_descriptor_allocator *descriptor_allocator, brx_pal_descriptor_set_layout const *wrapped_descriptor_set_layout, uint32_t unbounded_descriptor_count)
{
	assert(NULL != wrapped_descriptor_set_layout);
	brx_pal_d3d12_descriptor_set_layout const *const unwrapped_descriptor_set_layout = static_cast<brx_pal_d3d12_descriptor_set_layout const *>(wrapped_descriptor_set_layout);

	uint32_t const descriptor_layout_count = unwrapped_descriptor_set_layout->get_descriptor_layout_count();
	brx_pal_d3d12_descriptor_layout const *unwrapped_descriptor_layouts = unwrapped_descriptor_set_layout->get_descriptor_layouts();

	assert(0U == this->m_descriptor_bindings.size());
	this->m_descriptor_bindings.resize(descriptor_layout_count);
	for (uint32_t descriptor_layout_index = 0U; descriptor_layout_index < descriptor_layout_count; ++descriptor_layout_index)
	{
		brx_pal_d3d12_descriptor_layout const &unwrapped_descriptor_layout = unwrapped_descriptor_layouts[descriptor_layout_index];
		brx_pal_d3d12_descriptor_binding &descriptor_binding = this->m_descriptor_bindings[descriptor_layout_index];

		switch (unwrapped_descriptor_layout.m_root_parameter_type)
		{
		case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
		{
			descriptor_binding.m_root_parameter_type = unwrapped_descriptor_layout.m_root_parameter_type;
			descriptor_binding.m_root_parameter_shader_register = unwrapped_descriptor_layout.m_root_parameter_shader_register;
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
		{
			descriptor_binding.m_root_parameter_type = unwrapped_descriptor_layout.m_root_parameter_type;
			descriptor_binding.m_root_parameter_shader_register = unwrapped_descriptor_layout.m_root_parameter_shader_register;
			if (BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors)
			{
				descriptor_binding.m_root_descriptor_table_num_descriptors = unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors;
				descriptor_binding.m_root_descriptor_table_resources.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
				descriptor_binding.m_root_descriptor_table_shader_resource_views.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
			}
			else
			{
				assert(descriptor_layout_count == (descriptor_layout_index + 1U));
				descriptor_binding.m_root_descriptor_table_num_descriptors = unbounded_descriptor_count;
				descriptor_binding.m_root_descriptor_table_resources.resize(unbounded_descriptor_count);
				descriptor_binding.m_root_descriptor_table_shader_resource_views.resize(unbounded_descriptor_count);
			}
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		{
			descriptor_binding.m_root_parameter_type = unwrapped_descriptor_layout.m_root_parameter_type;
			descriptor_binding.m_root_parameter_shader_register = unwrapped_descriptor_layout.m_root_parameter_shader_register;
			assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
			descriptor_binding.m_root_descriptor_table_num_descriptors = unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors;
			descriptor_binding.m_root_descriptor_table_resources.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
			descriptor_binding.m_root_descriptor_table_unordered_access_views.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		{
			descriptor_binding.m_root_parameter_type = unwrapped_descriptor_layout.m_root_parameter_type;
			descriptor_binding.m_root_parameter_shader_register = unwrapped_descriptor_layout.m_root_parameter_shader_register;
			if (BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors)
			{
				descriptor_binding.m_root_descriptor_table_num_descriptors = unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors;
				descriptor_binding.m_root_descriptor_table_resources.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
				descriptor_binding.m_root_descriptor_table_shader_resource_views.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
			}
			else
			{
				assert(descriptor_layout_count == (descriptor_layout_index + 1U));
				descriptor_binding.m_root_descriptor_table_num_descriptors = unbounded_descriptor_count;
				descriptor_binding.m_root_descriptor_table_resources.resize(unbounded_descriptor_count);
				descriptor_binding.m_root_descriptor_table_shader_resource_views.resize(unbounded_descriptor_count);
			}
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		{
			descriptor_binding.m_root_parameter_type = unwrapped_descriptor_layout.m_root_parameter_type;
			descriptor_binding.m_root_parameter_shader_register = unwrapped_descriptor_layout.m_root_parameter_shader_register;
			assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
			descriptor_binding.m_root_descriptor_table_num_descriptors = unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors;
			descriptor_binding.m_root_descriptor_table_resources.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
			descriptor_binding.m_root_descriptor_table_unordered_access_views.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
		{
			descriptor_binding.m_root_parameter_type = unwrapped_descriptor_layout.m_root_parameter_type;
			descriptor_binding.m_root_parameter_shader_register = unwrapped_descriptor_layout.m_root_parameter_shader_register;
			assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
			descriptor_binding.m_root_descriptor_table_num_descriptors = unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors;
			descriptor_binding.m_root_descriptor_table_samplers.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
		}
		break;
		case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
		{
			descriptor_binding.m_root_parameter_type = unwrapped_descriptor_layout.m_root_parameter_type;
			descriptor_binding.m_root_parameter_shader_register = unwrapped_descriptor_layout.m_root_parameter_shader_register;
			assert(BRX_PAL_DESCRIPTOR_SET_LAYOUT_BINDING_DESCRIPTOR_COUNT_UNBOUNDED != unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
			descriptor_binding.m_root_descriptor_table_num_descriptors = unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors;
			descriptor_binding.m_root_descriptor_table_shader_resource_views.resize(unwrapped_descriptor_layout.m_root_descriptor_table_num_descriptors);
		}
		break;
		default:
		{
			assert(false);
			descriptor_binding.m_root_parameter_type = static_cast<BRX_PAL_DESCRIPTOR_TYPE>(-1);
		}
		}
	}
}

void brx_pal_d3d12_descriptor_set::uninit(brx_pal_d3d12_descriptor_allocator *descriptor_allocator)
{
	descriptor_allocator->invalidate_descriptor_set(this);

	this->m_descriptor_bindings.clear();
}

brx_pal_d3d12_descriptor_set::~brx_pal_d3d12_descriptor_set()
{
	assert(0U == this->m_descriptor_bindings.size());
}

void brx_pal_d3d12_descriptor_set::write_descriptor(ID3D12Device *device, brx_pal_d3d12_descriptor_allocator *descriptor_allocator, uint32_t dst_binding, BRX_PAL_DESCRIPTOR_TYPE wrapped_descriptor_type, uint32_t dst_descriptor_start_index, uint32_t src_descriptor_count, brx_pal_uniform_upload_buffer const *const *src_dynamic_uniform_buffers, uint32_t const *src_dynamic_uniform_buffer_ranges, brx_pal_read_only_storage_buffer const *const *src_read_only_storage_buffers, brx_pal_storage_buffer const *const *src_storage_buffers, brx_pal_sampled_image const *const *src_sampled_images, brx_pal_storage_image const *const *src_storage_images, brx_pal_sampler const *const *src_samplers, brx_pal_top_level_acceleration_structure const *const *src_top_level_acceleration_structures)
{
	// we are NOT allowed to write the descriptor when the GPU is using
	// it is safe the delete the cache here
	descriptor_allocator->invalidate_descriptor_set(this);

	uint32_t destination_descriptor_layout_index = -1;
	{
		uint32_t const descriptor_layout_count = static_cast<uint32_t>(this->m_descriptor_bindings.size());
		assert(0U < descriptor_layout_count);

		for (uint32_t descriptor_layout_index = 0U; descriptor_layout_index < descriptor_layout_count; ++descriptor_layout_index)
		{
			if (this->m_descriptor_bindings[descriptor_layout_index].m_root_parameter_shader_register == dst_binding)
			{
				destination_descriptor_layout_index = descriptor_layout_index;
				break;
			}
		}

		assert(static_cast<uint32_t>(-1) != destination_descriptor_layout_index);
	}

	brx_pal_d3d12_descriptor_binding &descriptor_binding = this->m_descriptor_bindings[destination_descriptor_layout_index];

	assert(wrapped_descriptor_type == descriptor_binding.m_root_parameter_type);

	switch (wrapped_descriptor_type)
	{
	case BRX_PAL_DESCRIPTOR_TYPE_DYNAMIC_UNIFORM_BUFFER:
	{
		assert(0U == dst_descriptor_start_index);
		assert(1U == src_descriptor_count);

		assert(NULL != src_dynamic_uniform_buffers);
		assert(NULL != src_dynamic_uniform_buffer_ranges);
		assert(NULL == src_read_only_storage_buffers);
		assert(NULL == src_storage_buffers);
		assert(NULL == src_sampled_images);
		assert(NULL == src_storage_images);
		assert(NULL == src_samplers);
		assert(NULL == src_top_level_acceleration_structures);

		assert(NULL != src_dynamic_uniform_buffers[0]);
		assert(NULL != src_dynamic_uniform_buffer_ranges[0]);

		assert(descriptor_binding.m_root_descriptor_table_resources.empty());
		assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

		descriptor_binding.m_root_constant_buffer_view_address_base = static_cast<brx_pal_d3d12_uniform_upload_buffer const *>(src_dynamic_uniform_buffers[0])->get_resource()->GetGPUVirtualAddress();
	}
	break;
	case BRX_PAL_DESCRIPTOR_TYPE_READ_ONLY_STORAGE_BUFFER:
	{
		assert(NULL == src_dynamic_uniform_buffers);
		assert(NULL == src_dynamic_uniform_buffer_ranges);
		assert(NULL != src_read_only_storage_buffers);
		assert(NULL == src_storage_buffers);
		assert(NULL == src_sampled_images);
		assert(NULL == src_storage_images);
		assert(NULL == src_samplers);
		assert(NULL == src_top_level_acceleration_structures);

		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
		assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

		for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
		{
			assert(NULL != src_read_only_storage_buffers[descriptor_index]);
			uint32_t const abs_descriptor_index = dst_descriptor_start_index + descriptor_index;
			assert(abs_descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors);

			ID3D12Resource *const resource = static_cast<brx_pal_d3d12_read_only_storage_buffer const *>(src_read_only_storage_buffers[descriptor_index])->get_resource();
			D3D12_SHADER_RESOURCE_VIEW_DESC const *const shader_resource_view_desc = static_cast<brx_pal_d3d12_read_only_storage_buffer const *>(src_read_only_storage_buffers[descriptor_index])->get_shader_resource_view_desc();

			descriptor_binding.m_root_descriptor_table_resources[abs_descriptor_index] = resource;
			descriptor_binding.m_root_descriptor_table_shader_resource_views[abs_descriptor_index] = (*shader_resource_view_desc);
		}
	}
	break;
	case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	{
		assert(NULL == src_dynamic_uniform_buffers);
		assert(NULL == src_dynamic_uniform_buffer_ranges);
		assert(NULL == src_read_only_storage_buffers);
		assert(NULL != src_storage_buffers);
		assert(NULL == src_sampled_images);
		assert(NULL == src_storage_images);
		assert(NULL == src_samplers);
		assert(NULL == src_top_level_acceleration_structures);

		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
		assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_unordered_access_views.size());
		assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

		for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
		{
			assert(NULL != src_storage_buffers[descriptor_index]);
			uint32_t const abs_descriptor_index = dst_descriptor_start_index + descriptor_index;
			assert(abs_descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors);

			ID3D12Resource *const resource = static_cast<brx_pal_d3d12_storage_buffer const *>(src_storage_buffers[descriptor_index])->get_resource();
			D3D12_UNORDERED_ACCESS_VIEW_DESC const *const unordered_access_view_desc = static_cast<brx_pal_d3d12_storage_buffer const *>(src_storage_buffers[descriptor_index])->get_unordered_access_view_desc();

			descriptor_binding.m_root_descriptor_table_resources[abs_descriptor_index] = resource;
			descriptor_binding.m_root_descriptor_table_unordered_access_views[abs_descriptor_index] = (*unordered_access_view_desc);
		}
	}
	break;
	case BRX_PAL_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
	{
		assert(NULL == src_dynamic_uniform_buffers);
		assert(NULL == src_dynamic_uniform_buffer_ranges);
		assert(NULL == src_read_only_storage_buffers);
		assert(NULL == src_storage_buffers);
		assert(NULL != src_sampled_images);
		assert(NULL == src_storage_images);
		assert(NULL == src_samplers);
		assert(NULL == src_top_level_acceleration_structures);

		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
		assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

		for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
		{
			assert(NULL != src_sampled_images[descriptor_index]);
			uint32_t const abs_descriptor_index = dst_descriptor_start_index + descriptor_index;
			assert(abs_descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors);

			ID3D12Resource *const resource = static_cast<brx_pal_d3d12_sampled_image const *>(src_sampled_images[descriptor_index])->get_resource();
			D3D12_SHADER_RESOURCE_VIEW_DESC const *const shader_resource_view_desc = static_cast<brx_pal_d3d12_sampled_image const *>(src_sampled_images[descriptor_index])->get_shader_resource_view_desc();

			descriptor_binding.m_root_descriptor_table_resources[abs_descriptor_index] = resource;
			descriptor_binding.m_root_descriptor_table_shader_resource_views[abs_descriptor_index] = (*shader_resource_view_desc);
		}
	}
	break;
	case BRX_PAL_DESCRIPTOR_TYPE_STORAGE_IMAGE:
	{
		assert(NULL == src_dynamic_uniform_buffers);
		assert(NULL == src_dynamic_uniform_buffer_ranges);
		assert(NULL == src_read_only_storage_buffers);
		assert(NULL == src_storage_buffers);
		assert(NULL == src_sampled_images);
		assert(NULL == src_samplers);
		assert(NULL != src_storage_images);
		assert(NULL == src_top_level_acceleration_structures);

		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_resources.size());
		assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_unordered_access_views.size());
		assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

		for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
		{
			assert(NULL != src_storage_images[descriptor_index]);
			uint32_t const abs_descriptor_index = dst_descriptor_start_index + descriptor_index;
			assert(abs_descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors);

			ID3D12Resource *const resource = static_cast<brx_pal_d3d12_storage_image const *>(src_storage_images[descriptor_index])->get_resource();
			D3D12_UNORDERED_ACCESS_VIEW_DESC const *const unordered_access_view_desc = static_cast<brx_pal_d3d12_storage_image const *>(src_storage_images[descriptor_index])->get_unordered_access_view_desc();

			descriptor_binding.m_root_descriptor_table_resources[abs_descriptor_index] = resource;
			descriptor_binding.m_root_descriptor_table_unordered_access_views[abs_descriptor_index] = (*unordered_access_view_desc);
		}
	}
	break;
	case BRX_PAL_DESCRIPTOR_TYPE_SAMPLER:
	{
		assert(NULL == src_dynamic_uniform_buffers);
		assert(NULL == src_dynamic_uniform_buffer_ranges);
		assert(NULL == src_read_only_storage_buffers);
		assert(NULL == src_storage_buffers);
		assert(NULL == src_sampled_images);
		assert(NULL == src_storage_images);
		assert(NULL != src_samplers);
		assert(NULL == src_top_level_acceleration_structures);

		assert(descriptor_binding.m_root_descriptor_table_resources.empty());
		assert(descriptor_binding.m_root_descriptor_table_shader_resource_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_samplers.size());

		for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
		{
			assert(NULL != src_samplers[descriptor_index]);
			uint32_t const abs_descriptor_index = dst_descriptor_start_index + descriptor_index;
			assert(abs_descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors);

			D3D12_SAMPLER_DESC const *const sampler_desc = static_cast<brx_pal_d3d12_sampler const *>(src_samplers[descriptor_index])->get_sampler_desc();

			descriptor_binding.m_root_descriptor_table_samplers[abs_descriptor_index] = (*sampler_desc);
		}
	}
	break;
	case BRX_PAL_DESCRIPTOR_TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE:
	{
		assert(NULL == src_dynamic_uniform_buffers);
		assert(NULL == src_dynamic_uniform_buffer_ranges);
		assert(NULL == src_storage_buffers);
		assert(NULL == src_sampled_images);
		assert(NULL == src_samplers);
		assert(NULL == src_storage_images);
		assert(NULL != src_top_level_acceleration_structures);

		assert(descriptor_binding.m_root_descriptor_table_resources.empty());
		assert(descriptor_binding.m_root_descriptor_table_num_descriptors == descriptor_binding.m_root_descriptor_table_shader_resource_views.size());
		assert(descriptor_binding.m_root_descriptor_table_unordered_access_views.empty());
		assert(descriptor_binding.m_root_descriptor_table_samplers.empty());

		for (uint32_t descriptor_index = 0U; descriptor_index < src_descriptor_count; ++descriptor_index)
		{
			assert(NULL != src_top_level_acceleration_structures[descriptor_index]);
			uint32_t const abs_descriptor_index = dst_descriptor_start_index + descriptor_index;
			assert(abs_descriptor_index < descriptor_binding.m_root_descriptor_table_num_descriptors);

			D3D12_SHADER_RESOURCE_VIEW_DESC const *const shader_resource_view_desc = static_cast<brx_pal_d3d12_top_level_acceleration_structure const *>(src_top_level_acceleration_structures[descriptor_index])->get_shader_resource_view_desc();

			descriptor_binding.m_root_descriptor_table_shader_resource_views[abs_descriptor_index] = (*shader_resource_view_desc);
		}
	}
	break;
	default:
	{
		assert(false);
	}
	}
}

uint32_t brx_pal_d3d12_descriptor_set::get_descriptor_binding_count() const
{
	return static_cast<uint32_t>(this->m_descriptor_bindings.size());
}

brx_pal_d3d12_descriptor_binding const *brx_pal_d3d12_descriptor_set::get_descriptor_bindings() const
{
	return this->m_descriptor_bindings.data();
}
