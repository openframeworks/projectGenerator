#pragma once

#include <string>
#include <vector>


#include "ofConstants.h"
#include "defines.h"

#ifdef OFADDON_OUTPUT_JSON_DEBUG // defined in defines.h. only for dev debugging
#include "ofJson.h"
#endif

namespace fs = of::filesystem;

class  LibraryBinary {
public:
    LibraryBinary(){}
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

#ifdef OFADDON_OUTPUT_JSON_DEBUG
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(LibraryBinary, path, arch, target)
#endif
    
};
