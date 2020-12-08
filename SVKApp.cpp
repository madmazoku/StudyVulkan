#include "SVKApp.h"

const int SVKApp::g_width = 1024;
const int SVKApp::g_height = 512;
const char* const SVKApp::g_appName = "Vulkan";
const char* const SVKApp::g_engineName = "HelloWorld";
const std::vector<const char*> SVKApp::g_validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool SVKApp::g_enableVkValidationLayers = true;
#else
const bool SVKApp::g_enableVkValidationLayers = false;
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

SVKApp::SVKApp() : m_window(nullptr), m_instance(0), m_debugMessenger(0)
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
	if (g_enableVkValidationLayers)
		CreateDebugMessenger();
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
	if (g_enableVkValidationLayers) {
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

	uint32_t vkLayerCount;
	vkEnumerateInstanceLayerProperties(&vkLayerCount, nullptr);
	std::vector<VkLayerProperties> vkLayers(vkLayerCount);
	vkEnumerateInstanceLayerProperties(&vkLayerCount, vkLayers.data());

	std::cerr << "Layers allowed: " << vkLayerCount << std::endl;
	for (const VkLayerProperties& vkLayer : vkLayers)
		std::cerr << '\t' << vkLayer.layerName << ": " << vkLayer.layerName << std::endl;

	// Check
	for (const char* layerName : validationLayers) {
		bool layerFound = false;
		for (const VkLayerProperties& vkAvailableLayer : vkLayers)
			if (strcmp(layerName, vkAvailableLayer.layerName) == 0) {
				layerFound = true;
				break;
			}

		if (!layerFound) {
			std::stringstream ss;
			ss << "Required validation layer '" << layerName << "' not found";
			throw std::runtime_error(ss.str());
		}
	}
}

std::vector<const char*> SVKApp::GetRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (g_enableVkValidationLayers)
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
	for (const char* extensionName : extensions) {
		bool extensionFound = false;
		for (const VkExtensionProperties& vkAvailableExtension : vkAvailableExtensions)
			if (strcmp(extensionName, vkAvailableExtension.extensionName) == 0) {
				extensionFound = true;
				break;
			}

		if (!extensionFound) {
			std::stringstream ss;
			ss << "Required extension '" << extensionName << "' not found";
			throw std::runtime_error(ss.str());
		}
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

void SVKApp::CleanupWindow() {
	glfwDestroyWindow(m_window);
	m_window = nullptr;

	glfwTerminate();
}

void SVKApp::CleanupVulkan() {
	if (g_enableVkValidationLayers) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
		m_debugMessenger = 0;
	}

	vkDestroyInstance(m_instance, nullptr);
	m_instance = 0;
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
