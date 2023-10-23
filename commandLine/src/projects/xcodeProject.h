#pragma once

#include "baseProject.h"
//#include <unordered_map>
#include <map>

using std::string;

class xcodeProject : public baseProject {
public:
	xcodeProject(const string & target);

private:
	bool createProjectFile();
	bool loadProjectFile();
	bool saveProjectFile();
	void saveMakefile();

public:
	void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT);
	void addInclude(string includeName);
	void addLibrary(const LibraryBinary & lib);
	void addLDFLAG(string ldflag, LibType libType = RELEASE_LIB);
	void addCFLAG(string cflag, LibType libType = RELEASE_LIB); // Other C Flags
	void addCPPFLAG(string cppflag, LibType libType = RELEASE_LIB); // Other C++ Flags
	void addAfterRule(string script);
	void addDefine(string define, LibType libType = RELEASE_LIB);

	void addFramework(const string & name, const fs::path & path, const fs::path & folder);
	void addDylib(const string & name, const fs::path & path, const fs::path & folder);

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

	// MARK: now this is not being used anymore, only buildConfigs
	// TODO: test in iOS and clean if OK
	string buildConfigurations[4] = {
		"E4B69B600A3A1757003C02F2", //macOS Debug
		"E4B69B610A3A1757003C02F2", //macOS Release

		"E4B69B4E0A3A1720003C02F2", //macOS Debug SDKROOT macosx
		"E4B69B4F0A3A1720003C02F2", //macOS Release SDKROOT macosx
	};

	string buildConfigs[2] = {
		"E4B69B610A3A1757003C02F2", //Release
		"E4B69B600A3A1757003C02F2", //Debug
	};

//	std::unordered_map <string, string> folderUUID ;
	std::map < fs::path , string> folderUUID ;
	string getFolderUUID(const fs::path & folder, bool isFolder = true, fs::path base = "");

	// TODO: Phase out relRoot. totally
	fs::path relRoot = "../../..";
};
