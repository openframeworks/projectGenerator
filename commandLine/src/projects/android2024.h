#pragma once

#include "baseProject.h"

class android2024Project : public baseProject {
public:
	android2024Project(const std::string & target);

    bool createProjectFile() override;
    void addInclude(const fs::path & includeName) override {}
    void addLibrary(const LibraryBinary & lib) override {}
    void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) override {};
    void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) override {}
    void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) override {}
    void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) override {}
    void addAfterRule(const std::string& script) override {}
    void addDefine(const std::string& define, LibType libType = RELEASE_LIB) override {}
    
    
    //TODO: not sure if the following function should return false. as it will stop the further execution in  baseProject::create and make it to return as it had failed.
	bool loadProjectFile() override { return false; };
    bool saveProjectFile() override { return false; };
	static std::string LOG_NAME;
};
