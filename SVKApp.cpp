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

const int SVKApp::g_width = 1024;
const int SVKApp::g_height = 512;
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

	return static_cast<SVKApp*>(pUserData)->DebugCallback(messageSeverity, messageType, pCallbackData) ? VK_TRUE : VK_FALSE;
}

SVKApp::SVKApp() :
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
	m_swapChainExtent{ 0, 0 }
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
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
	}
}

void SVKApp::InitializeWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(g_width, g_height, g_appName, nullptr, nullptr);

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

void SVKApp::CleanupWindow() {
	glfwDestroyWindow(m_window);
	m_window = nullptr;

	glfwTerminate();
}

void SVKApp::CleanupVulkan() {
	for (const VkImageView& view : m_swapChainImageViews)
		vkDestroyImageView(m_logicalDevice, view, nullptr);
	m_swapChainImageViews.clear();

	m_swapChainExtent = { 0, 0 };
	m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
	m_swapChainImages.clear();
	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
	m_swapChain = VK_NULL_HANDLE;

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

bool SVKApp::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return false;
}
