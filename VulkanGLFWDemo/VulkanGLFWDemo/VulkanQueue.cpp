#include "VulkanQueue.h"
#include "VulkanUtil.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanMemory.h"

FVulkanQueue::FVulkanQueue(const FVulkanDevice * InDevice, uint32_t InFamilyIndex)
	:m_Device(InDevice), m_FamilyIndex(InFamilyIndex)
{
	vkGetDeviceQueue(m_Device->GetLogicalDevice(), m_FamilyIndex, 0, &m_Handle);
}

FVulkanQueue::~FVulkanQueue()
{
}

void FVulkanQueue::Submit(FVulkanCommandBuffer * CmdBuffer) const
{

	const VkCommandBuffer cmdBuffers[] = { CmdBuffer->GetHandle() };

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = cmdBuffers;
	submitInfo.signalSemaphoreCount = CmdBuffer->m_SignalSemaphores.size();
	submitInfo.pSignalSemaphores = CmdBuffer->m_SignalSemaphores.data();

	submitInfo.waitSemaphoreCount = CmdBuffer->m_WaitSemaphores.size();
	submitInfo.pWaitSemaphores = CmdBuffer->m_WaitSemaphores.data();
	submitInfo.pWaitDstStageMask = CmdBuffer->m_WaitFlags.data();
	///

	if (vkQueueSubmit(m_Handle, 1, &submitInfo, CmdBuffer->m_Fence->GetHandle()) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	CmdBuffer->m_State = FVulkanCommandBuffer::EState::Submitted;
	CmdBuffer->GetOwner()->RefreshFenceStatus(CmdBuffer);
}



FVulkanQueueManager::FVulkanQueueManager(const FVulkanDevice * InDevice)
	:m_Device(InDevice)
{
}

FVulkanQueueManager::~FVulkanQueueManager()
{
}

void FVulkanQueueManager::Setup()
{
	FVulkanUtil::QueueFamilyIndices indices = FVulkanUtil::FindQueueFamilies(m_Device->GetPhysicalDevice(), m_Device->GetInstance()->GetSurface());

	if (indices.GraphicsIndex >= 0) {
		m_GraphicsQueue.reset(new FVulkanQueue(m_Device, indices.GraphicsIndex));
	}

	if (indices.PresentIndex >= 0) {
		m_PresentQueue.reset(new FVulkanQueue(m_Device, indices.PresentIndex));
	}

	if (indices.ComputeIndex >= 0) {
		m_ComputeQueue.reset(new FVulkanQueue(m_Device, indices.ComputeIndex));
	}

	if (indices.TransferIndex >= 0) {
		m_TransferQueue.reset(new FVulkanQueue(m_Device, indices.TransferIndex));
	}


}

void FVulkanQueueManager::Release()
{
	m_GraphicsQueue.reset();
	m_PresentQueue.reset();
	m_ComputeQueue.reset();
	m_TransferQueue.reset();
	
}
