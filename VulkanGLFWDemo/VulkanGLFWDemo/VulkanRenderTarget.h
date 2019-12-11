#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class FVulkanRenderTargetInfo
{
public:
	FVulkanRenderTargetInfo() {};
	~FVulkanRenderTargetInfo() {};

	inline const VkExtent2D& GetExtent2D() const { return m_Extent.Extent2D; }
	inline const VkExtent3D& GetExtent3D() const { return m_Extent.Extent3D; }
	inline const VkAttachmentDescription* GetAttachmentDescriptions() const { return m_Descs.data(); }
	inline uint32_t GetNumAttachmentDescriptions() const { return m_Descs.size(); }

	inline bool GetHasDepthStencil() const { return m_DepthStencilReferences.size() > 0; }
	inline bool GetHasResolveAttachments() const { return m_ResolveReferences.size() > 0; }

	inline uint32_t GetNumColorAttachments() const { return m_ColorReferences.size(); }
	inline uint32_t GetNumInputAttachments() const { return m_InputReferences.size(); }

	inline const VkAttachmentReference* GetColorAttachmentReferences() const { return  m_ColorReferences.size() > 0 ? m_ColorReferences.data() : nullptr; }
	inline const VkAttachmentReference* GetDepthStencilAttachmentReference() const { return m_DepthStencilReferences.size() > 0 ? m_DepthStencilReferences.data() : nullptr; }
	inline const VkAttachmentReference* GetResolveAttachmentReferences() const { return  m_ResolveReferences.size() > 0 ? m_ResolveReferences.data() : nullptr; }
	inline const VkAttachmentReference* GetInputAttachmentReferences() const { return  m_InputReferences.size() > 0 ? m_InputReferences.data() : nullptr; }

protected:
	void SetExtent(uint32_t InWidth, uint32_t InHeight, uint32_t InDepth = 0);

private:

	union
	{
		VkExtent3D	Extent3D;
		VkExtent2D	Extent2D;
	} m_Extent;

	std::vector<VkAttachmentReference> m_ColorReferences;
	std::vector<VkAttachmentReference> m_DepthStencilReferences;
	std::vector<VkAttachmentReference> m_ResolveReferences;
	std::vector<VkAttachmentReference> m_InputReferences;

	std::vector<VkAttachmentDescription> m_Descs;
};
