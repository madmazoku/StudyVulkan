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

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

public:
	static const int g_width;
	static const int g_height;
	static const char* const g_appName;
	static const char* const g_engineName;
	static const bool g_enableValidationLayers;
	static const std::vector<const char*> g_validationLayers;
	static const std::vector<const char*> g_deviceExtensions;
	static const int g_maxFramesInFlight;
	static const std::vector<SVKApp::Vertex> g_vertices;
	static const std::vector<uint16_t> g_indices;

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

	static void ResizeCallback(GLFWwindow* window, int width, int height);

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
	void CreateDescriptorSetLayout();

	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	void CreateFrameBuffers();
	void CreateCommandPool();
	
	void CreateTextureImage();
	void CreateImage(
		uint32_t width, 
		uint32_t height, 
		VkFormat format, 
		VkImageTiling tiling, 
		VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties, 
		VkImage& image, 
		VkDeviceMemory& imageMemory
	);
	void TransitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout
	);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();

	void CreateBuffer(
		VkDeviceSize size, 
		VkBufferUsageFlags usage, 
		VkMemoryPropertyFlags properties, 
		VkBuffer& buffer, 
		VkDeviceMemory& bufferMemory
	);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void CleanupVulkan();
	void CleanupWindow();

	void CleanupSwapChain();
	void RecreateSwapChain();

	void DrawFrame();
	void UpdateUniformBuffer(uint32_t currentImage);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	bool OnDebug(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);

	void OnResize(int width, int height);

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
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;
	std::vector<VkFramebuffer> m_swapChainFrameBuffers;
	VkCommandPool m_commandPool;
	VkImage m_textureImage;
	VkDeviceMemory m_textureImageMemory;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<VkCommandBuffer> m_commandBuffers;
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;
	size_t m_currentFrame;
	bool m_framebufferResized;
};

