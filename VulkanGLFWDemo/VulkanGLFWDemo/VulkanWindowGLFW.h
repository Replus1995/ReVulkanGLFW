#pragma once
#include <vector>
#include <functional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanWindow.h"


class FVulkanWindowGLFW : public FVulkanWindow
{
public:
	FVulkanWindowGLFW() {};
	~FVulkanWindowGLFW() {};

	typedef std::function<void(int, int)> WindowResizeCallbackFunction;

	void CreateWindow(uint32_t InWidth = 1920, uint32_t InHeight = 1080, const char* InName = "Vulkan-Window", WindowResizeCallbackFunction InCallback = nullptr);
	void DestoryWindow();


	bool GetWindowSize(uint32_t& OutWidth, uint32_t& OutHeight) const;
	VkResult CreateVulkanSurface(const VkInstance& InInstance,VkSurfaceKHR& OutSurface) const;
	
	void GetInstanceExtensions(std::vector<const char*>& OutExtensions) const;

private:
	static void GLFWOnWindowResized(GLFWwindow* Window, int Width, int Height);

private:
	GLFWwindow* m_Window;
	uint32_t m_Width, m_Height;

	WindowResizeCallbackFunction m_WindowResizeCallback;


};
