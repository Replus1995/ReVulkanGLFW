#pragma once

#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>

class FVulkanDebugger
{
public:
	FVulkanDebugger() {};
	~FVulkanDebugger() {};

	bool CheckValidationLayerSupport();
	void SetupDebugCallback(const VkInstance& instance);
	void DestoryDebugCallback(const VkInstance& instance);

	inline const std::vector<const char*>& GetValidationLayers() {
		return m_ValidationLayers;
	}

private:


	const std::vector<const char*> m_ValidationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	VkDebugReportCallbackEXT m_DebugCallback;

	VkResult CreateDebugReportCallbackEXT(const VkInstance& instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugReportCallbackEXT(const VkInstance& instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData) {

		std::cerr << "validation layer: " << msg << std::endl;

		return VK_FALSE;
	}
};


