#pragma once

#include <set>
#include <array>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

class FVulkanUtil
{
public:
	struct LayerProperties {
		VkLayerProperties Properties;
		std::vector<VkExtensionProperties> InstanceExtensions;
		std::vector<VkExtensionProperties> DeviceExtensions;
	};

	struct QueueFamilyIndices {
		int GraphicsIndex;
		int PresentIndex;
		int ComputeIndex;
		int TransferIndex;

		QueueFamilyIndices(int InGraphicsIndex = -1, int InPresentIndex = -1, int InComputeIndex = -1, int InTransferIndex = -1)
			:GraphicsIndex(InGraphicsIndex), PresentIndex(InPresentIndex), ComputeIndex(InComputeIndex), TransferIndex(InTransferIndex)
		{};
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;
	};

	static VkResult QueryInstanceExtensionProperties(LayerProperties& OutLayerProperties);
	static VkResult QueryLayerProperties(std::vector<LayerProperties>& OutPropertiesArray);

	static QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& InDevice, const VkSurfaceKHR& InSurface);
	static SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& InDevice, const VkSurfaceKHR& InSurface);
	static bool CheckDeviceExtensionSupport(const VkPhysicalDevice& InDevice, const std::vector<const char*>& InDeviceExtensions);
	static bool CheckDeviceSuitable(const VkPhysicalDevice& InDevice, const VkSurfaceKHR& InSurface, const std::vector<const char*>& InDeviceExtensions);

	static void GetInstanceExtensions(std::vector<const char*>& OutExtensions);
	static void GetDeviceExtensions(std::vector<const char*>& OutExtensions);


	struct Vertex {
		glm::vec2 Position;
		glm::vec3 Color;
		glm::vec2 TexCoord;

		static VkVertexInputBindingDescription GetBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, Position);
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, Color);
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, TexCoord);

			return attributeDescriptions;
		}
	};

	static uint32_t FindMemoryType(const VkPhysicalDevice& InDevice, uint32_t InTypeFilter, VkMemoryPropertyFlags InProperties);

};



