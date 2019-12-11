#include "ValkanCudaApp.h"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
};

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks * pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void varySigma()
{
	filter_radius += g_nFilterSign;

	if (filter_radius > 64) {
		filter_radius = 64;  // clamp to 64 and then negate sign
		g_nFilterSign = -1;
	}
	else if (filter_radius < 0) {
		filter_radius = 0;
		g_nFilterSign = 1;
	}
}


void vulkanImageCUDA::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics command pool!");
	}
}

void vulkanImageCUDA::createTextureImage()
{

	std::vector<UINT8> tex_data = GenerateTextureData(m_imageWidth, m_imageHeight, 4);

	VkDeviceSize imageSize = m_imageWidth * m_imageHeight * 4;
	/*mipLevels = static_cast<uint32_t>(
		std::floor(std::log2(std::max(imageWidth, imageHeight)))) +
		1;*/
	mipLevels = 1;
	printf("mipLevels = %d\n", mipLevels);

	if (!tex_data.size()) {
		throw std::runtime_error("failed to create texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, tex_data.data(), static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	// VK_FORMAT_R8G8B8A8_UNORM changed to VK_FORMAT_R8G8B8A8_UINT
	createImage(
		m_imageWidth, m_imageHeight, VK_FORMAT_R8G8B8A8_UINT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UINT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, textureImage,
		static_cast<uint32_t>(m_imageWidth),
		static_cast<uint32_t>(m_imageHeight));
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UINT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	//generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_UNORM);
}

void vulkanImageCUDA::generateMipmaps(VkImage image, VkFormat imageFormat)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat,
		&formatProperties);

	if (!(formatProperties.optimalTilingFeatures &
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error(
			"texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = m_imageWidth;
	int32_t mipHeight = m_imageHeight;

	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
			nullptr, 1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
							  mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
			0, nullptr, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
		0, nullptr, 1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

void vulkanImageCUDA::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0;  // Optional
	samplerInfo.maxLod = static_cast<float>(mipLevels);
	samplerInfo.mipLodBias = 0;  // Optional

	if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

VkImageView vulkanImageCUDA::createImageView(VkImage image, VkFormat format)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void vulkanImageCUDA::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkExternalMemoryImageCreateInfo vkExternalMemImageCreateInfo = {};
	vkExternalMemImageCreateInfo.sType =
		VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	vkExternalMemImageCreateInfo.pNext = NULL;
	vkExternalMemImageCreateInfo.handleTypes =
		VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;

	imageInfo.pNext = &vkExternalMemImageCreateInfo;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

#ifdef _WIN64
	WindowsSecurityAttributes winSecurityAttributes;

	VkExportMemoryWin32HandleInfoKHR vulkanExportMemoryWin32HandleInfoKHR = {};
	vulkanExportMemoryWin32HandleInfoKHR.sType =
		VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
	vulkanExportMemoryWin32HandleInfoKHR.pNext = NULL;
	vulkanExportMemoryWin32HandleInfoKHR.pAttributes = &winSecurityAttributes;
	vulkanExportMemoryWin32HandleInfoKHR.dwAccess =
		DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
	vulkanExportMemoryWin32HandleInfoKHR.name = (LPCWSTR)NULL;
#endif
	VkExportMemoryAllocateInfoKHR vulkanExportMemoryAllocateInfoKHR = {};
	vulkanExportMemoryAllocateInfoKHR.sType =
		VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
#ifdef _WIN64
	vulkanExportMemoryAllocateInfoKHR.pNext =
		IsWindows8OrGreater() ? &vulkanExportMemoryWin32HandleInfoKHR : NULL;
	vulkanExportMemoryAllocateInfoKHR.handleTypes =
		IsWindows8OrGreater()
		? VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
		: VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
#else
	vulkanExportMemoryAllocateInfoKHR.pNext = NULL;
	vulkanExportMemoryAllocateInfoKHR.handleTypes =
		VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.pNext = &vulkanExportMemoryAllocateInfoKHR;
	allocInfo.memoryTypeIndex =
		findMemoryType(memRequirements.memoryTypeBits, properties);

	VkMemoryRequirements vkMemoryRequirements = {};
	vkGetImageMemoryRequirements(device, image, &vkMemoryRequirements);
	totalImageMemSize = vkMemoryRequirements.size;

	if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, textureImageMemory, 0);
}

void vulkanImageCUDA::cudaVkImportSemaphore()
{
	cudaExternalSemaphoreHandleDesc externalSemaphoreHandleDesc;
	memset(&externalSemaphoreHandleDesc, 0,
		sizeof(externalSemaphoreHandleDesc));
#ifdef _WIN64
	externalSemaphoreHandleDesc.type =
		IsWindows8OrGreater() ? cudaExternalSemaphoreHandleTypeOpaqueWin32
		: cudaExternalSemaphoreHandleTypeOpaqueWin32Kmt;
	externalSemaphoreHandleDesc.handle.win32.handle = getVkSemaphoreHandle(
		IsWindows8OrGreater()
		? VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
		: VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
		cudaUpdateVkSemaphore);
#else
	externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
	externalSemaphoreHandleDesc.handle.fd = getVkSemaphoreHandle(
		VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT, cudaUpdateVkSemaphore);
#endif
	externalSemaphoreHandleDesc.flags = 0;

	checkCudaErrors(cudaImportExternalSemaphore(&cudaExtCudaUpdateVkSemaphore,
		&externalSemaphoreHandleDesc));

	memset(&externalSemaphoreHandleDesc, 0,
		sizeof(externalSemaphoreHandleDesc));
#ifdef _WIN64
	externalSemaphoreHandleDesc.type =
		IsWindows8OrGreater() ? cudaExternalSemaphoreHandleTypeOpaqueWin32
		: cudaExternalSemaphoreHandleTypeOpaqueWin32Kmt;
	;
	externalSemaphoreHandleDesc.handle.win32.handle = getVkSemaphoreHandle(
		IsWindows8OrGreater()
		? VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
		: VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
		vkUpdateCudaSemaphore);
#else
	externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
	externalSemaphoreHandleDesc.handle.fd = getVkSemaphoreHandle(
		VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT, vkUpdateCudaSemaphore);
#endif
	externalSemaphoreHandleDesc.flags = 0;
	checkCudaErrors(cudaImportExternalSemaphore(&cudaExtVkUpdateCudaSemaphore,
		&externalSemaphoreHandleDesc));
	printf("CUDA Imported Vulkan semaphore\n");
}

void vulkanImageCUDA::cudaVkImportImageMem()
{
	cudaExternalMemoryHandleDesc cudaExtMemHandleDesc;
	memset(&cudaExtMemHandleDesc, 0, sizeof(cudaExtMemHandleDesc));
#ifdef _WIN64
	cudaExtMemHandleDesc.type =
		IsWindows8OrGreater() ? cudaExternalMemoryHandleTypeOpaqueWin32
		: cudaExternalMemoryHandleTypeOpaqueWin32Kmt;
	cudaExtMemHandleDesc.handle.win32.handle = getVkImageMemHandle(
		IsWindows8OrGreater()
		? VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT
		: VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT);
#else
	cudaExtMemHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;

	cudaExtMemHandleDesc.handle.fd =
		getVkImageMemHandle(VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR);
#endif
	cudaExtMemHandleDesc.size = totalImageMemSize;

	checkCudaErrors(cudaImportExternalMemory(&cudaExtMemImageBuffer,
		&cudaExtMemHandleDesc));

	cudaExternalMemoryMipmappedArrayDesc externalMemoryMipmappedArrayDesc;

	memset(&externalMemoryMipmappedArrayDesc, 0,
		sizeof(externalMemoryMipmappedArrayDesc));

	cudaExtent extent = make_cudaExtent(m_imageWidth, m_imageHeight, 0);
	cudaChannelFormatDesc formatDesc;
	formatDesc.x = 8;
	formatDesc.y = 8;
	formatDesc.z = 8;
	formatDesc.w = 8;
	formatDesc.f = cudaChannelFormatKindUnsigned;

	externalMemoryMipmappedArrayDesc.offset = 0;
	externalMemoryMipmappedArrayDesc.formatDesc = formatDesc;
	externalMemoryMipmappedArrayDesc.extent = extent;
	externalMemoryMipmappedArrayDesc.flags = 0;
	externalMemoryMipmappedArrayDesc.numLevels = mipLevels;

	checkCudaErrors(cudaExternalMemoryGetMappedMipmappedArray(
		&cudaMipmappedImageArray, cudaExtMemImageBuffer,
		&externalMemoryMipmappedArrayDesc));

	checkCudaErrors(cudaMallocMipmappedArray(&cudaMipmappedImageArrayTemp,
		&formatDesc, extent, mipLevels));
	checkCudaErrors(cudaMallocMipmappedArray(&cudaMipmappedImageArrayOrig,
		&formatDesc, extent, mipLevels));

	for (int mipLevelIdx = 0; mipLevelIdx < mipLevels; mipLevelIdx++) {
		cudaArray_t cudaMipLevelArray, cudaMipLevelArrayTemp,
			cudaMipLevelArrayOrig;
		cudaResourceDesc resourceDesc;

		checkCudaErrors(cudaGetMipmappedArrayLevel(
			&cudaMipLevelArray, cudaMipmappedImageArray, mipLevelIdx));
		checkCudaErrors(cudaGetMipmappedArrayLevel(
			&cudaMipLevelArrayTemp, cudaMipmappedImageArrayTemp, mipLevelIdx));
		checkCudaErrors(cudaGetMipmappedArrayLevel(
			&cudaMipLevelArrayOrig, cudaMipmappedImageArrayOrig, mipLevelIdx));

		uint32_t width =
			(m_imageWidth >> mipLevelIdx) ? (m_imageWidth >> mipLevelIdx) : 1;
		uint32_t height =
			(m_imageHeight >> mipLevelIdx) ? (m_imageHeight >> mipLevelIdx) : 1;
		checkCudaErrors(cudaMemcpy2DArrayToArray(
			cudaMipLevelArrayOrig, 0, 0, cudaMipLevelArray, 0, 0,
			width * sizeof(uchar4), height, cudaMemcpyDeviceToDevice));

		memset(&resourceDesc, 0, sizeof(resourceDesc));
		resourceDesc.resType = cudaResourceTypeArray;
		resourceDesc.res.array.array = cudaMipLevelArray;

		cudaSurfaceObject_t surfaceObject;
		checkCudaErrors(cudaCreateSurfaceObject(&surfaceObject, &resourceDesc));

		surfaceObjectList.push_back(surfaceObject);

		memset(&resourceDesc, 0, sizeof(resourceDesc));
		resourceDesc.resType = cudaResourceTypeArray;
		resourceDesc.res.array.array = cudaMipLevelArrayTemp;

		cudaSurfaceObject_t surfaceObjectTemp;
		checkCudaErrors(
			cudaCreateSurfaceObject(&surfaceObjectTemp, &resourceDesc));
		surfaceObjectListTemp.push_back(surfaceObjectTemp);
	}

	cudaResourceDesc resDescr;
	memset(&resDescr, 0, sizeof(cudaResourceDesc));

	resDescr.resType = cudaResourceTypeMipmappedArray;
	resDescr.res.mipmap.mipmap = cudaMipmappedImageArrayOrig;

	cudaTextureDesc texDescr;
	memset(&texDescr, 0, sizeof(cudaTextureDesc));

	texDescr.normalizedCoords = true;
	texDescr.filterMode = cudaFilterModeLinear;
	texDescr.mipmapFilterMode = cudaFilterModeLinear;

	texDescr.addressMode[0] = cudaAddressModeWrap;
	texDescr.addressMode[1] = cudaAddressModeWrap;

	texDescr.maxMipmapLevelClamp = float(mipLevels - 1);

	texDescr.readMode = cudaReadModeNormalizedFloat;

	checkCudaErrors(cudaCreateTextureObject(&textureObjMipMapInput, &resDescr,
		&texDescr, NULL));

	checkCudaErrors(cudaMalloc((void**)&d_surfaceObjectList,
		sizeof(cudaSurfaceObject_t) * mipLevels));
	checkCudaErrors(cudaMalloc((void**)&d_surfaceObjectListTemp,
		sizeof(cudaSurfaceObject_t) * mipLevels));

	checkCudaErrors(cudaMemcpy(d_surfaceObjectList, surfaceObjectList.data(),
		sizeof(cudaSurfaceObject_t) * mipLevels,
		cudaMemcpyHostToDevice));
	checkCudaErrors(cudaMemcpy(
		d_surfaceObjectListTemp, surfaceObjectListTemp.data(),
		sizeof(cudaSurfaceObject_t) * mipLevels, cudaMemcpyHostToDevice));

	printf("CUDA Kernel Vulkan image buffer\n");
}

void vulkanImageCUDA::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
		nullptr, 0, nullptr, 1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

void vulkanImageCUDA::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

void vulkanImageCUDA::createVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void vulkanImageCUDA::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void vulkanImageCUDA::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		uniformBuffer, uniformBuffersMemory);
	/*uniformBuffers.resize(swapChainImages.size());
	uniformBuffersMemory.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i], uniformBuffersMemory[i]);
	}*/
}

