#pragma once

#include "baseProject.h"

class android2024Project : public baseProject {
public:
	android2024Project(const std::string & target);

	virtual bool createProjectFile() override;
	virtual void addInclude(const fs::path & includeName) override {}
	virtual void addLibrary(const LibraryBinary & lib) override {}
	virtual void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) override {};
    virtual void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) override {}
    virtual void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) override {}
    virtual void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) override {}
    virtual void addAfterRule(const std::string& script) override {}
    virtual void addDefine(const std::string& define, LibType libType = RELEASE_LIB) override {}
    
    
    
	virtual bool loadProjectFile() override { return false; };
    virtual bool saveProjectFile() override { return false; };
	static std::string LOG_NAME;
};
