#include "VulkanTexture.h"
#include "VulkanUtil.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"

FVulkanTexture::FVulkanTexture(const FVulkanDevice * InDevice)
	:m_Device(InDevice)
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
	/*InCmdBuffer->Begin();*/
	if (!InCmdBuffer->HasBegun()) return;

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

	/*InCmdBuffer->End();
	InCmdBuffer->GetOwner()->GetQueue()->Submit(InCmdBuffer);*/
}

void FVulkanTexture::CopyBufferToImage(FVulkanCommandBuffer * InCmdBuffer, VkBuffer buffer, VkImage image, VkExtent3D extent, uint32_t layers)
{
	/*InCmdBuffer->Begin();*/
	if (!InCmdBuffer->HasBegun()) return;

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

	/*InCmdBuffer->End();
	InCmdBuffer->GetOwner()->GetQueue()->Submit(InCmdBuffer);*/
}

uint32_t FVulkanTexture::GetBppFromFormat(VkFormat InFormat)
{

	switch (InFormat)
	{
	case VK_FORMAT_UNDEFINED:
		return 0;
		break;
	/*case VK_FORMAT_R4G4_UNORM_PACK8:
		break;
	case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
		break;
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		break;
	case VK_FORMAT_R5G6B5_UNORM_PACK16:
		break;
	case VK_FORMAT_B5G6R5_UNORM_PACK16:
		break;
	case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
		break;
	case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
		break;
	case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		break;*/
	case VK_FORMAT_R8_UNORM:
		return 1;
		break;
	case VK_FORMAT_R8_SNORM:
		return 1;
		break;
	/*case VK_FORMAT_R8_USCALED:
		break;
	case VK_FORMAT_R8_SSCALED:
		break;
	case VK_FORMAT_R8_UINT:
		break;
	case VK_FORMAT_R8_SINT:
		break;
	case VK_FORMAT_R8_SRGB:
		break;
	case VK_FORMAT_R8G8_UNORM:
		break;
	case VK_FORMAT_R8G8_SNORM:
		break;
	case VK_FORMAT_R8G8_USCALED:
		break;
	case VK_FORMAT_R8G8_SSCALED:
		break;
	case VK_FORMAT_R8G8_UINT:
		break;
	case VK_FORMAT_R8G8_SINT:
		break;
	case VK_FORMAT_R8G8_SRGB:
		break;
	case VK_FORMAT_R8G8B8_UNORM:
		break;
	case VK_FORMAT_R8G8B8_SNORM:
		break;
	case VK_FORMAT_R8G8B8_USCALED:
		break;
	case VK_FORMAT_R8G8B8_SSCALED:
		break;
	case VK_FORMAT_R8G8B8_UINT:
		break;
	case VK_FORMAT_R8G8B8_SINT:
		break;
	case VK_FORMAT_R8G8B8_SRGB:
		break;
	case VK_FORMAT_B8G8R8_UNORM:
		break;
	case VK_FORMAT_B8G8R8_SNORM:
		break;
	case VK_FORMAT_B8G8R8_USCALED:
		break;
	case VK_FORMAT_B8G8R8_SSCALED:
		break;
	case VK_FORMAT_B8G8R8_UINT:
		break;
	case VK_FORMAT_B8G8R8_SINT:
		break;
	case VK_FORMAT_B8G8R8_SRGB:
		break;*/
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4;
		break;
	case VK_FORMAT_R8G8B8A8_SNORM:
		return 4;
		break;
	case VK_FORMAT_R8G8B8A8_USCALED:
		return 4;
		break;
	case VK_FORMAT_R8G8B8A8_SSCALED:
		return 4;
		break;
	case VK_FORMAT_R8G8B8A8_UINT:
		return 4;
		break;
	case VK_FORMAT_R8G8B8A8_SINT:
		return 4;
		break;
	case VK_FORMAT_R8G8B8A8_SRGB:
		return 4;
		break;
	case VK_FORMAT_B8G8R8A8_UNORM:
		return 4;
		break;
	case VK_FORMAT_B8G8R8A8_SNORM:
		return 4;
		break;
	case VK_FORMAT_B8G8R8A8_USCALED:
		return 4;
		break;
	case VK_FORMAT_B8G8R8A8_SSCALED:
		return 4;
		break;
	case VK_FORMAT_B8G8R8A8_UINT:
		return 4;
		break;
	case VK_FORMAT_B8G8R8A8_SINT:
		return 4;
		break;
	case VK_FORMAT_B8G8R8A8_SRGB:
		return 4;
		break;
	case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
		return 4;
		break;
	case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
		return 4;
		break;
	case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
		return 4;
		break;
	case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
		return 4;
		break;
	case VK_FORMAT_A8B8G8R8_UINT_PACK32:
		return 4;
		break;
	case VK_FORMAT_A8B8G8R8_SINT_PACK32:
		return 4;
		break;
	case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
		return 4;
		break;
	/*case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
		break;
	case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
		break;
	case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
		break;
	case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
		break;
	case VK_FORMAT_A2R10G10B10_UINT_PACK32:
		break;
	case VK_FORMAT_A2R10G10B10_SINT_PACK32:
		break;
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		break;
	case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		break;
	case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
		break;
	case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
		break;
	case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		break;
	case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		break;*/
	/*case VK_FORMAT_R16_UNORM:
		break;
	case VK_FORMAT_R16_SNORM:
		break;
	case VK_FORMAT_R16_USCALED:
		break;
	case VK_FORMAT_R16_SSCALED:
		break;
	case VK_FORMAT_R16_UINT:
		break;
	case VK_FORMAT_R16_SINT:
		break;
	case VK_FORMAT_R16_SFLOAT:
		break;
	case VK_FORMAT_R16G16_UNORM:
		break;
	case VK_FORMAT_R16G16_SNORM:
		break;
	case VK_FORMAT_R16G16_USCALED:
		break;
	case VK_FORMAT_R16G16_SSCALED:
		break;
	case VK_FORMAT_R16G16_UINT:
		break;
	case VK_FORMAT_R16G16_SINT:
		break;
	case VK_FORMAT_R16G16_SFLOAT:
		break;
	case VK_FORMAT_R16G16B16_UNORM:
		break;
	case VK_FORMAT_R16G16B16_SNORM:
		break;
	case VK_FORMAT_R16G16B16_USCALED:
		break;
	case VK_FORMAT_R16G16B16_SSCALED:
		break;
	case VK_FORMAT_R16G16B16_UINT:
		break;
	case VK_FORMAT_R16G16B16_SINT:
		break;
	case VK_FORMAT_R16G16B16_SFLOAT:
		break;
	case VK_FORMAT_R16G16B16A16_UNORM:
		break;
	case VK_FORMAT_R16G16B16A16_SNORM:
		break;
	case VK_FORMAT_R16G16B16A16_USCALED:
		break;
	case VK_FORMAT_R16G16B16A16_SSCALED:
		break;
	case VK_FORMAT_R16G16B16A16_UINT:
		break;
	case VK_FORMAT_R16G16B16A16_SINT:
		break;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		break;*/
	/*case VK_FORMAT_R32_UINT:
		break;
	case VK_FORMAT_R32_SINT:
		break;
	case VK_FORMAT_R32_SFLOAT:
		break;
	case VK_FORMAT_R32G32_UINT:
		break;
	case VK_FORMAT_R32G32_SINT:
		break;
	case VK_FORMAT_R32G32_SFLOAT:
		break;
	case VK_FORMAT_R32G32B32_UINT:
		break;
	case VK_FORMAT_R32G32B32_SINT:
		break;
	case VK_FORMAT_R32G32B32_SFLOAT:
		break;
	case VK_FORMAT_R32G32B32A32_UINT:
		break;
	case VK_FORMAT_R32G32B32A32_SINT:
		break;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		break;*/
	/*case VK_FORMAT_R64_UINT:
		break;
	case VK_FORMAT_R64_SINT:
		break;
	case VK_FORMAT_R64_SFLOAT:
		break;
	case VK_FORMAT_R64G64_UINT:
		break;
	case VK_FORMAT_R64G64_SINT:
		break;
	case VK_FORMAT_R64G64_SFLOAT:
		break;
	case VK_FORMAT_R64G64B64_UINT:
		break;
	case VK_FORMAT_R64G64B64_SINT:
		break;
	case VK_FORMAT_R64G64B64_SFLOAT:
		break;
	case VK_FORMAT_R64G64B64A64_UINT:
		break;
	case VK_FORMAT_R64G64B64A64_SINT:
		break;
	case VK_FORMAT_R64G64B64A64_SFLOAT:
		break;*/
	//case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
	//	break;
	//case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
	//	break;
	//case VK_FORMAT_D16_UNORM:
	//	break;
	//case VK_FORMAT_X8_D24_UNORM_PACK32:
	//	break;
	//case VK_FORMAT_D32_SFLOAT:
	//	break;
	//case VK_FORMAT_S8_UINT:
	//	break;
	//case VK_FORMAT_D16_UNORM_S8_UINT:
	//	break;
	//case VK_FORMAT_D24_UNORM_S8_UINT:
	//	break;
	//case VK_FORMAT_D32_SFLOAT_S8_UINT:
	//	break;
	//case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_BC2_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC2_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_BC3_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC3_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_BC4_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC4_SNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC5_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC5_SNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC6H_UFLOAT_BLOCK:
	//	break;
	//case VK_FORMAT_BC6H_SFLOAT_BLOCK:
	//	break;
	//case VK_FORMAT_BC7_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_BC7_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_EAC_R11_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_EAC_R11_SNORM_BLOCK:
	//	break;
	//case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
	//	break;
	//case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
	//	break;
	//case VK_FORMAT_G8B8G8R8_422_UNORM:
	//	break;
	//case VK_FORMAT_B8G8R8G8_422_UNORM:
	//	break;
	//case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
	//	break;
	//case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
	//	break;
	//case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
	//	break;
	//case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
	//	break;
	//case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
	//	break;
	//case VK_FORMAT_R10X6_UNORM_PACK16:
	//	break;
	//case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
	//	break;
	//case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
	//	break;
	//case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
	//	break;
	//case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
	//	break;
	//case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_R12X4_UNORM_PACK16:
	//	break;
	//case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
	//	break;
	//case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
	//	break;
	//case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
	//	break;
	//case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
	//	break;
	//case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
	//	break;
	//case VK_FORMAT_G16B16G16R16_422_UNORM:
	//	break;
	//case VK_FORMAT_B16G16R16G16_422_UNORM:
	//	break;
	//case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
	//	break;
	//case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
	//	break;
	//case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
	//	break;
	//case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
	//	break;
	//case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
	//	break;
	//case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:
	//	break;
	//case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:
	//	break;
	//case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:
	//	break;
	//case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:
	//	break;
	//case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG:
	//	break;
	//case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG:
	//	break;
	//case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG:
	//	break;
	//case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG:
	//	break;
	//case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT:
	//	break;
	//case VK_FORMAT_G8B8G8R8_422_UNORM_KHR:
	//	break;
	//case VK_FORMAT_B8G8R8G8_422_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR:
	//	break;
	//case VK_FORMAT_R10X6_UNORM_PACK16_KHR:
	//	break;
	//case VK_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR:
	//	break;
	//case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR:
	//	break;
	//case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR:
	//	break;
	//case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR:
	//	break;
	//case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_R12X4_UNORM_PACK16_KHR:
	//	break;
	//case VK_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR:
	//	break;
	//case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR:
	//	break;
	//case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR:
	//	break;
	//case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR:
	//	break;
	//case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR:
	//	break;
	//case VK_FORMAT_G16B16G16R16_422_UNORM_KHR:
	//	break;
	//case VK_FORMAT_B16G16R16G16_422_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR:
	//	break;
	//case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR:
	//	break;
	//case VK_FORMAT_BEGIN_RANGE:
	//	break;
	//case VK_FORMAT_END_RANGE:
	//	break;
	/*case VK_FORMAT_RANGE_SIZE:
		break;
	case VK_FORMAT_MAX_ENUM:
		break;*/
	default:
		break;
	}

	return 0;
}

