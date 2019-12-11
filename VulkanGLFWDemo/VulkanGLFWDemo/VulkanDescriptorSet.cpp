#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"

FVulkanDescriptorSetLayout::FVulkanDescriptorSetLayout()
{
	memset(m_LayoutTypes, 0, VK_DESCRIPTOR_TYPE_RANGE_SIZE * sizeof(uint32_t));
}

FVulkanDescriptorSetLayout::~FVulkanDescriptorSetLayout()
{
}

bool FVulkanDescriptorSetLayout::AddDescriptor(const VkDescriptorSetLayoutBinding & InDescriptor)
{
	if (InDescriptor.binding == m_LayoutBindings.size()) {
		m_LayoutTypes[InDescriptor.descriptorType] += InDescriptor.descriptorCount;
		m_LayoutBindings.push_back(InDescriptor);
		return true;
	}

	return false;
}

uint32_t FVulkanDescriptorSetLayout::GetNextBinding()
{
	return m_LayoutBindings.size();
}

FVulkanDescriptorSetLayout::FVulkanDescriptorSetLayout(const FVulkanDevice* InDevice, const FVulkanDescriptorSetLayout & InLayout)
	:m_Device(InDevice)
{
	memcpy(m_LayoutTypes, InLayout.m_LayoutTypes, VK_DESCRIPTOR_TYPE_RANGE_SIZE * sizeof(uint32_t));
	m_LayoutBindings.resize(InLayout.m_LayoutBindings.size());
	memcpy(m_LayoutBindings.data(), InLayout.m_LayoutBindings.data(), InLayout.m_LayoutBindings.size() * sizeof(VkDescriptorSetLayoutBinding));
	/*m_Handle = InLayout.m_Handle;
	m_HandleId = InLayout.m_HandleId;*/
}

