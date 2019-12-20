#include "VulkanBuffer.h"
#include "VulkanUtil.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"


FVulkanBufferBase::FVulkanBufferBase(const FVulkanDevice * InDevice, uint64_t InBufferSize)
	:m_Device(InDevice), m_BufferSize(InBufferSize)
{
	
}

FVulkanBufferBase::~FVulkanBufferBase()
{
}


void FVulkanBufferBase::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
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

void FVulkanBufferBase::CopyBuffer(FVulkanCommandBuffer* InCmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{

	InCmdBuffer->Begin();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(InCmdBuffer->GetHandle(), srcBuffer, dstBuffer, 1, &copyRegion);

	InCmdBuffer->End();
	InCmdBuffer->GetOwner()->GetQueue()->Submit(InCmdBuffer);
}

FVulkanBuffer::FVulkanBuffer(const FVulkanDevice * InDevice, uint64_t InBufferSize, VkBufferUsageFlags InUsage)
	:FVulkanBufferBase(InDevice, InBufferSize)
{
	CreateBuffer(m_BufferSize, InUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_Memory);
}

FVulkanBuffer::~FVulkanBuffer()
{
	vkDestroyBuffer(m_Device->GetLogicalDevice(), m_Buffer, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), m_Memory, nullptr);
}

void FVulkanBuffer::UpdateBuffer(FVulkanCommandBuffer * InCmdBuffer, const void * InSrcData, uint64_t InSrcDataSize)
{
	if (InSrcDataSize > m_BufferSize) return;


	FVulkanStagingBuffer* stagingBuffer = new FVulkanStagingBuffer(m_Device, InSrcDataSize);
	stagingBuffer->UpdateFromData(InSrcData, InSrcDataSize);

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
	InCmdBuffer->AddDelayedTask(FVulkanCommandBuffer::DelayedTaskPtr(new DestoryTask(stagingBuffer)));

	CopyBuffer(InCmdBuffer, stagingBuffer->GetBuffer(), m_Buffer, InSrcDataSize);

	/*VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(InSrcDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* dstData;
	vkMapMemory(m_Device->GetLogicalDevice(), stagingBufferMemory, 0, InSrcDataSize, 0, &dstData);
	memcpy(dstData, InSrcData, (size_t)InSrcDataSize);
	vkUnmapMemory(m_Device->GetLogicalDevice(), stagingBufferMemory);

	CopyBuffer(InCmdBuffer, stagingBuffer, m_Buffer, InSrcDataSize);

	vkDestroyBuffer(m_Device->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), stagingBufferMemory, nullptr);*/
}

FVulkanStagingBuffer::FVulkanStagingBuffer(const FVulkanDevice * InDevice, uint64_t InBufferSize)
	:FVulkanBufferBase(InDevice, InBufferSize)
{
	CreateBuffer(m_BufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_Memory);
}

FVulkanStagingBuffer::~FVulkanStagingBuffer()
{
	vkDestroyBuffer(m_Device->GetLogicalDevice(), m_Buffer, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), m_Memory, nullptr);
}

void FVulkanStagingBuffer::UpdateFromData(const void * InSrcData, uint64_t InSrcDataSize)
{
	if (InSrcDataSize > m_BufferSize) return;

	void* dstData;
	vkMapMemory(m_Device->GetLogicalDevice(), m_Memory, 0, InSrcDataSize, 0, &dstData);
	memcpy(dstData, InSrcData, (size_t)InSrcDataSize);
	vkUnmapMemory(m_Device->GetLogicalDevice(), m_Memory);

}
