/*
 * CBLinuxProject.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#pragma once

#include "baseProject.h"

class CBWinProject: public baseProject {
public:
	CBWinProject(const std::string & target) : baseProject(target) {};

	bool createProjectFile() override;
	bool loadProjectFile() override;
	bool saveProjectFile() override;

	void addSrc(const fs::path & srcName, const fs::path & folder, SrcType type=DEFAULT) override;
	void addInclude(const fs::path & includeName) override;
	void addLibrary(const LibraryBinary & lib) override;

    void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) override {}
    void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) override {}
    void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) override {}
    void addAfterRule(const std::string& script) override {}
    void addDefine(const std::string& define, LibType libType = RELEASE_LIB) override {}
    
    
	static std::string LOG_NAME;
};
