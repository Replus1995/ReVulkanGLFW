#include "VulkanTexture.h"
#include "VulkanUtil.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"

FVulkanTexture::FVulkanTexture(const FVulkanDevice * InDevice)
	:m_Device(InDevice), m_CurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED)
{
}

FVulkanTexture::~FVulkanTexture()
{
}

void FVulkanTexture::CreateTexture(VkImageType imagetype, VkFormat format, VkExtent3D extent,
	uint32_t miplevels, uint32_t arraylayers, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
	VkImage& image, VkDeviceMemory& memory)
{

	/*m_Format = format;
	m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_LevelsCount = miplevels;
	m_LayersCount = arraylayers;*/

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = imagetype;
	imageInfo.extent = extent;
	imageInfo.mipLevels = miplevels;
	imageInfo.arrayLayers = arraylayers;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = 0;

	if (vkCreateImage(m_Device->GetLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Device->GetLogicalDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FVulkanUtil::FindMemoryType(m_Device->GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_Device->GetLogicalDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(m_Device->GetLogicalDevice(), image, memory, 0);
}

void FVulkanTexture::CreateImageView(VkImage image, VkImageViewType viewtype, VkFormat format, uint32_t levels, uint32_t layers, VkImageView& view)
{

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = viewtype;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = levels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layers;

	if (vkCreateImageView(m_Device->GetLogicalDevice(), &viewInfo, nullptr, &view) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}

void FVulkanTexture::CreateTextureSampler(VkSampler& sampler)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(m_Device->GetLogicalDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

void FVulkanTexture::TransitionImageLayout(FVulkanCommandBuffer * InCmdBuffer, VkImage image, uint32_t levels, uint32_t layers, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	InCmdBuffer->Begin();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = levels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = layers;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}


	vkCmdPipelineBarrier(
		InCmdBuffer->GetHandle(),
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	InCmdBuffer->End();
	InCmdBuffer->GetOwner()->GetQueue()->Submit(InCmdBuffer);

	m_CurrentLayout = newLayout;
}

void FVulkanTexture::CopyBufferToImage(FVulkanCommandBuffer * InCmdBuffer, VkBuffer buffer, VkImage image, VkExtent3D extent, uint32_t layers)
{
	InCmdBuffer->Begin();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layers;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = extent;

	vkCmdCopyBufferToImage(
		InCmdBuffer->GetHandle,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	InCmdBuffer->End();
	InCmdBuffer->GetOwner()->GetQueue()->Submit(InCmdBuffer);
}

void FVulkanTexture::DestoryTexture()
{
	vkDestroySampler(m_Device->GetLogicalDevice(), m_TextureSampler, nullptr);
	vkDestroyImageView(m_Device->GetLogicalDevice(), m_TextureImageView, nullptr);

	vkDestroyImage(m_Device->GetLogicalDevice(), m_TextureImage, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), m_TextureImageMemory, nullptr);
}

FVulkanTexture2D::FVulkanTexture2D(const FVulkanDevice * InDevice, uint32_t InWidth, uint32_t InHeight, VkFormat InFormat)
	:FVulkanTexture(InDevice),m_Width(InWidth),m_Height(InHeight),m_Format(InFormat)
{
	CreateTexture(VK_IMAGE_TYPE_2D, m_Format, VkExtent3D{ m_Width,m_Height,1 },
		0, 0, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_TextureImage, m_TextureImageMemory);
	CreateImageView(m_TextureImage, VK_IMAGE_VIEW_TYPE_2D, m_Format, 0, 0, m_TextureImageView);
	CreateTextureSampler(m_TextureSampler);
}

FVulkanTexture2D::~FVulkanTexture2D()
{
	DestoryTexture();
}