void FVulkanTexture::DestoryTexture()
{
	vkDestroySampler(m_Device->GetLogicalDevice(), m_TextureSampler, nullptr);
	vkDestroyImageView(m_Device->GetLogicalDevice(), m_TextureImageView, nullptr);

	vkDestroyImage(m_Device->GetLogicalDevice(), m_TextureImage, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), m_TextureImageMemory, nullptr);
}

FVulkanTexture2D::FVulkanTexture2D(const FVulkanDevice * InDevice, uint32_t InWidth, uint32_t InHeight, VkFormat InFormat)
	:FVulkanTexture(InDevice),m_Width(InWidth),m_Height(InHeight),m_Format(InFormat), m_CurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED)
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

void FVulkanTexture2D::UpdateFromData(const void * InSrcData, uint32_t InWidth, uint32_t InHeight, FVulkanCommandBufferManager* InCmdBufferManager)
{
	if (InWidth > m_Width || InHeight > m_Height) return;

	FVulkanStagingBuffer* stagingBuffer = new FVulkanStagingBuffer(m_Device, InWidth * InHeight * GetBppFromFormat(m_Format));
	stagingBuffer->UpdateFromData(InSrcData, InWidth * InHeight * GetBppFromFormat(m_Format));

	FVulkanCommandBuffer* cmdBuffer = InCmdBufferManager->GetNewCommandBuffer();

	class DestoryTask : public FVulkanCommandBuffer::DelayedTask
	{
	public:
		DestoryTask(FVulkanStagingBuffer* buffer) : m_buffer(buffer) {};
		~DestoryTask();

		void DoTask()
		{
			delete m_buffer;
		};
	private:
		FVulkanStagingBuffer* m_buffer;
	};
	cmdBuffer->AddDelayedTask(FVulkanCommandBuffer::DelayedTaskPtr(new DestoryTask(stagingBuffer)));

	cmdBuffer->Begin();

	TransitionImageLayout(cmdBuffer, m_TextureImage, 0, 0, m_CurrentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	m_CurrentLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	CopyBufferToImage(cmdBuffer, stagingBuffer->GetBuffer(), m_TextureImage, VkExtent3D{ InWidth , InHeight ,1 }, 0);
	TransitionImageLayout(cmdBuffer, m_TextureImage, 0, 0, m_CurrentLayout, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	cmdBuffer->End();
	InCmdBufferManager->GetQueue()->Submit(cmdBuffer); 
}
