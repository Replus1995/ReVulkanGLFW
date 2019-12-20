#pragma once

#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanCommandBuffer;

class FVulkanBufferBase
{
public:
	FVulkanBufferBase(const FVulkanDevice* InDevice, uint64_t InBufferSize);
	virtual ~FVulkanBufferBase();

	inline VkBuffer GetBuffer() const 
	{
		return m_Buffer;
	}

	inline VkDeviceMemory GetMemory() const
	{
		return m_Memory;
	}

protected:
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory);
	void CopyBuffer(FVulkanCommandBuffer* InCmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


protected:
	const FVulkanDevice* m_Device;
	VkDeviceSize m_BufferSize;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;

};

class FVulkanStagingBuffer : public FVulkanBufferBase
{
public:
	FVulkanStagingBuffer(const FVulkanDevice* InDevice, uint64_t InBufferSize);
	~FVulkanStagingBuffer();

	void UpdateFromData(const void* InSrcData, uint64_t InSrcDataSize);

private:

};

class FVulkanBuffer : public FVulkanBufferBase
{
public:
	FVulkanBuffer(const FVulkanDevice* InDevice, uint64_t InBufferSize, VkBufferUsageFlags InUsage);
	~FVulkanBuffer();

	void UpdateBuffer(FVulkanCommandBuffer* InCmdBuffer, const void* InSrcData, uint64_t InSrcDataSize);

private:

};
