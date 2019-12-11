#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanMemory.h"


FVulkanCommandBuffer::FVulkanCommandBuffer(const FVulkanDevice* InDevice, FVulkanCommandBufferManager* InOwner)
	:m_Device(InDevice), m_Owner(InOwner)
{
}

FVulkanCommandBuffer::~FVulkanCommandBuffer()
{
}

void FVulkanCommandBuffer::AddWaitSemaphore(VkPipelineStageFlags InWaitFlags, const FVulkanSemaphore * InWaitSemaphore)
{
	m_WaitFlags.push_back(InWaitFlags);
	m_WaitSemaphores.push_back(InWaitSemaphore->GetHandle());
}

void FVulkanCommandBuffer::AddSignalSemaphore(const FVulkanSemaphore * InSignalSemaphore)
{
	m_SignalSemaphores.push_back(InSignalSemaphore->GetHandle());
}

void FVulkanCommandBuffer::NeverUse()
{
	Reset();
	m_Owner->CollectUsedBuffer(this);
}

void FVulkanCommandBuffer::Begin()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(m_Handle, &beginInfo);
}

void FVulkanCommandBuffer::End()
{
	vkEndCommandBuffer(m_Handle);
}

void FVulkanCommandBuffer::AllocMemory()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_Owner->GetPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(m_Device->GetLogicalDevice(), &allocInfo, &m_Handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void FVulkanCommandBuffer::FreeMemory()
{
	if (m_Handle != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(m_Device->GetLogicalDevice(), m_Owner->GetPool(), 1, &m_Handle);
		m_Handle = VK_NULL_HANDLE;
	}
	
}

void FVulkanCommandBuffer::Reset()
{
	m_WaitFlags.clear();
	m_WaitSemaphores.clear();
	m_SignalSemaphores.clear();
}


FVulkanCommandBufferManager::FVulkanCommandBufferManager(const FVulkanDevice* InDevice, const FVulkanQueue* InQueue)
	:m_Device(InDevice), m_Queue(InQueue)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.pNext = NULL;
	poolInfo.queueFamilyIndex = m_Queue->GetFamilyIndex();
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_Pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
	//ActiveCmdBuffer->Begin();
}

FVulkanCommandBufferManager::~FVulkanCommandBufferManager()
{
	DestoryBuffers();

	vkDestroyCommandPool(m_Device->GetLogicalDevice(), m_Pool, nullptr);
	m_Pool = VK_NULL_HANDLE;
}

FVulkanCommandBuffer * FVulkanCommandBufferManager::GetNewCommandBuffer()
{
	for (int32_t Index = m_FreeCmdBuffers.size() - 1; Index >= 0; --Index)
	{
		FVulkanCommandBuffer* cmdBuffer = m_FreeCmdBuffers[Index];
		{
			m_FreeCmdBuffers.erase(m_FreeCmdBuffers.begin() + Index);
			cmdBuffer->AllocMemory();
			m_CmdBuffers.push_back(cmdBuffer);
			return cmdBuffer;
		}
	}

	FVulkanCommandBuffer* CmdBuffer = new FVulkanCommandBuffer(m_Device, this);
	CmdBuffer->AllocMemory();
	m_CmdBuffers.push_back(CmdBuffer);

	return CmdBuffer;
}



void FVulkanCommandBufferManager::DestoryBuffers()
{
	for (int32_t Index = 0; Index < m_CmdBuffers.size(); ++Index)
	{
		FVulkanCommandBuffer* CmdBuffer = m_CmdBuffers[Index];
		CmdBuffer->FreeMemory();
		delete CmdBuffer;
	}
	m_CmdBuffers.clear();

	/*for (int32_t Index = 0; Index < m_FreeCmdBuffers.size(); ++Index)
	{
		FVulkanCommandBuffer* CmdBuffer = m_FreeCmdBuffers[Index];
		delete CmdBuffer;
	}*/
	m_FreeCmdBuffers.clear();

}

void FVulkanCommandBufferManager::CollectUsedBuffer(FVulkanCommandBuffer * InUsedBuffer)
{
	m_FreeCmdBuffers.push_back(InUsedBuffer);
}

