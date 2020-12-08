#pragma once

#include "common.h"

class SVKApp
{
public:
	static const int g_width;
	static const int g_height;
	static const char* const g_appName;
	static const char* const g_engineName;

public:
	SVKApp();

	void Initialize();
	void Cleanup();
	void Run();

private:
	void InitializeWindow();
	void InitializeVulkan();

	void CreateInstance();

	void CleanupVulkan();
	void CleanupWindow();

protected:
	GLFWwindow* m_window;
	VkInstance m_instance;
};

