#pragma once

#include "common.h"

class SVKConfig
{
public:
	SVKConfig(int argc, char** argv);

public:
	std::filesystem::path m_appDir;
};

