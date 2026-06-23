#include "LibraryBinary.h"
#include "ofFileUtils.h"
#include <algorithm>

const std::vector<std::string> LibraryBinary::archs { "x86", "Win32", "x64", "armv7", "ARM64", "ARM64EC" };
const std::vector<std::string> LibraryBinary::targets{ "Debug", "Release" };

LibraryBinary::LibraryBinary(fs::path p):path(p){
    findArch(path);
    findTarget(path);
}

std::string findStringsInPath(const std::vector<std::string> & strings, const std::filesystem::path & path){
    for (const auto& part : path) {
        std::string p = ofPathToString(part);
        if (std::find(strings.begin(), strings.end(), p) != strings.end()) {
            return p;
        }
    }
    return "";
}


void LibraryBinary::findArch(const std::filesystem::path & path){
    this->arch = findStringsInPath(LibraryBinary::archs, path);
}

void LibraryBinary::findTarget(const std::filesystem::path & path){
    this->target = findStringsInPath(LibraryBinary::targets, path);
}

bool LibraryBinary::isValidFor(const std::string& _arch, const std::string& _target){
    bool targetOK = (_target.empty() || (_target == this->target));
    bool archOK = (_arch.empty() || (_arch == this->arch));
    return targetOK && archOK;
}

