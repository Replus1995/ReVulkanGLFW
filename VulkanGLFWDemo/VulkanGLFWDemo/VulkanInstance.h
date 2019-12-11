#pragma once
#include <memory>
#include <vulkan/vulkan.h>

class FVulkanWindow;
class FVulkanDebugger;
typedef std::unique_ptr<FVulkanDebugger> FVulkanDebuggerPtr;

class FVulkanInstance
{
public:
	FVulkanInstance(const FVulkanWindow* Window);
	~FVulkanInstance();

	inline VkInstance GetHandle() const 
	{
		return m_Handle;
	}

	inline VkSurfaceKHR GetSurface() const
	{
		return m_Surface;
	}

	inline const FVulkanWindow* GetWindow() const
	{
		return m_Window;
	}

	inline const FVulkanDebuggerPtr& GetDebugger() const
	{
		return m_Debugger;
	}

	void Setup(const char * InApplicationName, const char * InEngineName);
	void Release();


private:
	void CreateInstance(const char * InApplicationName, const char * InEngineName);
	void CreateSurface();


private:
	const FVulkanWindow* m_Window;
	VkInstance m_Handle;
	VkSurfaceKHR m_Surface;

	bool m_InstanceCreated = false;

	FVulkanDebuggerPtr m_Debugger;
};