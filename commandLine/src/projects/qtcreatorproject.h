#pragma once

#include "ofFileUtils.h" // ofBuffer
#include "baseProject.h"
#include <set>

class QtCreatorProject : public baseProject {
public:
	QtCreatorProject(const std::string & target);

	bool createProjectFile() override;
	void addInclude(const fs::path & includeName) override {}
	void addLibrary(const LibraryBinary & lib)override{}
	void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) override;
    
    void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) override {}
    void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) override {}
    void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) override {}
    void addAfterRule(const std::string& script) override {}
    void addDefine(const std::string& define, LibType libType = RELEASE_LIB) override {}
    
	bool loadProjectFile() override;
	bool saveProjectFile() override;
	static std::string LOG_NAME;

private:
	void addAddon(ofAddon & addon);
	using baseProject::addAddon;
	std::set<std::string> qbsProjectFiles;
	ofBuffer qbs;
	std::string originalFilesStr, originalAddonsStr;
};
