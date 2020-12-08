#pragma once

#include "common.h"

class SVKApp
{
protected:
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		bool IsComplete() {
			return graphicsFamily.has_value();
		}
	};

public:
	static const int g_width;
	static const int g_height;
	static const char* const g_appName;
	static const char* const g_engineName;
	static const bool g_enableVkValidationLayers;
	static const std::vector<const char*> g_validationLayers;

private:
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	static VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

public:
	SVKApp();

	void Initialize();
	void Cleanup();
	void Run();

protected:
	void InitializeWindow();
	void InitializeVulkan();

	void CreateInstance();
	void CheckValidationLayerSupport(const std::vector<const char*> validationLayers);
	std::vector<const char*> GetRequiredExtensions();
	void CheckRequiredExtensionsSupport(const std::vector<const char*> extensions);

	void FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void CreateDebugMessenger();

	void CreatePhysicalDevice();
	bool IsPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice);
	int GetPhysicalDeviceScore(VkPhysicalDevice physicalDevice);
	QueueFamilyIndices FindQueueFamilyIndices(VkPhysicalDevice physicalDevice);

	void CleanupVulkan();
	void CleanupWindow();

	bool DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

protected:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice;
	QueueFamilyIndices m_queueFamilyIndices;
};

