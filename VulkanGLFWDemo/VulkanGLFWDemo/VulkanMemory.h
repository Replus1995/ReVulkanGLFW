#pragma once

#include <vulkan/vulkan.h>

class FVulkanDevice;

class FVulkanSemaphore
{
public:
	FVulkanSemaphore(const FVulkanDevice* InDevice);
	~FVulkanSemaphore();

	inline VkSemaphore GetHandle() const
	{
		return m_Handle;
	}

private:
	const FVulkanDevice* m_Device;
	VkSemaphore m_Handle;
};
