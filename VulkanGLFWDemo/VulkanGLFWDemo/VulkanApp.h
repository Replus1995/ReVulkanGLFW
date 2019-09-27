#pragma once

#include <iostream>
#include <vector>
#include <set>
#include <thread>
#include <cstring>
#include <algorithm>
#include <fstream>

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
		bQuit = true;
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
		createInstance("Vulkan GLFW", "Vulkan HPP");
		setupDebugCallback();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createSemaphores();

	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_Window)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(m_Device);
	}

	void cleanup() {

		cleanupSwapChain();

		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		
		vkDestroyDevice(m_Device, nullptr);
		DestroyDebugReportCallbackEXT(m_Instance, debug_callback, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		glfwDestroyWindow(m_Window);

		glfwTerminate();
	}

private:

	void createInstance(const char* pApplicationName, const char* pEngineName);
	void setupDebugCallback();
	void pickPhysicalDevice();

	struct QueueFamilyIndices {
		int GraphicsFamilyIndex;
		int PresentFamilyIndex;

		QueueFamilyIndices(int gindex, int pindex) :GraphicsFamilyIndex(gindex), PresentFamilyIndex(pindex) {};
	};

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool isDeviceSuitable(VkPhysicalDevice device);
	void createLogicalDevice();

	void createSurface() {
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

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

	void createSemaphores() {
		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphores!");
		}
	}

	void drawFrame() {

		vkQueueWaitIdle(m_PresentQueue);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(m_Device, m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];
		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		//vkQueueWaitIdle(m_PresentQueue);
	}

	void cleanupSwapChain() {
		for (size_t i = 0; i < m_SwapChainFramebuffers.size(); i++) {
			vkDestroyFramebuffer(m_Device, m_SwapChainFramebuffers[i], nullptr);
		}
		vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

		vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
		for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
			vkDestroyImageView(m_Device, m_SwapChainImageViews[i], nullptr);
		}
		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	}


	void recreateSwapChain() {
		vkDeviceWaitIdle(m_Device);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	static void onWindowResized(GLFWwindow* window, int width, int height) {
		if (width == 0 || height == 0) return;

		VulkanGLFWApp* app = reinterpret_cast<VulkanGLFWApp*>(glfwGetWindowUserPointer(window));
		app->recreateSwapChain();
	}


private:

	int m_nWidth, m_nHeight;

	bool bReady = false;
	bool bQuit = false;
	//std::mutex mtx;
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
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;

	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	VkSemaphore m_ImageAvailableSemaphore;
	VkSemaphore m_RenderFinishedSemaphore;







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
