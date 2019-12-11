#include "VulkanDebugger.h"

bool FVulkanDebugger::CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_ValidationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void FVulkanDebugger::SetupDebugCallback(const VkInstance & instance)
{

	VkDebugReportCallbackCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = DebugCallback;

	if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &m_DebugCallback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}

}

void FVulkanDebugger::DestoryDebugCallback(const VkInstance & instance)
{
	DestroyDebugReportCallbackEXT(instance, m_DebugCallback, nullptr);
}
