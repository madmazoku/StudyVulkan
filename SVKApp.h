#pragma once

#include "common.h"

#include "SVKConfig.h"

class SVKApp
{
protected:
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool IsComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

public:
	static const int g_width;
	static const int g_height;
	static const char* const g_appName;
	static const char* const g_engineName;
	static const bool g_enableValidationLayers;
	static const std::vector<const char*> g_validationLayers;
	static const std::vector<const char*> g_deviceExtensions;

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
	SVKApp(const SVKConfig& config);

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

	void CreateSurface();

	void PickPhysicalDevice();
	bool IsPhysicalDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<const char*> extensions);
	bool IsPhysicalDeviceSuitable(VkPhysicalDevice physicalDevice);
	int GetPhysicalDeviceScore(VkPhysicalDevice physicalDevice);
	QueueFamilyIndices FindQueueFamilyIndices(VkPhysicalDevice physicalDevice);
	SwapChainSupportDetails FindSwapChainSupportDetails(VkPhysicalDevice physicalDevice);

	void CreateLogicalDevice();

	void CreateSwapChain();
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void CreateImageViews();
	
	void CreateRenderPass();

	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void CleanupVulkan();
	void CleanupWindow();

	bool DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

protected:
	const SVKConfig& m_config;

	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkSurfaceKHR m_windowSurface;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_logicalDevice;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	std::vector<VkImageView> m_swapChainImageViews;
	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;

};

