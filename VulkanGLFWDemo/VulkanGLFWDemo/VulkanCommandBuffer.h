#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanQueue;
class FVulkanSemaphore;
class FVulkanFence;

class FVulkanCommandBuffer
{
protected:
	friend class FVulkanCommandBufferManager;
	friend class FVulkanQueue;

	FVulkanCommandBuffer(const FVulkanDevice* InDevice, FVulkanCommandBufferManager* InOwner);
	~FVulkanCommandBuffer();

public:

	inline VkCommandBuffer GetHandle() const
	{
		return m_Handle;
	}

	inline FVulkanCommandBufferManager* GetOwner() const
	{
		return m_Owner;
	}

	void AddWaitSemaphore(VkPipelineStageFlags InWaitFlags, const FVulkanSemaphore* InWaitSemaphore);
	void AddSignalSemaphore(const FVulkanSemaphore* InSignalSemaphore);

	enum class EState : uint8_t
	{
		ReadyForBegin,
		IsInsideBegin,
		IsInsideRenderPass,
		HasEnded,
		Submitted,
		NotAllocated,
	};


	class DelayedTask
	{
	public:
		DelayedTask() {};
		virtual ~DelayedTask() {};

		virtual void DoTask() {};
	};

	typedef std::shared_ptr<DelayedTask> DelayedTaskPtr;

	void AddDelayedTask(DelayedTaskPtr InTask);

	void Begin();
	void End();

	/*void BeginRenderPass();
	void EndRenderPass();*/

	inline bool HasBegun() const 
	{
		return m_State == EState::IsInsideBegin;
	}

	inline bool HasEnded() const
	{
		return m_State == EState::HasEnded;
	}

protected:
	EState m_State;
	FVulkanFence* m_Fence;

	void RefreshFenceStatus();

	std::vector<DelayedTaskPtr> m_DelayedTasks;

private:
	const FVulkanDevice* m_Device;
	FVulkanCommandBufferManager* m_Owner;

	VkCommandBuffer m_Handle = VK_NULL_HANDLE;

	std::vector<VkPipelineStageFlags> m_WaitFlags;
	std::vector<VkSemaphore> m_WaitSemaphores;
	std::vector<VkSemaphore> m_SignalSemaphores;


	void AllocMemory();
	void FreeMemory();
	void ResetSemaphores();
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


	void RefreshFenceStatus(FVulkanCommandBuffer* SkipCmdBuffer = nullptr);

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