void FVulkanDescriptorSetLayout::Compile()
{
	if (!m_Device) return;

	Release();

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(m_LayoutBindings.size());
	layoutInfo.pBindings = m_LayoutBindings.data();

	if (vkCreateDescriptorSetLayout(m_Device->GetLogicalDevice(), &layoutInfo, nullptr, &m_Handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void FVulkanDescriptorSetLayout::Release()
{
	if (!m_Device) return;

	if (m_Handle) {
		vkDestroyDescriptorSetLayout(m_Device->GetLogicalDevice(), m_Handle, nullptr);
	}
}

bool FVulkanDescriptorSetLayout::CheckUniqueBind(uint32_t InIndex)
{
	for (size_t i = 0; i < m_LayoutBindings.size(); i++)
	{
		if (InIndex == m_LayoutBindings[i].binding) {
			return false;
		}
	}
	return true;
}


FVulkanDescriptorSet::FVulkanDescriptorSet()
{
}

FVulkanDescriptorSet::~FVulkanDescriptorSet()
{
}

FVulkanDescriptorSet::FVulkanDescriptorSet(const FVulkanDevice * InDevice, const FVulkanDescriptorSetManager * InOwner, const FVulkanDescriptorSetLayout & InLayout)
	:m_Device(InDevice), m_Owner(InOwner)
{
	m_Layout.reset(new FVulkanDescriptorSetLayout(InLayout));
}


void FVulkanDescriptorSet::Setup()
{
	if (m_Layout->m_LayoutBindings.size() == 0) {
		return;
	}
	m_Layout->Compile();

	//CreateSet
	VkDescriptorSetLayout layouts[] = { m_Layout->GetHandle() };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_Owner->GetPool();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(m_Device->GetLogicalDevice(), &allocInfo, &m_Handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

}

void FVulkanDescriptorSet::Release()
{
	vkFreeDescriptorSets(m_Device->GetLogicalDevice(), m_Owner->GetPool(), 1, &m_Handle);
	m_Layout->Release();
}

bool FVulkanDescriptorSet::BindUniformBuffer(uint32_t InBinding, const VkDescriptorBufferInfo * InBufferInfos, uint32_t InSize)
{
	m_Writes.resize(m_Layout->m_LayoutBindings.size());
	if (InBinding >= m_Writes.size()) return false;
	if (m_Layout->m_LayoutBindings[InBinding].descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) return false;

	m_Writes[InBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	m_Writes[InBinding].dstSet = m_Handle;
	m_Writes[InBinding].dstBinding = InBinding;
	m_Writes[InBinding].dstArrayElement = 0;
	m_Writes[InBinding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	m_Writes[InBinding].descriptorCount = InSize;
	m_Writes[InBinding].pImageInfo = nullptr;
	m_Writes[InBinding].pBufferInfo = InBufferInfos;
	m_Writes[InBinding].pTexelBufferView = nullptr;

}

bool FVulkanDescriptorSet::BindImageSampler(uint32_t InBinding, const VkDescriptorImageInfo * InImageInfos, uint32_t InSize)
{
	m_Writes.resize(m_Layout->m_LayoutBindings.size());
	if (InBinding >= m_Writes.size()) return false;
	if (m_Layout->m_LayoutBindings[InBinding].descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) return false;

	m_Writes[InBinding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	m_Writes[InBinding].dstSet = m_Handle;
	m_Writes[InBinding].dstBinding = InBinding;
	m_Writes[InBinding].dstArrayElement = 0;
	m_Writes[InBinding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	m_Writes[InBinding].descriptorCount = InSize;
	m_Writes[InBinding].pImageInfo = InImageInfos;
	m_Writes[InBinding].pBufferInfo = nullptr;
	m_Writes[InBinding].pTexelBufferView = nullptr;
}

void FVulkanDescriptorSet::UpdateBindings()
{
	vkUpdateDescriptorSets(m_Device->GetLogicalDevice(), static_cast<uint32_t>(m_Writes.size()), m_Writes.data(), 0, nullptr);
}

FVulkanDescriptorSetManager::FVulkanDescriptorSetManager(const FVulkanDevice * InDevice, uint32_t InMaxSets)
	:m_Device(InDevice), m_MaxSets(InMaxSets)
{
}

FVulkanDescriptorSetManager::~FVulkanDescriptorSetManager()
{
}

FVulkanDescriptorSet* FVulkanDescriptorSetManager::AddDescriptorSet(const FVulkanDescriptorSetLayout & InLayout)
{
	if (m_DescriptorSets.size() >= m_MaxSets) {
		return nullptr;
	}

	std::shared_ptr<FVulkanDescriptorSet> descriptorSet;
	descriptorSet.reset(new FVulkanDescriptorSet(m_Device, this, InLayout));
	m_DescriptorSets.push_back(descriptorSet);
	return descriptorSet.get();
}

void FVulkanDescriptorSetManager::Setup()
{
	uint32_t layoutTypesCount[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	memset(layoutTypesCount, 0, VK_DESCRIPTOR_TYPE_RANGE_SIZE * sizeof(uint32_t));
	for (auto &set : m_DescriptorSets)
	{
		for (uint32_t type = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; type <= VK_DESCRIPTOR_TYPE_END_RANGE; type++)
		{
			layoutTypesCount[type] += set->m_Layout->m_LayoutTypes[type];
		}
	}

	std::vector<VkDescriptorPoolSize> poolSizes;
	for (uint32_t type = VK_DESCRIPTOR_TYPE_BEGIN_RANGE; type <= VK_DESCRIPTOR_TYPE_END_RANGE; type++)
	{
		if (layoutTypesCount[type] > 0) {
			VkDescriptorPoolSize t_poolSize;
			t_poolSize.type = (VkDescriptorType)type;
			t_poolSize.descriptorCount = layoutTypesCount[type];
			poolSizes.push_back(t_poolSize);
		}
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = m_MaxSets;

	if (vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_Pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	for (auto &set : m_DescriptorSets)
	{
		set->Setup();
	}
}

void FVulkanDescriptorSetManager::Release()
{
	for (auto &set : m_DescriptorSets)
	{
		set->Release();
	}
	m_DescriptorSets.clear();

	vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_Pool, nullptr);
}

void FVulkanDescriptorSetManager::GetLayouts(std::vector<VkDescriptorSetLayout>& OutLayouts) const
{
	OutLayouts.clear();
	for (auto &set : m_DescriptorSets)
	{
		OutLayouts.push_back(set->m_Layout->GetHandle());
	}
}