void vulkanImageCUDA::createDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount =
		static_cast<uint32_t>(swapChainImages.size());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount =
		static_cast<uint32_t>(swapChainImages.size());

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void vulkanImageCUDA::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(),
		descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount =
		static_cast<uint32_t>(swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(swapChainImages.size());
	if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(), 0, nullptr);
	}
}

void vulkanImageCUDA::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkCommandBuffer vulkanImageCUDA::beginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void vulkanImageCUDA::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void vulkanImageCUDA::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

uint32_t vulkanImageCUDA::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) ==
			properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void vulkanImageCUDA::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipeline);

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0,
			VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(commandBuffers[i],
			VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			0, 1, &descriptorSets[i], 0, nullptr);

		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()),
			1, 0, 0, 0);
		// vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1,
		// 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void vulkanImageCUDA::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES);
	renderFinishedSemaphores.resize(MAX_FRAMES);
	inFlightFences.resize(MAX_FRAMES);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES; i++) {
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
			&imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr,
				&renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
			VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create synchronization objects for a frame!");
		}
	}
}

void vulkanImageCUDA::createSyncObjectsExt()
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	memset(&semaphoreInfo, 0, sizeof(semaphoreInfo));
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

#ifdef _WIN64
	WindowsSecurityAttributes winSecurityAttributes;

	VkExportSemaphoreWin32HandleInfoKHR
		vulkanExportSemaphoreWin32HandleInfoKHR = {};
	vulkanExportSemaphoreWin32HandleInfoKHR.sType =
		VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
	vulkanExportSemaphoreWin32HandleInfoKHR.pNext = NULL;
	vulkanExportSemaphoreWin32HandleInfoKHR.pAttributes =
		&winSecurityAttributes;
	vulkanExportSemaphoreWin32HandleInfoKHR.dwAccess =
		DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
	vulkanExportSemaphoreWin32HandleInfoKHR.name = (LPCWSTR)NULL;
