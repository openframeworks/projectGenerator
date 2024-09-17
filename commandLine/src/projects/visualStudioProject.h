#pragma once

#include "baseProject.h"

class visualStudioProject : public baseProject {

public:
	visualStudioProject(const std::string & target) : baseProject(target) {};

	virtual bool createProjectFile() override;
	virtual bool loadProjectFile() override;
	virtual bool saveProjectFile() override;

	virtual void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) override;
	virtual void addInclude(const fs::path & includeName) override;
	void addProps(fs::path propsFile);
	virtual void addLibrary(const LibraryBinary & lib)override;
	virtual void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) override ; // C
	virtual void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) override ; // C++
	virtual void addDefine(const std::string& define, LibType libType = RELEASE_LIB) override ;
    
    virtual void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) override {}
    virtual void addAfterRule(const std::string& script) override {}
    
    
    void ensureDllDirectoriesExist() ;
    void addAddon(ofAddon & addon) ;

	static std::string LOG_NAME;

	pugi::xml_document filterXmlDoc;

	void appendFilter(std::string folderName);

	vector <fs::path> additionalvcxproj;
	fs::path solution;
private:

};
