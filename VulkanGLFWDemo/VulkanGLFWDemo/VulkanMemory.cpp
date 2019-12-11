#include "VulkanMemory.h"
#include <iostream>
#include "VulkanDevice.h"

FVulkanSemaphore::FVulkanSemaphore(const FVulkanDevice * InDevice)
	:m_Device(InDevice),m_Handle(VK_NULL_HANDLE)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_Handle) != VK_SUCCESS) {
		throw std::runtime_error("[VulkanSemaphore] Failed to create!");
	}
}

FVulkanSemaphore::~FVulkanSemaphore()
{
	vkDestroySemaphore(m_Device->GetLogicalDevice(), m_Handle, nullptr);
}
