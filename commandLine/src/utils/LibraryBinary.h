#pragma once

#include <algorithm>
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
    LibraryBinary(fs::path p);
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
    
    ///\ brief check if  it is valid for the passed arch and target.
    ///\param arch the architecture to check for. An empty string means any architecture
    ///\param target the target to check for. An empty string means any target
    ///\return true if valid false otherwise
    bool isValidFor(const std::string& arch, const std::string& target);
    
    
    operator fs::path() const {
        return path;
//        return ofPathToString(path);
    }

    
protected:
    void findArch(const std::filesystem::path & path);
    void findTarget(const std::filesystem::path & path);
    
    
    
};
