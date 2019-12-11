#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanRenderTargetInfo;

class FVulkanRenderPass
{
public:
	FVulkanRenderPass(const FVulkanDevice* InDevice);
	~FVulkanRenderPass();

	void Setup(const FVulkanRenderTargetInfo* InTargetInfo);
	void Release();

	inline VkRenderPass GetHandle() const
	{
		return m_Handle;
	}

private:

	void CreateRenderPass(const FVulkanRenderTargetInfo* InTargetInfo);

private:
	const FVulkanDevice* m_Device;

	VkRenderPass m_Handle;
	
};