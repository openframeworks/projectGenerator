#pragma once

#include <string>
#include <vector>

//#include "ofConstants.h"
//namespace fs = of::filesystem;

struct LibraryBinary {
	// FIXME: FS (and includes)
//	fs::path path;
	std::string path;
	std::string arch;
	std::string target;

	static const std::vector<std::string> archs;
	static const std::vector<std::string> targets;
};
