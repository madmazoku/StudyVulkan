#include "SVKConfig.h"

SVKConfig::SVKConfig(int argc, char** argv) {
	m_appDir = std::filesystem::path(argv[0]).parent_path();
}