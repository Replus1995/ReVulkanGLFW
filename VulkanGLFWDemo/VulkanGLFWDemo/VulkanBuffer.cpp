#include "VulkanBuffer.h"
#include "VulkanUtil.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"


FVulkanBuffer::FVulkanBuffer(const FVulkanDevice * InDevice, uint64_t InBufferSize, VkBufferUsageFlags InUsage)
	:m_Device(InDevice), m_BufferSize(InBufferSize)
{
	CreateBuffer(m_BufferSize, InUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_Memory);
}

FVulkanBuffer::~FVulkanBuffer()
{
	vkDestroyBuffer(m_Device->GetLogicalDevice(), m_Buffer, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), m_Memory, nullptr);
}

void FVulkanBuffer::UpdateBuffer(FVulkanCommandBuffer* InCmdBuffer, const void * InSrcData, uint64_t InSrcDataSize)
{
	if (InSrcDataSize > m_BufferSize) return;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(InSrcDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* dstData;
	vkMapMemory(m_Device->GetLogicalDevice(), stagingBufferMemory, 0, InSrcDataSize, 0, &dstData);
	memcpy(dstData, InSrcData, (size_t)InSrcDataSize);
	vkUnmapMemory(m_Device->GetLogicalDevice(), stagingBufferMemory);

	CopyBuffer(InCmdBuffer, stagingBuffer, m_Buffer, InSrcDataSize);

	vkDestroyBuffer(m_Device->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), stagingBufferMemory, nullptr);

}

void FVulkanBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_Device->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_Device->GetLogicalDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FVulkanUtil::FindMemoryType(m_Device->GetPhysicalDevice(), memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_Device->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(m_Device->GetLogicalDevice(), buffer, bufferMemory, 0);
}

void FVulkanBuffer::CopyBuffer(FVulkanCommandBuffer* InCmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{

	InCmdBuffer->Begin();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(InCmdBuffer->GetHandle(), srcBuffer, dstBuffer, 1, &copyRegion);

	InCmdBuffer->End();
	InCmdBuffer->GetOwner()->GetQueue()->Submit(InCmdBuffer);
}
