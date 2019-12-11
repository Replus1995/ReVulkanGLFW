#pragma once

#include <memory>
#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanCommandBuffer;
class FVulkanSemaphore;

class FVulkanQueue
{
public:
	FVulkanQueue(const FVulkanDevice* InDevice, uint32_t InFamilyIndex);
	~FVulkanQueue();


	inline VkQueue GetHandle() const
	{
		return m_Handle;
	}

	inline uint32_t GetFamilyIndex() const
	{
		return m_FamilyIndex;
	}

	void Submit(FVulkanCommandBuffer* CmdBuffer) const;



private:
	const FVulkanDevice* m_Device;
	VkQueue m_Handle;
	uint32_t m_FamilyIndex;
	uint32_t m_QueueIndex;

};


class FVulkanQueueManager
{
public:
	FVulkanQueueManager(const FVulkanDevice* InDevice);
	~FVulkanQueueManager();

	inline const FVulkanQueue* GetGraphicsQueue() const 
	{
		return m_GraphicsQueue.get();
	}
	inline const FVulkanQueue* GetPresentQueue() const
	{
		return m_PresentQueue.get();
	}
	inline const FVulkanQueue* GetComputeQueue() const
	{
		return m_ComputeQueue.get();
	}
	inline const FVulkanQueue* GetTransferQueue() const
	{
		return m_TransferQueue.get();
	}

	void Setup();
	void Release();

private:
	const FVulkanDevice* m_Device;

	std::unique_ptr<FVulkanQueue> m_GraphicsQueue;
	std::unique_ptr<FVulkanQueue> m_PresentQueue;
	std::unique_ptr<FVulkanQueue> m_ComputeQueue;
	std::unique_ptr<FVulkanQueue> m_TransferQueue;
};

