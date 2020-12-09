#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <map>
#include <set>
#include <array>

#include <optional>
#include <filesystem>
#include <chrono>

#ifdef _DEBUG
#define NDEBUG
#endif // _DEBUG

#include "VkException.h"

inline void vkCheckResult(VkResult result, const char* errMsg) {
	if (result != VK_SUCCESS)
		throw VkException(result, errMsg);
}
