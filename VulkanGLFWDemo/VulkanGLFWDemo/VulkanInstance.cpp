#include "VulkanInstance.h"
#include "VulkanUtil.h"
#include "VulkanWindow.h"
#include "VulkanDebugger.h"

FVulkanInstance::FVulkanInstance(const FVulkanWindow* Window)
	:m_Window(Window)
{
}

FVulkanInstance::~FVulkanInstance()
{
}

void FVulkanInstance::Setup(const char * InApplicationName, const char * InEngineName)
{
	if (m_InstanceCreated) return;

#ifdef _DEBUG
	m_Debugger.reset(new FVulkanDebugger());
//#else
//	const bool enableValidationLayers = true;
#endif

	CreateInstance(InApplicationName, InEngineName);
	CreateSurface();
	if (m_Debugger) {
		m_Debugger->SetupDebugCallback(m_Handle);
	}

	m_InstanceCreated = true;
}

void FVulkanInstance::Release()
{
	if (!m_InstanceCreated) return;

	if (m_Debugger) {
		m_Debugger->DestoryDebugCallback(m_Handle);
		m_Debugger.reset();
	}
	vkDestroySurfaceKHR(m_Handle, m_Surface, nullptr);
	vkDestroyInstance(m_Handle, nullptr);

	m_InstanceCreated = false;
}

void FVulkanInstance::CreateInstance(const char * InApplicationName, const char * InEngineName)
{
	if (m_Debugger) {
		if (!m_Debugger->CheckValidationLayerSupport()) {
			//throw std::runtime_error("validation layers requested, but not available!");
			printf("validation layers requested, but not available!");
			m_Debugger.reset();
		}
	}

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = InApplicationName;
	app_info.applicationVersion = 1;
	app_info.pEngineName = InEngineName;
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_0;

	// initialize the VkInstanceCreateInfo structure
	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = 0;
	create_info.ppEnabledExtensionNames = NULL;
	create_info.enabledLayerCount = 0;
	create_info.ppEnabledLayerNames = NULL;

	std::vector<const char*> extensions;
	FVulkanUtil::GetInstanceExtensions(extensions);
	if (m_Debugger) {
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	std::vector<const char*> layers;
	if (m_Debugger) {
		const std::vector<const char*>& debug_layers = m_Debugger->GetValidationLayers();
		for (size_t i = 0; i < debug_layers.size(); i++)
		{
			layers.push_back(debug_layers[i]);
		}
	}
	create_info.enabledLayerCount = layers.size();
	create_info.ppEnabledLayerNames = layers.data();


	VkResult res;

	res = vkCreateInstance(&create_info, NULL, &m_Handle);
	if (res == VK_ERROR_INCOMPATIBLE_DRIVER) {
		std::cout << "cannot find a compatible Vulkan ICD\n";
		exit(-1);
	}
	else if (res) {
		std::cout << "unknown error\n";
		exit(-1);
	}

	//Checking for extension support

	uint32_t extension_properties_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_properties_count, nullptr);

	std::vector<VkExtensionProperties> extension_properties(extension_properties_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_properties_count, extension_properties.data());

	std::cout << "available extensions:" << std::endl;

	for (const auto& extension_properties : extension_properties) {
		std::cout << "\t" << extension_properties.extensionName << std::endl;
	}

	//Checking for extension support
}

void FVulkanInstance::CreateSurface()
{
	m_Window->CreateVulkanSurface(m_Handle, m_Surface);
}
