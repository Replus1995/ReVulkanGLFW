#include "VulkanUtil.h"

#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

VkResult FVulkanUtil::QueryInstanceExtensionProperties(LayerProperties& OutLayerProperties)
{
	VkExtensionProperties *instance_extensions;
	VkExtensionProperties *device_extensions;
	uint32_t instance_extension_count;
	VkResult res;
	char *layer_name = NULL;

	layer_name = OutLayerProperties.Properties.layerName;

	do {
		res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, NULL);
		if (res) return res;

		if (instance_extension_count == 0) {
			return VK_SUCCESS;
		}

		OutLayerProperties.InstanceExtensions.resize(instance_extension_count);
		instance_extensions = OutLayerProperties.InstanceExtensions.data();
		res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, instance_extensions);
	} while (res == VK_INCOMPLETE);

	return res;
}

VkResult FVulkanUtil::QueryLayerProperties(std::vector<LayerProperties>& OutPropertiesArray)
{
	uint32_t instance_layer_count;
	VkLayerProperties *vk_props = NULL;
	VkResult res;

	do {
		res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
		if (res) return res;

		if (instance_layer_count == 0) {
			return VK_SUCCESS;
		}

		vk_props = (VkLayerProperties *)realloc(vk_props, instance_layer_count * sizeof(VkLayerProperties));

		res = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
	} while (res == VK_INCOMPLETE);

	/*
	 * Now gather the extension list for each instance layer.
	 */
	for (uint32_t i = 0; i < instance_layer_count; i++) {
		LayerProperties layer_props;
		layer_props.Properties = vk_props[i];
		res = QueryInstanceExtensionProperties(layer_props);
		if (res) return res;
		OutPropertiesArray.push_back(layer_props);
	}
	free(vk_props);

	return res;
}

FVulkanUtil::QueueFamilyIndices FVulkanUtil::FindQueueFamilies(const VkPhysicalDevice & InDevice, const VkSurfaceKHR & InSurface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(InDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(InDevice, &queueFamilyCount, queueFamilies.data());

	// look for an family index that supports both graphics and present
	for (size_t i = 0; i < queueFamilyCount; i++) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(InDevice, i, InSurface, &presentSupport);

		if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && presentSupport) {
			/*m_GraphicsFamilyIndex = static_cast<uint32_t>(i);
			m_PresentFamilyIndex = static_cast<uint32_t>(i);*/
			return QueueFamilyIndices(i, i);
		}
	}

	QueueFamilyIndices indices;

	// there's nothing like a single family index that supports both graphics and present -> look for an other family index that supports present

	for (size_t i = 0; i < queueFamilyCount; i++) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(InDevice, i, InSurface, &presentSupport);

		if (queueFamilies[i].queueCount > 0 && presentSupport) {
			indices.PresentIndex = i;
		}


		if (queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.GraphicsIndex = i;
		}

		if (indices.PresentIndex >= 0 && indices.GraphicsIndex >= 0) {
			/*m_GraphicsFamilyIndex = static_cast<uint32_t>(t_g_index);
			m_PresentFamilyIndex = static_cast<uint32_t>(t_p_index);*/
			return indices;
		}
	}

	return QueueFamilyIndices();
}

FVulkanUtil::SwapChainSupportDetails FVulkanUtil::QuerySwapChainSupport(const VkPhysicalDevice & InDevice, const VkSurfaceKHR& InSurface)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(InDevice, InSurface, &details.Capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(InDevice, InSurface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(InDevice, InSurface, &formatCount, details.Formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(InDevice, InSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(InDevice, InSurface, &presentModeCount, details.PresentModes.data());
	}


	return details;
}

bool FVulkanUtil::CheckDeviceExtensionSupport(const VkPhysicalDevice & InDevice, const std::vector<const char*>& InDeviceExtensions)
{
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(InDevice, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extension_count);
	vkEnumerateDeviceExtensionProperties(InDevice, nullptr, &extension_count, availableExtensions.data());

	std::set<std::string> requiredExtensions(InDeviceExtensions.begin(), InDeviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

bool FVulkanUtil::CheckDeviceSuitable(const VkPhysicalDevice & InDevice, const VkSurfaceKHR& InSurface, const std::vector<const char*>& InDeviceExtensions)
{
	QueueFamilyIndices indices = FindQueueFamilies(InDevice, InSurface);
	if (indices.GraphicsIndex < 0 || indices.PresentIndex < 0) {
		return false;
	}

	if (!CheckDeviceExtensionSupport(InDevice, InDeviceExtensions))
		return false;

	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(InDevice, InSurface);
	if (swapChainSupport.Formats.empty() && swapChainSupport.PresentModes.empty())
		return false;

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(InDevice, &supportedFeatures);
	if (!supportedFeatures.samplerAnisotropy) {
		return false;
	}

	return true;
}

void FVulkanUtil::GetInstanceExtensions(std::vector<const char*>& OutExtensions)
{
	OutExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef __ANDROID__
	out_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_WIN32)
	OutExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
	out_extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	out_extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#else
	out_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
}

void FVulkanUtil::GetDeviceExtensions(std::vector<const char*>& OutExtensions)
{
	OutExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

uint32_t FVulkanUtil::FindMemoryType(const VkPhysicalDevice & InDevice, uint32_t InTypeFilter, VkMemoryPropertyFlags InProperties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(InDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((InTypeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & InProperties) == InProperties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}
