#pragma once

#include <vulkan/vulkan.h>

class FVulkanDevice;
class FVulkanRenderTargetInfo;
class FVulkanRenderPass;
class FVulkanDescriptorSetManager;
class FVulkanShader;


class FVulkanGraphicsPipeline
{
public:
	FVulkanGraphicsPipeline(
		const FVulkanDevice* InDevice/*,const FVulkanRenderPass* InRenderPass, const FVulkanDescriptorSetManager* InSetManager, const FVulkanShader* InShader*/);
	~FVulkanGraphicsPipeline();


	void Setup(
		const FVulkanRenderTargetInfo* InTargetInfo, const FVulkanRenderPass* InRenderPass,
		const FVulkanDescriptorSetManager* InSetManager, const FVulkanShader* InShader
	);
	void Release();

private:

	void CreateGraphicsPipeline(
		const FVulkanRenderTargetInfo* InTargetInfo, const FVulkanRenderPass* InRenderPass,
		const FVulkanDescriptorSetManager* InSetManager, const FVulkanShader* InShader);

private:
	const FVulkanDevice* m_Device;

	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_GraphicsPipeline;

};
