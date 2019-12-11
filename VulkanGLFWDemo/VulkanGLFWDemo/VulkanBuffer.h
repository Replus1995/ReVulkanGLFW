#pragma once

#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanCommandBuffer;

class FVulkanBuffer
{
public:
	FVulkanBuffer(const FVulkanDevice* InDevice, uint64_t InBufferSize, VkBufferUsageFlags InUsage);
	~FVulkanBuffer();

	void UpdateBuffer(FVulkanCommandBuffer* InCmdBuffer, const void* InSrcData, uint64_t InSrcDataSize);

protected:
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory);
	void CopyBuffer(FVulkanCommandBuffer* InCmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
	const FVulkanDevice* m_Device;

	VkDeviceSize m_BufferSize;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;

};

