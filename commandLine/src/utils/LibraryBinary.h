#pragma once

#include <string>
#include <vector>

#include "ofConstants.h"
namespace fs = of::filesystem;

struct LibraryBinary {

	LibraryBinary(fs::path p, std::string a, std::string t) {
		path = p;
		arch = a;
		target = t;
	}
	fs::path path;
	std::string arch;
	std::string target;

	static const std::vector<std::string> archs;
	static const std::vector<std::string> targets;
};
