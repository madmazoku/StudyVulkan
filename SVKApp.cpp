#include "SVKApp.h"

template<class T>
std::string JoinStrings(T strings) {
	std::stringstream ss;
	auto it = strings.cbegin();
	if (it != strings.cend()) {
		ss << '\'' << *it;
		while (++it != strings.cend())
			ss << "', '" << *it;
		ss << '\'';
	}
	return ss.str();
}

static std::vector<char> ReadFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::stringstream ss;
		ss << "Failed to open file: '" << filename << '\'';
		throw std::runtime_error(ss.str());
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

// *********************************************************************************

const int SVKApp::g_width = 1024;
const int SVKApp::g_height = 1024;
const char* const SVKApp::g_appName = "Vulkan";
const char* const SVKApp::g_engineName = "HelloWorld";
const std::vector<const char*> SVKApp::g_validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> SVKApp::g_deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool SVKApp::g_enableValidationLayers = true;
#else
const bool SVKApp::g_enableValidationLayers = false;
#endif // _DEBUG

const int SVKApp::g_maxFramesInFlight = 2;

const std::vector<SVKApp::Vertex> SVKApp::g_vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> SVKApp::g_indices = {
	0, 1, 2, 2, 3, 0
};

// *********************************************************************************

VkResult SVKApp::CreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void SVKApp::DestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL SVKApp::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	return reinterpret_cast<SVKApp*>(pUserData)->OnDebug(messageSeverity, messageType, pCallbackData) ? VK_TRUE : VK_FALSE;
}

void SVKApp::ResizeCallback(GLFWwindow* window, int width, int height) {
	reinterpret_cast<SVKApp*>(glfwGetWindowUserPointer(window))->OnResize(width, height);
}

VkVertexInputBindingDescription SVKApp::Vertex::GetBindingDescription() {
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> SVKApp::Vertex::GetAttributeDescriptions() {
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	return attributeDescriptions;
}

// *********************************************************************************

SVKApp::SVKApp(const SVKConfig& config) :
	m_config(config),
	m_window(nullptr),
	m_instance(VK_NULL_HANDLE),
	m_debugMessenger(VK_NULL_HANDLE),
	m_windowSurface(VK_NULL_HANDLE),
	m_physicalDevice(VK_NULL_HANDLE),
	m_logicalDevice(VK_NULL_HANDLE),
	m_graphicsQueue(VK_NULL_HANDLE),
	m_presentQueue(VK_NULL_HANDLE),
	m_swapChain(VK_NULL_HANDLE),
	m_swapChainImageFormat(VK_FORMAT_UNDEFINED),
	m_swapChainExtent{ 0, 0 },
	m_renderPass(VK_NULL_HANDLE),
	m_descriptorSetLayout(VK_NULL_HANDLE),
	m_pipelineLayout(VK_NULL_HANDLE),
	m_graphicsPipeline(VK_NULL_HANDLE),
	m_commandPool(VK_NULL_HANDLE),
	m_textureImage(VK_NULL_HANDLE),
	m_textureImageMemory(VK_NULL_HANDLE),
	m_vertexBuffer(VK_NULL_HANDLE),
	m_vertexBufferMemory(VK_NULL_HANDLE),
	m_indexBuffer(VK_NULL_HANDLE),
	m_indexBufferMemory(VK_NULL_HANDLE),
	m_descriptorPool(VK_NULL_HANDLE),
	m_currentFrame(0),
	m_framebufferResized(false)
{
}

void SVKApp::Initialize() {
	InitializeWindow();
	InitializeVulkan();
}

void SVKApp::Cleanup() {
	CleanupVulkan();
	CleanupWindow();
}

void SVKApp::Run() {
	auto startTm = std::chrono::high_resolution_clock::now();
	auto prevTm = startTm;
	uint32_t frameCount = 0;
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		if (m_framebufferResized) {
			RecreateSwapChain();
			m_framebufferResized = false;
		}
		try {
			++frameCount;
			DrawFrame();
		}
		catch (const VkException& e) {
			if (e.result() == VK_ERROR_OUT_OF_DATE_KHR || e.result() == VK_SUBOPTIMAL_KHR)
				RecreateSwapChain();
			else
				throw;
		}
		auto currTm = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diffPrevTm = currTm - prevTm;
		if (diffPrevTm.count() > 1.0f) {
			std::chrono::duration<double> diffStartTm = currTm - startTm;
			std::stringstream ss;
			ss << g_appName << " [" << int(diffStartTm.count()) << "] FPS: " << (frameCount / diffPrevTm.count());
			glfwSetWindowTitle(m_window, ss.str().c_str());
			prevTm = currTm;
			frameCount = 0;
		}
	}
	vkDeviceWaitIdle(m_logicalDevice);
}

