#pragma once
#include <vulkan/vulkan.h>

class FVulkanWindow
{
public:
	FVulkanWindow() {};
	virtual ~FVulkanWindow() {};

	virtual bool GetWindowSize(uint32_t& OutWidth, uint32_t& OutHeight) const = 0;
	virtual VkResult CreateVulkanSurface(const VkInstance& InInstance, VkSurfaceKHR& OutSurface) const = 0;
};
