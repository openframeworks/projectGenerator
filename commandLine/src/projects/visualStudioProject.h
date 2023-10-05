#pragma once

#include "baseProject.h"

class visualStudioProject : public baseProject {

public:
	visualStudioProject(const std::string & target) : baseProject(target) {};

	bool createProjectFile();
	bool loadProjectFile();
	bool saveProjectFile();

	void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT);
	void addInclude(std::string includeName);
	void addProps(fs::path propsFile);
	void addLibrary(const LibraryBinary & lib);
	void addCFLAG(std::string cflag, LibType libType = RELEASE_LIB); // C
	void addCPPFLAG(std::string cppflag, LibType libType = RELEASE_LIB); // C++
	void addDefine(std::string define, LibType libType = RELEASE_LIB);

	void addAddon(ofAddon & addon);

	static std::string LOG_NAME;

	pugi::xml_document filterXmlDoc;

	void appendFilter(std::string folderName);

	vector <fs::path> additionalvcxproj;
	fs::path solution;
private:

};