void SVKApp::InitializeWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(g_width, g_height, g_appName, nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, ResizeCallback);

	glfwSetWindowPos(m_window, 1024, 100);
}

void SVKApp::InitializeVulkan() {
	CreateInstance();
	if (g_enableValidationLayers)
		CreateDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFrameBuffers();
	CreateCommandPool();
	CreateTextureImage();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
	CreateSyncObjects();
}

void SVKApp::CreateInstance() {
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = g_appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = g_engineName;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = GetRequiredExtensions();
	CheckRequiredExtensionsSupport(extensions);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	createInfo.enabledLayerCount = 0;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (g_enableValidationLayers) {
		CheckValidationLayerSupport(g_validationLayers);
		createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
		createInfo.ppEnabledLayerNames = g_validationLayers.data();

		FillDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = &debugCreateInfo;
	}
	else
		createInfo.enabledLayerCount = 0;

	vkCheckResult(vkCreateInstance(&createInfo, nullptr, &m_instance), "Create Instance");
}

void SVKApp::CheckValidationLayerSupport(const std::vector<const char*> validationLayers) {
	// Dump
	std::cerr << "Layers required: " << validationLayers.size() << std::endl;
	for (const char* layerName : validationLayers)
		std::cerr << '\t' << layerName << std::endl;

	uint32_t vkAvaliableLayerCount;
	vkEnumerateInstanceLayerProperties(&vkAvaliableLayerCount, nullptr);
	std::vector<VkLayerProperties> vkAvaliableLayers(vkAvaliableLayerCount);
	vkEnumerateInstanceLayerProperties(&vkAvaliableLayerCount, vkAvaliableLayers.data());

	std::cerr << "Layers allowed: " << vkAvaliableLayerCount << std::endl;
	for (const VkLayerProperties& vkAvaliableLayer : vkAvaliableLayers)
		std::cerr << '\t' << vkAvaliableLayer.layerName << ": " << vkAvaliableLayer.layerName << std::endl;

	// Check
	std::set<std::string> requiredValidationLayers(validationLayers.begin(), validationLayers.end());
	for (const VkLayerProperties& vkAvailableLayer : vkAvaliableLayers)
		requiredValidationLayers.erase(vkAvailableLayer.layerName);

	if (!requiredValidationLayers.empty()) {
		std::stringstream ss;
		ss << "Required validation layers not found: " << JoinStrings(requiredValidationLayers);
		throw std::runtime_error(ss.str());
	}
}

std::vector<const char*> SVKApp::GetRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (g_enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

void SVKApp::CheckRequiredExtensionsSupport(const std::vector<const char*> extensions) {
	// Dump
	std::cerr << "Extensions required: " << extensions.size() << std::endl;
	for (const char* extensionName : extensions)
		std::cerr << '\t' << extensionName << std::endl;

	uint32_t vkAvailableExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &vkAvailableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> vkAvailableExtensions(vkAvailableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &vkAvailableExtensionCount, vkAvailableExtensions.data());

	std::cerr << "Extensions allowed: " << vkAvailableExtensionCount << std::endl;
	for (const VkExtensionProperties& vkAvailableExtension : vkAvailableExtensions)
		std::cerr << '\t' << vkAvailableExtension.extensionName << std::endl;

	// Check
	std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
	for (const VkExtensionProperties& vkAvailableExtension : vkAvailableExtensions)
		requiredExtensions.erase(vkAvailableExtension.extensionName);

	if (!requiredExtensions.empty()) {
		std::stringstream ss;
		ss << "Required instance extensions not found: " << JoinStrings(requiredExtensions);
		throw std::runtime_error(ss.str());
	}
}

void SVKApp::FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = this;
}

