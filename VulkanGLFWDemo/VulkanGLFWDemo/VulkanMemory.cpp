#include "VulkanMemory.h"
#include <iostream>
#include "VulkanInstance.h"
#include "VulkanDevice.h"

FVulkanSemaphore::FVulkanSemaphore(const FVulkanDevice * InDevice)
	:m_Device(InDevice),m_Handle(VK_NULL_HANDLE)
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_Handle) != VK_SUCCESS) {
		throw std::runtime_error("[VulkanSemaphore] Failed to create!");
	}
}

FVulkanSemaphore::~FVulkanSemaphore()
{
	vkDestroySemaphore(m_Device->GetLogicalDevice(), m_Handle, nullptr);
}

FVulkanFence::FVulkanFence(const FVulkanDevice* InDevice, FVulkanFenceManager * InOwner)
	:m_Device(InDevice), m_Owner(InOwner), m_Handle(VK_NULL_HANDLE)
{
}

FVulkanFence::~FVulkanFence()
{
}

void FVulkanFence::Allocate(bool bCreateSignaled)
{
	VkFenceCreateInfo Info;
	Info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	Info.flags = bCreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
	m_State = bCreateSignaled ? EState::Signaled : EState::NotReady;
	vkCreateFence(m_Device->GetLogicalDevice(), &Info, nullptr, &m_Handle);
}

void FVulkanFence::Free()
{
	vkDestroyFence(m_Device->GetLogicalDevice(), m_Handle, nullptr);
}

void FVulkanFence::Reset()
{
	if (m_State != EState::NotReady)
	{
		vkResetFences(m_Device->GetLogicalDevice(), 1, &m_Handle);
		m_State = EState::NotReady;
	}
}

bool FVulkanFence::CheckState()
{
	if (m_State == EState::NotReady) {
		VkResult Result = vkGetFenceStatus(m_Device->GetLogicalDevice(), m_Handle);

		switch (Result)
		{
		case VK_SUCCESS:
			m_State = EState::Signaled;
			return true;

		case VK_NOT_READY:
			break;

		default:
			break;
		}
	}

	return false;
}

FVulkanFenceManager::FVulkanFenceManager(const FVulkanDevice * InDevice)
	:m_Device(InDevice)
{
}

FVulkanFenceManager::~FVulkanFenceManager()
{
	DestoryFences();
}

FVulkanFence * FVulkanFenceManager::GetNewFence(bool bCreateSignaled)
{
	if (m_FreeFences.size() != 0)
	{
		FVulkanFence* Fence = m_FreeFences[0];
		m_FreeFences.erase(m_FreeFences.begin());

		if (bCreateSignaled)
		{
			Fence->m_State = FVulkanFence::EState::Signaled;
		}
		return Fence;
	}

	FVulkanFence* NewFence = new FVulkanFence(m_Device, this);
	NewFence->Allocate(bCreateSignaled);
	m_Fences.push_back(NewFence);

	return NewFence;
}

void FVulkanFenceManager::CollectFreeFence(FVulkanFence * InFence)
{
	m_FreeFences.push_back(InFence);
}

void FVulkanFenceManager::DestoryFences()
{
	for (size_t i = 0; i < m_Fences.size(); i++)
	{
		if (m_Fences[i]->GetHandle) {
			m_Fences[i]->Free();
		}
		delete m_Fences[i];
	}
	m_Fences.clear();
	m_FreeFences.clear();
}
