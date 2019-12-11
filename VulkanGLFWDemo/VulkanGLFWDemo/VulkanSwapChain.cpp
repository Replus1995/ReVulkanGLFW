#include "VulkanSwapChain.h"
#include <algorithm>
#include "VulkanUtil.h"
#include "VulkanWindow.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"

FVulkanSwapChain::FVulkanSwapChain(const FVulkanDevice* InDevice)
	:m_Device(InDevice)
{

}

FVulkanSwapChain::~FVulkanSwapChain()
{
}

void FVulkanSwapChain::Setup()
{
	CreateSwapChain();
	CreateSwapChainImageViews();
}

void FVulkanSwapChain::Release()
{
	for (size_t i = 0; i < m_Framebuffers.size(); i++) {
		vkDestroyFramebuffer(m_Device->GetLogicalDevice(), m_Framebuffers[i], nullptr);
	}
	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
		vkDestroyImageView(m_Device->GetLogicalDevice(), m_SwapChainImageViews[i], nullptr);
	}
	vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_Handle, nullptr);
}

void FVulkanSwapChain::CreateFramebuffers(const FVulkanRenderPass * InRenderPass)
{
	m_Framebuffers.resize(m_SwapChainImageViews.size());

	for (size_t i = 0; i < m_Framebuffers.size(); i++) {
		VkImageView attachments[] = {
			m_SwapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = InRenderPass->GetHandle();
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_SwapChainExtent.width;
		framebufferInfo.height = m_SwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_Device->GetLogicalDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void FVulkanSwapChain::CreateSwapChain()
{
	FVulkanUtil::SwapChainSupportDetails swapChainSupport = FVulkanUtil::QuerySwapChainSupport(m_Device->GetPhysicalDevice(), m_Device->GetInstance()->GetSurface());

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);


	uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount) {
		imageCount = swapChainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Device->GetInstance()->GetSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	FVulkanUtil::QueueFamilyIndices indices = FVulkanUtil::FindQueueFamilies(m_Device->GetPhysicalDevice(), m_Device->GetInstance()->GetSurface());
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.GraphicsIndex, (uint32_t)indices.PresentIndex };

	if (indices.GraphicsIndex != indices.PresentIndex) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_Device->GetLogicalDevice(), &createInfo, nullptr, &m_Handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Handle, &imageCount, nullptr);
	m_SwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_Handle, &imageCount, m_SwapChainImages.data());

	m_SwapChainImageFormat = surfaceFormat.format;
	m_SwapChainExtent = extent;
}

void FVulkanSwapChain::CreateSwapChainImageViews()
{
	m_SwapChainImageViews.resize(m_SwapChainImages.size());

	for (size_t i = 0; i < m_SwapChainImages.size(); i++) {
		m_SwapChainImageViews[i] = CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat);

	}
}

VkSurfaceFormatKHR FVulkanSwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& InAvailableFormats)
{
	if (InAvailableFormats.size() == 1 && InAvailableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : InAvailableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return InAvailableFormats[0];
}

VkPresentModeKHR FVulkanSwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> InAvailablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : InAvailablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
			break;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D FVulkanSwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & InCapabilities)
{
	if (InCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return InCapabilities.currentExtent;
	}
	else {
		uint32_t width, height;
		m_Device->GetInstance()->GetWindow()->GetWindowSize(width, height);
		
		VkExtent2D actualExtent = { width, height };

		actualExtent.width = std::max(InCapabilities.minImageExtent.width, std::min(InCapabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(InCapabilities.minImageExtent.height, std::min(InCapabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkImageView FVulkanSwapChain::CreateImageView(VkImage& image, VkFormat& format)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(m_Device->GetLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}