void SVKApp::CreateDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	FillDebugMessengerCreateInfo(createInfo);
	vkCheckResult(CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger), "Create DebugMessenger");
}

void SVKApp::CreateSurface() {
	vkCheckResult(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_windowSurface), "Create Surface");
}

void SVKApp::PickPhysicalDevice() {
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
	if (physicalDeviceCount == 0)
		throw std::runtime_error("Physical device with Vulkan support not found");
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

	std::cerr << "physical devices: " << physicalDeviceCount << std::endl;
	for (VkPhysicalDevice& physicalDevice : physicalDevices) {
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		std::cerr << '\t' << physicalDeviceProperties.deviceName << " (";
		if (IsPhysicalDeviceSuitable(physicalDevice))
			std::cerr << GetPhysicalDeviceScore(physicalDevice);
		else
			std::cerr << "not suitable";
		std::cerr << ')' << std::endl;
	}

	std::optional<std::pair<int, VkPhysicalDevice>> physicalDeviceCandidate;
	for (VkPhysicalDevice& physicalDevice : physicalDevices)
		if (IsPhysicalDeviceSuitable(physicalDevice)) {
			int score = GetPhysicalDeviceScore(physicalDevice);
			if (!physicalDeviceCandidate.has_value() || physicalDeviceCandidate.value().first < score)
				physicalDeviceCandidate = std::make_pair(score, physicalDevice);
		}

	if (!physicalDeviceCandidate.has_value())
		throw std::runtime_error("No suitable physical device found");

	m_physicalDevice = physicalDeviceCandidate.value().second;

	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);
	std::cerr << "selected: " << physicalDeviceProperties.deviceName << std::endl;
}

bool SVKApp::IsPhysicalDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char*> extensions) {
	uint32_t vkAvailableExtensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &vkAvailableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> vkAvailableExtensions(vkAvailableExtensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &vkAvailableExtensionCount, vkAvailableExtensions.data());

	std::set<std::string> requiredExtensions(extensions.begin(), extensions.end());
	for (const VkExtensionProperties& vkAvailableExtension : vkAvailableExtensions)
		requiredExtensions.erase(vkAvailableExtension.extensionName);

	return requiredExtensions.empty();
}

bool SVKApp::IsPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice) {
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	if (physicalDeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		return false;

	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
	if (!physicalDeviceFeatures.geometryShader)
		return false;

	QueueFamilyIndices queueFamilyIndices = FindQueueFamilyIndices(physicalDevice);
	if (!queueFamilyIndices.IsComplete())
		return false;

	if (!IsPhysicalDeviceExtensionSupport(physicalDevice, g_deviceExtensions))
		return false;

	SwapChainSupportDetails swapChainSupportDetails = FindSwapChainSupportDetails(physicalDevice);
	if (swapChainSupportDetails.formats.empty())
		return false;
	if (swapChainSupportDetails.presentModes.empty())
		return false;

	return true;
}

int SVKApp::GetPhysicalDeviceScore(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

	int score = 0;

	score += physicalDeviceProperties.limits.maxImageDimension2D;

	return score;
}

SVKApp::QueueFamilyIndices SVKApp::FindQueueFamilyIndices(VkPhysicalDevice physicalDevice) {
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	QueueFamilyIndices queueFamilyIndices;
	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		VkQueueFamilyProperties& queueFamily = queueFamilies[i];
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			queueFamilyIndices.graphicsFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_windowSurface, &presentSupport);
		if (presentSupport)
			queueFamilyIndices.presentFamily = i;

		if (queueFamilyIndices.IsComplete())
			break;
	}

	return queueFamilyIndices;
}

