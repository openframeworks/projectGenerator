/*
 * VSCodeProject.h
 *
 *  Created on: 28/09/2023
 *      Author: Dimitre Lima
 */

#pragma once

#include "baseProject.h"

class VSCodeProject: public baseProject {
public:
	VSCodeProject(const std::string & target) : baseProject(target) {};

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
    

	void addAddon(ofAddon & addon) ;

	static std::string LOG_NAME;

private:

};
