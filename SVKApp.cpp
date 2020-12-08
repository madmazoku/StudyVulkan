#include "SVKApp.h"

const int SVKApp::g_width = 1024;
const int SVKApp::g_height = 512;
const char* const SVKApp::g_appName = "Vulkan";
const char* const SVKApp::g_engineName = "HelloWorld";

SVKApp::SVKApp(): m_window(nullptr)
{
}

void SVKApp::Initialize() {
	InitializeWindow();
	InitializeVulkan();
}

void SVKApp::Cleanup() {
	CleanupWindow();
	CleanupVulkan();
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
}

void SVKApp::InitializeVulkan() {
	CreateInstance();
}

void SVKApp::CreateInstance() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::cerr << "glfw Extensions need: " << glfwExtensionCount << std::endl;
	for (uint32_t i = 0; i < glfwExtensionCount; ++i)
		std::cerr << '\t' << glfwExtensions[i] << std::endl;

	uint32_t vkExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
	std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());

	std::cerr << "vk Extensions allowed: " << vkExtensionCount << std::endl;
	for (uint32_t i = 0; i < vkExtensionCount; ++i)
		std::cerr << '\t' << vkExtensions[i].extensionName << std::endl;

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

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	createInfo.enabledLayerCount = 0;

	vkCheckResult(vkCreateInstance(&createInfo, nullptr, &m_instance), "Create Instance");
}

void SVKApp::CleanupWindow() {
	glfwDestroyWindow(m_window);

	glfwTerminate();
}

void SVKApp::CleanupVulkan() {
	vkDestroyInstance(m_instance, nullptr);
}