SVKApp::SwapChainSupportDetails SVKApp::FindSwapChainSupportDetails(VkPhysicalDevice physicalDevice) {
	SwapChainSupportDetails swapChainSupportDetails{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_windowSurface, &swapChainSupportDetails.capabilities);

	uint32_t vkAvailableFormatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_windowSurface, &vkAvailableFormatCount, nullptr);
	if (vkAvailableFormatCount != 0) {
		swapChainSupportDetails.formats.resize(vkAvailableFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_windowSurface, &vkAvailableFormatCount, swapChainSupportDetails.formats.data());
	}

	uint32_t vkAvailablePresentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_windowSurface, &vkAvailablePresentModeCount, nullptr);
	if (vkAvailablePresentModeCount != 0) {
		swapChainSupportDetails.presentModes.resize(vkAvailablePresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_windowSurface, &vkAvailablePresentModeCount, swapChainSupportDetails.presentModes.data());
	}

	return swapChainSupportDetails;
}

void SVKApp::CreateLogicalDevice() {
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilyIndices(m_physicalDevice);
	std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };
	float queuePriority = 1.0;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures physicalDeviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &physicalDeviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(g_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();

	if (g_enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
		createInfo.ppEnabledLayerNames = g_validationLayers.data();
	}
	else
		createInfo.enabledLayerCount = 0;

	vkCheckResult(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice), "Create Device");

	vkGetDeviceQueue(m_logicalDevice, queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
}

void SVKApp::CreateSwapChain() {
	SwapChainSupportDetails swapChainSupportDetails = FindSwapChainSupportDetails(m_physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupportDetails.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupportDetails.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupportDetails.capabilities);

	uint32_t maxImageCount = swapChainSupportDetails.capabilities.maxImageCount;
	uint32_t minImageCount = swapChainSupportDetails.capabilities.minImageCount + 1;
	if (maxImageCount > 0 && minImageCount > maxImageCount)
		minImageCount = maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_windowSurface;
	createInfo.minImageCount = minImageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilyIndices(m_physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

	createInfo.preTransform = swapChainSupportDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	vkCheckResult(vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapChain), "Create SwapChain");

	uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;
}

VkSurfaceFormatKHR SVKApp::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return availableFormat;
	return availableFormats[0];
}

VkPresentModeKHR SVKApp::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes)
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			return availablePresentMode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SVKApp::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;

	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

void SVKApp::CreateImageViews() {
	m_swapChainImageViews.resize(m_swapChainImages.size());
	for (int i = 0; i < m_swapChainImages.size(); ++i) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		vkCheckResult(vkCreateImageView(m_logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[i]), "Create ImageView");
	}
}

void SVKApp::CreateRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	vkCheckResult(vkCreateRenderPass(m_logicalDevice, &renderPassInfo, nullptr, &m_renderPass), "Create RenderPass");
}

void SVKApp::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	vkCheckResult(vkCreateDescriptorSetLayout(m_logicalDevice, &layoutInfo, nullptr, &m_descriptorSetLayout), "Create DescriptorSetLayout");
}

void SVKApp::CreateGraphicsPipeline() {
	std::vector<char> shaderVert = ReadFile((m_config.m_appDir / "shader.vert.spv").string());
	std::vector<char> shaderFrag = ReadFile((m_config.m_appDir / "./shader.frag.spv").string());

	VkShaderModule shaderVertModule = CreateShaderModule(shaderVert);
	VkShaderModule shaderFragModule = CreateShaderModule(shaderFrag);

	VkPipelineShaderStageCreateInfo shaderVertStageInfo{};
	shaderVertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderVertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderVertStageInfo.module = shaderVertModule;
	shaderVertStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderFragStageInfo{};
	shaderFragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderFragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderFragStageInfo.module = shaderFragModule;
	shaderFragStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { shaderVertStageInfo, shaderFragStageInfo };

	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapChainExtent.width);
	viewport.height = static_cast<float>(m_swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	vkCheckResult(vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout), "Create PipelineLayout");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	vkCheckResult(vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline), "Create GraphicsPipeline");

	vkDestroyShaderModule(m_logicalDevice, shaderVertModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, shaderFragModule, nullptr);
}

