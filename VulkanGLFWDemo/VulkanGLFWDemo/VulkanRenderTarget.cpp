#include "VulkanRenderTarget.h"

void FVulkanRenderTargetInfo::SetExtent(uint32_t InWidth, uint32_t InHeight, uint32_t InDepth)
{
	m_Extent.Extent3D.width = InWidth;
	m_Extent.Extent3D.height = InHeight;
	m_Extent.Extent3D.depth = InDepth;
}

