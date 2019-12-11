#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class FVulkanDevice;

class FVulkanShader
{
public:
	FVulkanShader(const FVulkanDevice* InDevice);
	~FVulkanShader();

	enum ShaderType : uint8_t
	{
		SHADER_TYPE_VERTEX = 0,
		SHADER_TYPE_FRAGMENT = 1, 
		SHADER_TYPE_COMPUTE = 2,
		SHADER_TYPE_GEOMETRY = 3,
		SHADER_TYPE_RANGE_SIZE = 4
	};

	void LoadShaderFromFile(const char* InShaderPath, const char* InShaderEntry, ShaderType InShaderType);
	void ReleaseAllShaders();
	void GetShaderStages(std::vector<VkPipelineShaderStageCreateInfo>& OutStages) const;
	
private:

	void LoadBufferFromFile(std::vector<char>& OutBuffer, const std::string& InFileName);
	VkShaderModule CreateShaderModule(const std::vector<char>& InCode);

private:
	const FVulkanDevice* m_Device;

	struct DetailedShader
	{
		VkShaderModule Module;
		std::string Entry;
	};

	DetailedShader m_Shaders[SHADER_TYPE_RANGE_SIZE];
};