VkShaderModule SVKApp::CreateShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	vkCheckResult(vkCreateShaderModule(m_logicalDevice, &createInfo, nullptr, &shaderModule), "Create ShaderModule");
	return shaderModule;
}

void SVKApp::CreateFrameBuffers() {
	m_swapChainFrameBuffers.resize(m_swapChainImageViews.size());

	for (size_t i = 0; i < m_swapChainImageViews.size(); ++i) {
		VkImageView attachments[] = { m_swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_swapChainExtent.width;
		framebufferInfo.height = m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		vkCheckResult(vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_swapChainFrameBuffers[i]), "Create SwapChain FrameBuffer");
	}
}

void SVKApp::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilyIndices(m_physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0; // Optional

	vkCheckResult(vkCreateCommandPool(m_logicalDevice, &poolInfo, nullptr, &m_commandPool), "Create CommandPool");
}

void SVKApp::CreateTextureImage() {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load((m_config.m_appDir / "texture.jpg").string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = static_cast<long>(texWidth) * static_cast<long>(texHeight) * 4;

	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(
		imageSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, 
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	CreateImage(
		texWidth, 
		texHeight, 
		VK_FORMAT_R8G8B8A8_SRGB, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		m_textureImage, 
		m_textureImageMemory
	);

	TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void SVKApp::CreateImage(
	uint32_t width, 
	uint32_t height, 
	VkFormat format, 
	VkImageTiling tiling, 
	VkImageUsageFlags usage, 
	VkMemoryPropertyFlags properties, 
	VkImage& image, 
	VkDeviceMemory& imageMemory
) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCheckResult(vkCreateImage(m_logicalDevice, &imageInfo, nullptr, &image), "Create Image");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_logicalDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	vkCheckResult(vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &imageMemory), "Allocate Texture Memory");
	vkCheckResult(vkBindImageMemory(m_logicalDevice, image, imageMemory, 0), "Bind Image Memory");
}

void SVKApp::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
		throw std::runtime_error("unsupported layout transition!");

	vkCmdPipelineBarrier(commandBuffer, sourceStage,  destinationStage, 0, 0, nullptr, 0, nullptr, 1,  &barrier);

	EndSingleTimeCommands(commandBuffer);
}

void SVKApp::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(commandBuffer);
}

void SVKApp::CreateVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(g_vertices[0]) * g_vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, 
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, g_vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

	CreateBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		m_vertexBuffer, 
		m_vertexBufferMemory
	);

	CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void SVKApp::CreateIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(g_indices[0]) * g_indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, 
		stagingBufferMemory
	);

	void* data;
	vkMapMemory(m_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, g_indices.data(), (size_t)bufferSize);
	vkUnmapMemory(m_logicalDevice, stagingBufferMemory);

	CreateBuffer(
		bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		m_indexBuffer, 
		m_indexBufferMemory
	);

	CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(m_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(m_logicalDevice, stagingBufferMemory, nullptr);
}

void SVKApp::CreateUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	m_uniformBuffers.resize(m_swapChainImages.size());
	m_uniformBuffersMemory.resize(m_swapChainImages.size());

	for (size_t i = 0; i < m_swapChainImages.size(); ++i)
		CreateBuffer(
			bufferSize, 
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			m_uniformBuffers[i], 
			m_uniformBuffersMemory[i]
		);
}

void SVKApp::CreateBuffer(
	VkDeviceSize size, 
	VkBufferUsageFlags usage, 
	VkMemoryPropertyFlags properties, 
	VkBuffer& buffer, 
	VkDeviceMemory& bufferMemory
) {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCheckResult(vkCreateBuffer(m_logicalDevice, &bufferInfo, nullptr, &buffer), "Create Buffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_logicalDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	vkCheckResult(vkAllocateMemory(m_logicalDevice, &allocInfo, nullptr, &bufferMemory), "Allocate Memory");
	vkCheckResult(vkBindBufferMemory(m_logicalDevice, buffer, bufferMemory, 0), "Bind Memory To Buffer");
}

void SVKApp::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

uint32_t SVKApp::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;

	throw std::runtime_error("No suitable memory type found");
}

