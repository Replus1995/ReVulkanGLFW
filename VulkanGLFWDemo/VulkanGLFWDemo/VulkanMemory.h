#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanFenceManager;

class FVulkanSemaphore
{
public:
	FVulkanSemaphore(const FVulkanDevice* InDevice);
	~FVulkanSemaphore();

	inline VkSemaphore GetHandle() const
	{
		return m_Handle;
	}

private:
	const FVulkanDevice* m_Device;
	VkSemaphore m_Handle;
};


class FVulkanFence
{
public:
	FVulkanFence(const FVulkanDevice* InDevice, FVulkanFenceManager* InOwner);
	~FVulkanFence();

	inline VkFence GetHandle() const
	{
		return m_Handle;
	}

	FVulkanFenceManager* GetOwner()
	{
		return m_Owner;
	}

	inline bool IsSignaled()
	{
		if (m_State == EState::Signaled) return true;

		return  CheckState();
	}

	void Reset();

protected:
	friend class FVulkanFenceManager;

	void Allocate(bool bCreateSignaled);
	void Free();
	bool CheckState();

private:
	const FVulkanDevice* m_Device;
	FVulkanFenceManager* m_Owner;
	VkFence m_Handle;

	enum class EState
	{
		// Initial state
		NotReady,

		// After GPU processed it
		Signaled,
	};

	EState m_State;
};

class FVulkanFenceManager
{
public:
	FVulkanFenceManager(const FVulkanDevice* InDevice);
	~FVulkanFenceManager();

	FVulkanFence* GetNewFence(bool bCreateSignaled);

protected:
	friend class FVulkanFence;
	void CollectFreeFence(FVulkanFence* InFence);
	void DestoryFences();

private:
	const FVulkanDevice* m_Device;
	std::vector< FVulkanFence*> m_Fences;
	std::vector< FVulkanFence*> m_FreeFences;

	
};
