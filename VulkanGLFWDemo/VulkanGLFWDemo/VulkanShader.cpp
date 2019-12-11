#include "VulkanShader.h"
#include <fstream>
#include "VulkanDevice.h"

FVulkanShader::FVulkanShader(const FVulkanDevice * InDevice)
	:m_Device(InDevice)
{
	for (uint8_t i = 0; i < SHADER_TYPE_RANGE_SIZE; i++)
	{
		m_Shaders[i].Module = VK_NULL_HANDLE;
		m_Shaders[i].Entry.clear();
	}
}

FVulkanShader::~FVulkanShader()
{
}

void FVulkanShader::LoadShaderFromFile(const char * InShaderPath, const char * InShaderEntry, ShaderType InShaderType)
{
	if (m_Shaders[InShaderType].Module) {
		vkDestroyShaderModule(m_Device->GetLogicalDevice(), m_Shaders[InShaderType].Module, nullptr);
		m_Shaders[InShaderType].Module = VK_NULL_HANDLE;
	}

	std::vector<char> shaderCode;
	LoadBufferFromFile(shaderCode, InShaderPath);
	m_Shaders[InShaderType].Module = CreateShaderModule(shaderCode);
	m_Shaders[InShaderType].Entry = InShaderEntry;

}

void FVulkanShader::ReleaseAllShaders()
{
	for (uint8_t i = 0; i < SHADER_TYPE_RANGE_SIZE; i++)
	{
		if (m_Shaders[i].Module) {
			vkDestroyShaderModule(m_Device->GetLogicalDevice(), m_Shaders[i].Module, nullptr);
			m_Shaders[i].Module = VK_NULL_HANDLE;
		}
		m_Shaders[i].Entry.clear();
	}
}

void FVulkanShader::GetShaderStages(std::vector<VkPipelineShaderStageCreateInfo>& OutStages) const
{
	OutStages.clear();
	for (uint8_t shaderIndex = 0; shaderIndex < SHADER_TYPE_RANGE_SIZE; shaderIndex++)
	{
		if (m_Shaders[shaderIndex].Module) {

			VkPipelineShaderStageCreateInfo shaderStageInfo = {};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.module = m_Shaders[shaderIndex].Module;
			shaderStageInfo.pName = m_Shaders[shaderIndex].Entry.c_str();

			switch (shaderIndex)
			{
			case SHADER_TYPE_VERTEX:
				shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			case SHADER_TYPE_FRAGMENT:
				shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			case SHADER_TYPE_COMPUTE:
				shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			case SHADER_TYPE_GEOMETRY:
				shaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			default:
				break;
			}

			OutStages.push_back(shaderStageInfo);
		}
	}
}

void FVulkanShader::LoadBufferFromFile(std::vector<char>& OutBuffer, const std::string & InFileName)
{
	std::ifstream file(InFileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("[VulkanShader] Failed to open shader file!");
	}

	size_t fileSize = (size_t)file.tellg();
	OutBuffer.resize(fileSize);

	file.seekg(0);
	file.read(OutBuffer.data(), fileSize);
	file.close();

}

VkShaderModule FVulkanShader::CreateShaderModule(const std::vector<char>& InCode)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = InCode.size();

	createInfo.pCode = reinterpret_cast<const uint32_t*>(InCode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_Device->GetLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}
