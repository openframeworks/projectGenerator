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

	virtual bool createProjectFile() override;
	virtual bool loadProjectFile() override;
	virtual bool saveProjectFile() override;

	virtual void addSrc(const fs::path & srcName, const fs::path & folder, SrcType type=DEFAULT) override;
	virtual void addInclude(const fs::path & includeName) override;
	virtual void addLibrary(const LibraryBinary & lib) override;

    virtual void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) override {}
    virtual void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) override {}
    virtual void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) override {}
    virtual void addAfterRule(const std::string& script) override {}
    virtual void addDefine(const std::string& define, LibType libType = RELEASE_LIB) override {}
    
    
	static std::string LOG_NAME;
};
