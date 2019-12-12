#pragma once
#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanCommandBuffer;
class FVulkanCommandBufferManager;

class FVulkanTexture
{
public:
	FVulkanTexture(const FVulkanDevice* InDevice);
	virtual ~FVulkanTexture();

protected:
	void CreateTexture(VkImageType imagetype, VkFormat format, VkExtent3D extent, 
		uint32_t miplevels, uint32_t arraylayers, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
		VkImage& image, VkDeviceMemory& memory);
	void CreateImageView(VkImage image, VkImageViewType viewtype, VkFormat format, uint32_t levels, uint32_t layers, VkImageView& view);
	void CreateTextureSampler(VkSampler& sampler);

	void TransitionImageLayout(FVulkanCommandBuffer* InCmdBuffer, VkImage image, uint32_t levels, uint32_t layers, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(FVulkanCommandBuffer* InCmdBuffer, VkBuffer buffer, VkImage image, VkExtent3D extent, uint32_t layers);
	
	void DestoryTexture();


	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
	VkImageView m_TextureImageView;
	VkSampler m_TextureSampler;

	VkImageLayout m_CurrentLayout;

	const FVulkanDevice* m_Device;
private:
	
};


class FVulkanTexture2D : public FVulkanTexture
{
public:
	FVulkanTexture2D(const FVulkanDevice* InDevice, uint32_t InWidth, uint32_t InHeight, VkFormat InFormat);
	~FVulkanTexture2D();

	void UpdateFromData(const void* InSrcData, uint32_t InWidth, uint32_t InHeight);


private:
	uint32_t m_Width;
	uint32_t m_Height;
	VkFormat m_Format;
	

};