#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanQueue;
class FVulkanSemaphore;

class FVulkanCommandBuffer
{
protected:
	friend class FVulkanCommandBufferManager;
	friend class FVulkanQueue;

	FVulkanCommandBuffer(const FVulkanDevice* InDevice, FVulkanCommandBufferManager* InOwner);
	~FVulkanCommandBuffer();

public:
	
	void AddWaitSemaphore(VkPipelineStageFlags InWaitFlags, const FVulkanSemaphore* InWaitSemaphore);
	void AddSignalSemaphore(const FVulkanSemaphore* InSignalSemaphore);

	void NeverUse();

	void Begin();
	void End();

	void BeginRenderPass();
	void EndRenderPass();

	inline VkCommandBuffer GetHandle() const
	{
		return m_Handle;
	}

	inline FVulkanCommandBufferManager* GetOwner() const
	{
		return m_Owner;
	}

private:
	const FVulkanDevice* m_Device;
	FVulkanCommandBufferManager* m_Owner;

	VkCommandBuffer m_Handle = VK_NULL_HANDLE;

	std::vector<VkPipelineStageFlags> m_WaitFlags;
	std::vector<VkSemaphore> m_WaitSemaphores;
	std::vector<VkSemaphore> m_SignalSemaphores;


	void AllocMemory();
	void FreeMemory();
	void Reset();
};



class FVulkanCommandBufferManager
{
public:
	FVulkanCommandBufferManager(const FVulkanDevice* InDevice, const FVulkanQueue* InQueue);
	~FVulkanCommandBufferManager();

	inline const FVulkanQueue* GetQueue() const 
	{
		return m_Queue;
	}

	FVulkanCommandBuffer* GetNewCommandBuffer();


protected:
	friend class FVulkanCommandBuffer;

	inline VkCommandPool GetPool() const
	{
		return m_Pool;
	}

private:
	const FVulkanDevice* m_Device;
	const FVulkanQueue* m_Queue;

	VkCommandPool m_Pool;
	std::vector<FVulkanCommandBuffer*> m_CmdBuffers;
	std::vector<FVulkanCommandBuffer*> m_FreeCmdBuffers;

	void DestoryBuffers();
	void CollectUsedBuffer(FVulkanCommandBuffer* InUsedBuffer);

};