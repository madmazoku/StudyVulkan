#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <exception>

class VkException : public std::exception
{
protected:
	static const char* VkResultName(VkResult result);

public:
	explicit VkException(VkResult result, const char* errMsg);
	virtual ~VkException() noexcept;

	virtual const char* what() const noexcept;

	VkResult result() const noexcept;

protected:
	VkResult m_result;
	const char* m_errMsg;
};

