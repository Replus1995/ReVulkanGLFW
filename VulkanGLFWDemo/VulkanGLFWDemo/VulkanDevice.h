#pragma once
#include <vulkan/vulkan.h>

class FVulkanInstance;

class FVulkanDevice
{
public:
	FVulkanDevice(const FVulkanInstance* InInstance);
	~FVulkanDevice();

	void Setup(const char * InApplicationName, const char * InEngineName);
	void Release();


public:
	inline const FVulkanInstance* GetInstance() const
	{
		return m_Instance;
	}
	inline const VkPhysicalDevice& GetPhysicalDevice() const
	{
		return m_PhysicalDevice;
	}
	inline const VkDevice& GetLogicalDevice() const
	{
		return m_LogicalDevice;
	}
	


private:
	void PickPhysicalDevice();
	void CreateLogicalDevice();

private:
	const FVulkanInstance* m_Instance;

	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_LogicalDevice;

	bool m_DeviceCreated = false;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	
};