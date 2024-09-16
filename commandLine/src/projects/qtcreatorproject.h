#pragma once

#include "ofFileUtils.h" // ofBuffer
#include "baseProject.h"
#include <set>

class QtCreatorProject : public baseProject {
public:
	QtCreatorProject(const std::string & target);

	virtual bool createProjectFile() override;
	virtual void addInclude(const fs::path & includeName) override {}
	virtual void addLibrary(const LibraryBinary & lib)override{}
	virtual void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) override;
    
    virtual void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) override {}
    virtual void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) override {}
    virtual void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) override {}
    virtual void addAfterRule(const std::string& script) override {}
    virtual void addDefine(const std::string& define, LibType libType = RELEASE_LIB) override {}
    
    
    
	virtual bool loadProjectFile() override;
	virtual bool saveProjectFile() override;
	static std::string LOG_NAME;

private:
	void addAddon(ofAddon & addon);
	using baseProject::addAddon;
	std::set<std::string> qbsProjectFiles;
	ofBuffer qbs;
	std::string originalFilesStr, originalAddonsStr;
};
