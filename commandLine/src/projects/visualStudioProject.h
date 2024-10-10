#pragma once

#include "baseProject.h"

class visualStudioProject : public baseProject {

public:
	visualStudioProject(const std::string & target) : baseProject(target) {};

	bool createProjectFile() override;
	bool loadProjectFile() override;
	bool saveProjectFile() override;

	void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) override;
	void addInclude(const fs::path & includeName) override;
	void addProps(fs::path propsFile);
	void addLibrary(const LibraryBinary & lib)override;
	void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) override ; // C
	void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) override ; // C++
	void addDefine(const std::string& define, LibType libType = RELEASE_LIB) override ;
    
    void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) override {}
    void addAfterRule(const std::string& script) override {}
    
    

	void addAddonIncludePaths(const ofAddon& addon) override;

	void addAddonCflags(const ofAddon& addon) override;
	void addAddonCppflags(const ofAddon& addon) override;

	void addAddonDefines(const ofAddon& addon) override;

	void addAddonProps(const ofAddon& addon) override;

	void addAddonBegin(const ofAddon& addon) override;


   void ensureDllDirectoriesExist() ;

	static std::string LOG_NAME;

	pugi::xml_document filterXmlDoc;

	void appendFilter(std::string folderName);

	vector <fs::path> additionalvcxproj;
	fs::path solution;
protected:
	void addSrcFiles(ofAddon& addon, const vector<fs::path> &filepaths, SrcType type, bool bFindInFilesToFolder = true) override;


};
