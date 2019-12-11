#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanRenderPass;

class FVulkanSwapChain
{
public:
	FVulkanSwapChain(const FVulkanDevice* InDevice);
	~FVulkanSwapChain();

	void Setup();
	void Release();

	void CreateFramebuffers(const FVulkanRenderPass* InRenderPass);

	inline VkFormat GetImageFormat() const
	{
		return m_SwapChainImageFormat;
	}

	inline VkExtent2D GetExtent() const
	{
		return m_SwapChainExtent;
	}

	/*inline uint32_t GetImageCount() const
	{
		return m_SwapChainImages.size();
	}*/

	inline const std::vector<VkImageView>& GetImageViews() const
	{
		return m_SwapChainImageViews;
	}

	inline const FVulkanDevice* GetDevice() const
	{
		return m_Device;
	}

	inline VkSwapchainKHR GetHandle() const
	{
		return m_Handle;
	}

	

private:
	void CreateSwapChain();
	void CreateSwapChainImageViews();
	

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& InAvailableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> InAvailablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& InCapabilities);
	VkImageView CreateImageView(VkImage& image, VkFormat& format);


private:
	const FVulkanDevice* m_Device;

	VkSwapchainKHR m_Handle;

	VkExtent2D m_SwapChainExtent;
	VkFormat m_SwapChainImageFormat;
	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;
	std::vector<VkFramebuffer> m_Framebuffers;
	
};
