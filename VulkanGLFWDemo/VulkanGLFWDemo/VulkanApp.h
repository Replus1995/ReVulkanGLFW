#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <mutex>
#include <thread>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#pragma comment ( lib, "glfw3.lib")
#pragma comment ( lib, "vulkan-1.lib")

class VulkanGLFWApp
{
public:
	VulkanGLFWApp(int nWidth, int nHeight) :
		m_nWidth(nWidth), m_nHeight(nHeight)
	{
		bReady = false;
		pthMsgLoop = new std::thread(ThreadProc, this);
		while (!bReady) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	~VulkanGLFWApp() {
		pthMsgLoop->join();
		delete pthMsgLoop;
	}

	bool GetIsReady() {
		return bReady;
	}

private:
	static void ThreadProc(VulkanGLFWApp *This) {
		This->Run();
	}

	void Run() {
		initWindow();
		initVulkan();
		bReady = true;
		mainLoop();
		cleanup();
	}

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_Window = glfwCreateWindow(m_nWidth, m_nHeight, "FramePresenterVulkan", nullptr, nullptr);

		glfwSetWindowUserPointer(m_Window, this);
		glfwSetWindowSizeCallback(m_Window, VulkanGLFWApp::onWindowResized);
	}

	void initVulkan() {
		
		// create a UniqueInstance
		createInstance("Vulkan GLFW", "Vulkan");
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		setupDebugCallback();
		

		createSwapChain();
		createCommandPool();
		createCommandBuffers();
		createImageViews();

		createDescriptorSetLayout();
		createDescriptorPool();

		createRenderPass();
		createFramebuffers();

		createGraphicsPipeline();
	

		

		createTextureImage();
		createTextureImageView();
		createTextureSampler();

		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffer();

		
		
		createDescriptorSet();

		bindCommandBuffers();
		createSemaphores();

	}

	void mainLoop() {

		updateUniformBuffer();

		while (!glfwWindowShouldClose(m_Window)) {
			glfwPollEvents();

			
			drawFrame();
		}
		vkDeviceWaitIdle(m_Device);
	}

	void cleanup() {

		cleanupSwapChain();

		vkDestroySampler(m_Device, m_TextureSampler, nullptr);
		vkDestroyImageView(m_Device, m_TextureImageView, nullptr);

		vkDestroyImage(m_Device, m_TextureImage, nullptr);
		vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);

		vkDestroyBuffer(m_Device, m_TexStagingBuffer, nullptr);
		vkFreeMemory(m_Device, m_TexStagingBufferMemory, nullptr);


		vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
		vkDestroyBuffer(m_Device, m_UniformBuffer, nullptr);
		vkFreeMemory(m_Device, m_UniformBufferMemory, nullptr);

		vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);
		vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);

		vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
		vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);

		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		
		DestroyDebugReportCallbackEXT(m_Instance, debug_callback, nullptr);
		vkDestroyDevice(m_Device, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		glfwDestroyWindow(m_Window);

		glfwTerminate();
	}

private:
	struct QueueFamilyIndices {
		int GraphicsFamilyIndex;
		int PresentFamilyIndex;

		QueueFamilyIndices(int gindex, int pindex) :GraphicsFamilyIndex(gindex), PresentFamilyIndex(pindex) {};
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};


private:

	void createInstance(const char* pApplicationName, const char* pEngineName);
	void setupDebugCallback();
	void pickPhysicalDevice();

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device);
	void createLogicalDevice();

	void createSurface() {
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<char>& code);
	
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();

	void bindCommandBuffers();

	void createSemaphores();
	void drawFrame();

	void cleanupSwapChain();
	void recreateSwapChain();

	static void onWindowResized(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;

		VulkanGLFWApp* app = reinterpret_cast<VulkanGLFWApp*>(glfwGetWindowUserPointer(window));
		app->recreateSwapChain();
	}

	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffer();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void updateUniformBuffer();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSet();

	void createTextureImage();

	void createImage(uint32_t width, uint32_t height, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImage& image, VkDeviceMemory& imageMemory);

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createTextureImageView();
	VkImageView createImageView(VkImage image, VkFormat format);
	void createTextureSampler();


private:

	int m_nWidth, m_nHeight;

	bool bReady = false;
	std::mutex mtx;

	std::thread *pthMsgLoop = NULL;

	GLFWwindow* m_Window;

	VkInstance m_Instance;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device;
	
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	VkSurfaceKHR m_Surface;

	const std::vector<const char*> m_DeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkSwapchainKHR m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	VkFormat m_SwapChainImageFormat;
	VkExtent2D m_SwapChainExtent;

	std::vector<VkImageView> m_SwapChainImageViews;

	VkRenderPass m_RenderPass;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;

	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	VkSemaphore m_ImageAvailableSemaphore;
	VkSemaphore m_RenderFinishedSemaphore;


	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

			return attributeDescriptions;
		}
	};

	const std::vector<Vertex> m_Vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};
	const std::vector<uint16_t> m_Indices = {
		0, 1, 2, 2, 3, 0
	};


	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_VertexBufferMemory;
	VkBuffer m_IndexBuffer;
	VkDeviceMemory m_IndexBufferMemory;
	VkBuffer m_UniformBuffer;
	VkDeviceMemory m_UniformBufferMemory;

	VkDescriptorPool m_DescriptorPool;
	VkDescriptorSet m_DescriptorSet;

	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
	VkImageView m_TextureImageView;
	VkSampler m_TextureSampler;


	VkBuffer m_TexStagingBuffer;
	VkDeviceMemory m_TexStagingBufferMemory;





	const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	VkDebugReportCallbackEXT debug_callback;

private:

	bool checkValidationLayerSupport();

	std::vector<const char*> getRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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

	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) {
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pCallback);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}
};
