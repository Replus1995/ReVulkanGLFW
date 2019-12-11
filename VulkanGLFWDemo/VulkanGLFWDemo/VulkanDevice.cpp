#include "VulkanDevice.h"
#include <iostream>
#include <vector>
#include "VulkanUtil.h"
#include "VulkanDebugger.h"
#include "VulkanInstance.h"

FVulkanDevice::FVulkanDevice(const FVulkanInstance* Window)
	:m_Instance(Window)
{
}

FVulkanDevice::~FVulkanDevice()
{
}

void FVulkanDevice::Setup(const char * InApplicationName, const char * InEngineName)
{
	if (m_DeviceCreated) return;

	PickPhysicalDevice();
	CreateLogicalDevice();

	m_DeviceCreated = true;
}

void FVulkanDevice::Release()
{
	if (!m_DeviceCreated) return;

	vkDestroyDevice(m_LogicalDevice, nullptr);

	m_DeviceCreated = false;

}


void FVulkanDevice::PickPhysicalDevice()
{
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(m_Instance->GetHandle(), &physical_device_count, nullptr);
	if (physical_device_count == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
	vkEnumeratePhysicalDevices(m_Instance->GetHandle(), &physical_device_count, physical_devices.data());

	std::vector<const char*> extensions;
	FVulkanUtil::GetDeviceExtensions(extensions);

	for (const auto& physical_device : physical_devices) {
		if (FVulkanUtil::CheckDeviceSuitable(physical_device, m_Instance->GetSurface(), extensions)) {
			m_PhysicalDevice = physical_device;
			break;
		}
	}

	if (m_PhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void FVulkanDevice::CreateLogicalDevice()
{
	FVulkanUtil::QueueFamilyIndices indices = FVulkanUtil::FindQueueFamilies(m_PhysicalDevice, m_Instance->GetSurface());

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.GraphicsIndex, indices.PresentIndex };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}


	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;

	std::vector<const char*> extensions;
	FVulkanUtil::GetDeviceExtensions(extensions);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (m_Instance->GetDebugger()) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_Instance->GetDebugger()->GetValidationLayers().size());
		createInfo.ppEnabledLayerNames = m_Instance->GetDebugger()->GetValidationLayers().data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}


	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	
}






