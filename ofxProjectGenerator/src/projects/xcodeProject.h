

#pragma once

#include "baseProject.h"

class xcodeProject : public baseProject {
public:
	xcodeProject(std::string target);

private:
	bool createProjectFile();
	bool loadProjectFile();
	bool saveProjectFile();
	void saveMakefile();

public:
	void addSrc(std::string srcFile, std::string folder, SrcType type=DEFAULT);
	void addInclude(std::string includeName);
	void addLibrary(const LibraryBinary & lib);
	void addLDFLAG(std::string ldflag, LibType libType = RELEASE_LIB);
	void addCFLAG(std::string cflag, LibType libType = RELEASE_LIB); // Other C Flags
	void addCPPFLAG(std::string cppflag, LibType libType = RELEASE_LIB); // Other C++ Flags
	void addAfterRule(std::string script);
	void addDefine(std::string define, LibType libType = RELEASE_LIB);
	
	// macOS specific
	void addFramework(std::string name, std::string path, std::string folder="");

	void addAddon(ofAddon & addon);
	void saveScheme();
	void renameProject();


	std::string projRootUUID;
	std::string resourcesUUID;
	std::string frameworksUUID;
	std::string buildPhaseResourcesUUID;
	std::string afterPhaseUUID;
	std::string buildPhasesUUID;
	// note this UUID is in an array of *all* the build steps

	// new vars
	std::string buildConfigurationListUUID;
	std::string buildActionMaskUUID;
	
	std::vector<std::string> commands;

	const std::string buildConfigurations[6] = {
		"E4B69B610A3A1757003C02F2", //Release
		"E4B69B600A3A1757003C02F2", //Debug
		"99FA3DBC1C7456C400CFA0EE", //AppStore
		
		"99FA3DBB1C7456C400CFA0EE", //AppStore SDKROOT macosx
		"E4B69B4E0A3A1720003C02F2", //Debug SDKROOT macosx
		"E4B69B4F0A3A1720003C02F2", //Release SDKROOT macosx
	};
	
	const std::string buildConfigs[3] = {
		"E4B69B610A3A1757003C02F2", //Release
		"E4B69B600A3A1757003C02F2", //Debug
		"99FA3DBC1C7456C400CFA0EE", //AppStore
	};
	
	std::map <std::string, std::string> folderUUID = {
	};
	
	std::string getFolderUUID(std::string folder);
};
