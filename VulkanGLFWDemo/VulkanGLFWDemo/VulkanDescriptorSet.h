#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

class FVulkanDevice;

class FVulkanDescriptorSetLayout
{
public:
	FVulkanDescriptorSetLayout();
	~FVulkanDescriptorSetLayout();

	bool AddDescriptor(const VkDescriptorSetLayoutBinding& InDescriptor);
	uint32_t GetNextBinding();

protected:
	friend class FVulkanDescriptorSet;
	friend class FVulkanDescriptorSetManager;

	FVulkanDescriptorSetLayout(const FVulkanDevice* InDevice, const FVulkanDescriptorSetLayout& InLayout);

	inline VkDescriptorSetLayout GetHandle()
	{
		return m_Handle;
	}

	void Compile();
	void Release();

private:
	bool CheckUniqueBind(uint32_t InIndex);

private:
	const FVulkanDevice* m_Device;

	uint32_t m_LayoutTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	std::vector<VkDescriptorSetLayoutBinding> m_LayoutBindings;

	VkDescriptorSetLayout m_Handle = 0;
	uint32_t m_HandleId = 0;

	
};


class FVulkanDescriptorSet
{
public:
	FVulkanDescriptorSet();
	~FVulkanDescriptorSet();

	bool BindUniformBuffer(uint32_t InBinding, const VkDescriptorBufferInfo* InBufferInfos, uint32_t InSize);
	bool BindImageSampler(uint32_t InBinding, const VkDescriptorImageInfo* InImageInfos, uint32_t InSize);

	void UpdateBindings();
	

protected:
	friend class FVulkanDescriptorSetManager;
	FVulkanDescriptorSet(const FVulkanDevice* InDevice, const FVulkanDescriptorSetManager* InOwner, const FVulkanDescriptorSetLayout& InLayout);
	
	void Setup();
	void Release();

private:
	const FVulkanDevice* m_Device;
	const FVulkanDescriptorSetManager* m_Owner;

	VkDescriptorSet m_Handle;
	std::unique_ptr<FVulkanDescriptorSetLayout> m_Layout;
	std::vector<VkWriteDescriptorSet> m_Writes;

	
};

class FVulkanDescriptorSetManager
{
public:
	FVulkanDescriptorSetManager(const FVulkanDevice* InDevice, uint32_t InMaxSets);
	~FVulkanDescriptorSetManager();


	FVulkanDescriptorSet* AddDescriptorSet(const FVulkanDescriptorSetLayout& InLayout);

	void Setup();
	void Release();

	void GetLayouts(std::vector<VkDescriptorSetLayout>& OutLayouts) const;

protected:
	friend class FVulkanDescriptorSet;

	inline VkDescriptorPool GetPool() const
	{
		return m_Pool;
	}

private:
	const FVulkanDevice* m_Device;
	uint32_t m_MaxSets;

	VkDescriptorPool m_Pool;
	std::vector<std::shared_ptr<FVulkanDescriptorSet>> m_DescriptorSets;

	
};