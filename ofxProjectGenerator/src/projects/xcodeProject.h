

#pragma once

#include "baseProject.h"

using std::string;

class xcodeProject : public baseProject {
public:
	xcodeProject(string target);

private:
	bool createProjectFile();
	bool loadProjectFile();
	bool saveProjectFile();
	void saveMakefile();

public:
	void addSrc(string srcFile, string folder, SrcType type=DEFAULT);
	void addInclude(string includeName);
	void addLibrary(const LibraryBinary & lib);
	void addLDFLAG(string ldflag, LibType libType = RELEASE_LIB);
	void addCFLAG(string cflag, LibType libType = RELEASE_LIB); // Other C Flags
	void addCPPFLAG(string cppflag, LibType libType = RELEASE_LIB); // Other C++ Flags
	void addAfterRule(string script);
	void addDefine(string define, LibType libType = RELEASE_LIB);
	
	// macOS specific
	void addFramework(string name, string path, string folder="");

	void addAddon(ofAddon & addon);
	void saveScheme();
	void renameProject();


	string projRootUUID;
	string resourcesUUID;
	string frameworksUUID;
	string buildPhaseResourcesUUID;
	string afterPhaseUUID;
	string buildPhasesUUID;
	// note this UUID is in an array of *all* the build steps

	// new vars
	string buildConfigurationListUUID;
	string buildActionMaskUUID;
	
	std::vector<string> commands;

	const string buildConfigurations[4] = {
		"E4B69B610A3A1757003C02F2", //Release
		"E4B69B600A3A1757003C02F2", //Debug
		
		"E4B69B4E0A3A1720003C02F2", //Debug SDKROOT macosx
		"E4B69B4F0A3A1720003C02F2", //Release SDKROOT macosx
	};
	
	const string buildConfigs[2] = {
		"E4B69B610A3A1757003C02F2", //Release
		"E4B69B600A3A1757003C02F2", //Debug
	};
	
	std::map <string, string> folderUUID = {
	};
	
	string getFolderUUID(string folder);
};