void SVKApp::CreateDescriptorPool() {
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(m_swapChainImages.size());

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(m_swapChainImages.size());

	vkCheckResult(vkCreateDescriptorPool(m_logicalDevice, &poolInfo, nullptr, &m_descriptorPool), "Create DescriptorPool");
}

void SVKApp::CreateDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(m_swapChainImages.size(), m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(m_swapChainImages.size());
	vkCheckResult(vkAllocateDescriptorSets(m_logicalDevice, &allocInfo, m_descriptorSets.data()), "Allocate DescriptorSets");

	for (size_t i = 0; i < m_swapChainImages.size(); ++i) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional	

		vkUpdateDescriptorSets(m_logicalDevice, 1, &descriptorWrite, 0, nullptr);
	}
}

void SVKApp::CreateCommandBuffers() {
	m_commandBuffers.resize(m_swapChainFrameBuffers.size());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	vkCheckResult(vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_commandBuffers.data()), "Allocate CommandBuffers");

	for (size_t i = 0; i < m_commandBuffers.size(); ++i) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vkCheckResult(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo), "Begin Command Sequence");

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFrameBuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainExtent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
		vkCmdBindVertexBuffers(m_commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(m_commandBuffers[i], m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);
		vkCmdDrawIndexed(m_commandBuffers[i], static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);
		vkCmdEndRenderPass(m_commandBuffers[i]);

		vkCheckResult(vkEndCommandBuffer(m_commandBuffers[i]), "End Command Sequence");
	}
}

void SVKApp::CreateSyncObjects() {
	m_imageAvailableSemaphores.resize(g_maxFramesInFlight);
	m_renderFinishedSemaphores.resize(g_maxFramesInFlight);
	m_inFlightFences.resize(g_maxFramesInFlight);
	m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < g_maxFramesInFlight; ++i) {
		vkCheckResult(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]), "Create ImageAvailable Semaphore");
		vkCheckResult(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]), "Create RenderFinished Semaphore");
		vkCheckResult(vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i]), "Create InFlight Fence");
	}
}

void SVKApp::CleanupWindow() {
	glfwDestroyWindow(m_window);
	m_window = nullptr;

	glfwTerminate();
}

