#include "VulkanWindowGLFW.h"


void FVulkanWindowGLFW::CreateWindow(uint32_t InWidth, uint32_t InHeight, const char * InName, WindowResizeCallbackFunction InCallback)
{
	m_Width = InWidth;
	m_Height = InHeight;
	m_WindowResizeCallback = InCallback;

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_Window = glfwCreateWindow(m_Width, m_Height, InName, nullptr, nullptr);
	glfwSetWindowUserPointer(m_Window, this);
	glfwSetWindowSizeCallback(m_Window, GLFWOnWindowResized);
}

void FVulkanWindowGLFW::DestoryWindow()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}


bool FVulkanWindowGLFW::GetWindowSize(uint32_t & OutWidth, uint32_t & OutHeight) const
{
	int width, height;
	glfwGetWindowSize(m_Window, &width, &height);
	if (width > 0 && height > 0) {
		OutWidth = width;
		OutHeight = height;
		return true;
	}
	return false;
}

VkResult FVulkanWindowGLFW::CreateVulkanSurface(const VkInstance & InInstance, VkSurfaceKHR & OutSurface) const
{
	return glfwCreateWindowSurface(InInstance, m_Window, nullptr, &OutSurface);
}

void FVulkanWindowGLFW::GetInstanceExtensions(std::vector<const char*>& OutExtensions) const
{
	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++) {
		OutExtensions.push_back(glfwExtensions[i]);
	}

}
 

void FVulkanWindowGLFW::GLFWOnWindowResized(GLFWwindow * Window, int Width, int Height)
{
	FVulkanWindowGLFW* host = reinterpret_cast<FVulkanWindowGLFW*>(glfwGetWindowUserPointer(Window));
	host->m_Width = Width;
	host->m_Height = Height;
	if (host->m_WindowResizeCallback) {
		host->m_WindowResizeCallback(host->m_Width, host->m_Height);
	}
}
