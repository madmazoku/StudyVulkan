#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <sstream>

#include <vector>
#include <map>
#include <set>

#include <optional>

#ifdef _DEBUG
#define NDEBUG
#endif // _DEBUG

void vkCheckResultThrow(VkResult result, const char* const errMsg);
const char* const vkResultName(VkResult result);

inline void vkCheckResult(VkResult result, const char* const errMsg) {
	if (result != VK_SUCCESS)
		vkCheckResultThrow(result, errMsg);
}