void SVKApp::CleanupVulkan() {
	CleanupSwapChain();

	vkDestroyDescriptorSetLayout(m_logicalDevice, m_descriptorSetLayout, nullptr);
	m_descriptorSetLayout = VK_NULL_HANDLE;

	m_currentFrame = 0;
	m_imagesInFlight.clear();
	for (int i = 0; i < g_maxFramesInFlight; ++i) {
		vkDestroyFence(m_logicalDevice, m_inFlightFences[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphores[i], nullptr);
	}
	m_inFlightFences.clear();
	m_renderFinishedSemaphores.clear();
	m_imageAvailableSemaphores.clear();

	vkDestroyBuffer(m_logicalDevice, m_indexBuffer, nullptr);
	m_indexBuffer = VK_NULL_HANDLE;
	vkFreeMemory(m_logicalDevice, m_indexBufferMemory, nullptr);
	m_indexBufferMemory = VK_NULL_HANDLE;

	vkDestroyBuffer(m_logicalDevice, m_vertexBuffer, nullptr);
	m_vertexBuffer = VK_NULL_HANDLE;
	vkFreeMemory(m_logicalDevice, m_vertexBufferMemory, nullptr);
	m_vertexBufferMemory = VK_NULL_HANDLE;

	vkDestroyImage(m_logicalDevice, m_textureImage, nullptr);
	m_textureImage = VK_NULL_HANDLE;
	vkFreeMemory(m_logicalDevice, m_textureImageMemory, nullptr);
	m_textureImageMemory = VK_NULL_HANDLE;

	vkDestroyCommandPool(m_logicalDevice, m_commandPool, nullptr);
	m_commandPool = VK_NULL_HANDLE;

	m_presentQueue = VK_NULL_HANDLE;
	m_graphicsQueue = VK_NULL_HANDLE;
	vkDestroyDevice(m_logicalDevice, nullptr);
	m_logicalDevice = VK_NULL_HANDLE;

	m_physicalDevice = VK_NULL_HANDLE;
	vkDestroySurfaceKHR(m_instance, m_windowSurface, nullptr);
	m_windowSurface = VK_NULL_HANDLE;

	if (g_enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		m_debugMessenger = VK_NULL_HANDLE;
	}

	vkDestroyInstance(m_instance, nullptr);
	m_instance = VK_NULL_HANDLE;
}

void SVKApp::CleanupSwapChain() {
	for (const VkFramebuffer& frameBuffer : m_swapChainFrameBuffers)
		vkDestroyFramebuffer(m_logicalDevice, frameBuffer, nullptr);
	m_swapChainFrameBuffers.clear();

	vkDestroyDescriptorPool(m_logicalDevice, m_descriptorPool, nullptr);
	m_descriptorPool = VK_NULL_HANDLE;

	for (size_t i = 0; i < m_swapChainImages.size(); ++i) {
		vkDestroyBuffer(m_logicalDevice, m_uniformBuffers[i], nullptr);
		vkFreeMemory(m_logicalDevice, m_uniformBuffersMemory[i], nullptr);
	}
	m_uniformBuffers.clear();
	m_uniformBuffersMemory.clear();

	vkFreeCommandBuffers(m_logicalDevice, m_commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

	vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
	m_graphicsPipeline = VK_NULL_HANDLE;

	vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
	m_pipelineLayout = VK_NULL_HANDLE;

	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);
	m_renderPass = VK_NULL_HANDLE;

	for (const VkImageView& view : m_swapChainImageViews)
		vkDestroyImageView(m_logicalDevice, view, nullptr);
	m_swapChainImageViews.clear();

	m_swapChainExtent = { 0, 0 };
	m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
	m_swapChainImages.clear();
	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
	m_swapChain = VK_NULL_HANDLE;
}

void SVKApp::RecreateSwapChain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0) {
		std::cerr << "Window Minimized" << std::endl;
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_logicalDevice);

	std::cerr << "RecreateSwapChain" << std::endl;

	CleanupSwapChain();

	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffers();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffers();
}

void SVKApp::DrawFrame() {
	vkCheckResult(vkWaitForFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX), "InFlight Fence Wait");

	uint32_t imageIndex;
	vkCheckResult(vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex), "Acquire Next Image");

	if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
		vkCheckResult(vkWaitForFences(m_logicalDevice, 1, &m_imagesInFlight[m_currentFrame], VK_TRUE, UINT64_MAX), "InFlight Fence Wait");
	m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];

	UpdateUniformBuffer(imageIndex);

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkCheckResult(vkResetFences(m_logicalDevice, 1, &m_inFlightFences[m_currentFrame]), "inFlight Fence Reset");
	vkCheckResult(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]), "Queue Submit");

	VkSwapchainKHR swapChains[] = { m_swapChain };

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	vkCheckResult(vkQueuePresentKHR(m_presentQueue, &presentInfo), "Queue Present");
	vkCheckResult(vkQueueWaitIdle(m_presentQueue), "Queue Present Wait");

	m_currentFrame = (m_currentFrame + 1) % g_maxFramesInFlight;
}

void SVKApp::UpdateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), m_swapChainExtent.width / (float)m_swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(m_logicalDevice, m_uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_logicalDevice, m_uniformBuffersMemory[currentImage]);
}

VkCommandBuffer SVKApp::BeginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void SVKApp::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphicsQueue);

	vkFreeCommandBuffers(m_logicalDevice, m_commandPool, 1, &commandBuffer);
}


bool SVKApp::OnDebug(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return false;
}

void SVKApp::OnResize(int width, int height)
{
	m_framebufferResized = true;
}