#endif
	VkExportSemaphoreCreateInfoKHR vulkanExportSemaphoreCreateInfo = {};
	vulkanExportSemaphoreCreateInfo.sType =
		VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHR;
#ifdef _WIN64
	vulkanExportSemaphoreCreateInfo.pNext =
		IsWindows8OrGreater() ? &vulkanExportSemaphoreWin32HandleInfoKHR : NULL;
	vulkanExportSemaphoreCreateInfo.handleTypes =
		IsWindows8OrGreater()
		? VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT
		: VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
#else
	vulkanExportSemaphoreCreateInfo.pNext = NULL;
	vulkanExportSemaphoreCreateInfo.handleTypes =
		VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
	semaphoreInfo.pNext = &vulkanExportSemaphoreCreateInfo;

	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
		&cudaUpdateVkSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr,
			&vkUpdateCudaSemaphore) != VK_SUCCESS) {
		throw std::runtime_error(
			"failed to create synchronization objects for a CUDA-Vulkan!");
	}

}

void vulkanImageCUDA::updateUniformBuffer()
{
	UniformBufferObject ubo = {};

	mat4x4_identity(ubo.model);
	mat4x4 Model;
	mat4x4_dup(Model, ubo.model);
	mat4x4_rotate(ubo.model, Model, 0.0f, 1.0f, 1.0f, degreesToRadians(0.0f));

	vec3 eye = { 0.0f, 0.0f, 2.0f };
	vec3 center = { 0.0f, 0.0f, 0.0f };
	vec3 up = { 0.0f, 1.0f, 0.0f };
	mat4x4_look_at(ubo.view, eye, center, up);

	mat4x4_perspective(ubo.proj, degreesToRadians(45.0f),
		swapChainExtent.width / (float)swapChainExtent.height,
		0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(device, uniformBuffersMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBuffersMemory);

	/*for (size_t i = 0; i < swapChainImages.size(); i++) {
		void* data;
		vkMapMemory(device, uniformBuffersMemory[i], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, uniformBuffersMemory[i]);
	}*/
}

void vulkanImageCUDA::cudaVkSemaphoreSignal(cudaExternalSemaphore_t & extSemaphore)
{
	cudaExternalSemaphoreSignalParams extSemaphoreSignalParams;
	memset(&extSemaphoreSignalParams, 0, sizeof(extSemaphoreSignalParams));

	extSemaphoreSignalParams.params.fence.value = 0;
	extSemaphoreSignalParams.flags = 0;
	checkCudaErrors(cudaSignalExternalSemaphoresAsync(
		&extSemaphore, &extSemaphoreSignalParams, 1, streamToRun));
}

void vulkanImageCUDA::cudaVkSemaphoreWait(cudaExternalSemaphore_t & extSemaphore)
{
	cudaExternalSemaphoreWaitParams extSemaphoreWaitParams;

	memset(&extSemaphoreWaitParams, 0, sizeof(extSemaphoreWaitParams));

	extSemaphoreWaitParams.params.fence.value = 0;
	extSemaphoreWaitParams.flags = 0;

	checkCudaErrors(cudaWaitExternalSemaphoresAsync(
		&extSemaphore, &extSemaphoreWaitParams, 1, streamToRun));
}

QueueFamilyIndices vulkanImageCUDA::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
		nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
		queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueCount > 0 &&
			queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

std::vector<const char*> vulkanImageCUDA::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions,
		glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool vulkanImageCUDA::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<char> vulkanImageCUDA::readFile(const std::string & filename)
{
	char* file_path = sdkFindFilePath(filename.c_str(), execution_path.c_str());
	std::ifstream file(file_path, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}