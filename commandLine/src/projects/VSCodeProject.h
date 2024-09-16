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
    

	void addAddon(ofAddon & addon) ;

	static std::string LOG_NAME;

private:

};
