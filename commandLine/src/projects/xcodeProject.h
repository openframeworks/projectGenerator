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
	bool debugCommands = false;
    
    static std::string LOG_NAME;

public:

	void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT);
	void addInclude(const fs::path & includeName);
	void addLibrary(const LibraryBinary & lib);
	void addLDFLAG(string ldflag, LibType libType = RELEASE_LIB);
	void addCFLAG(string cflag, LibType libType = RELEASE_LIB); // Other C Flags
	void addCPPFLAG(string cppflag, LibType libType = RELEASE_LIB); // Other C++ Flags
	void addAfterRule(string script);
	void addDefine(string define, LibType libType = RELEASE_LIB);

	void addCompileFlagsForMMFile(const fs::path & srcFile);
	void addFramework(const fs::path & path, const fs::path & folder);
	void addXCFramework(const fs::path & path, const fs::path & folder);
	void addDylib(const fs::path & path, const fs::path & folder);

	void addAddon(ofAddon & addon);
	void saveScheme();
	void renameProject();

	struct fileProperties {
		bool absolute = false;
		bool reference = true;
		bool addToBuildPhase = false;
		bool codeSignOnCopy = false;
		bool copyFilesBuildPhase = false;
		bool linkBinaryWithLibraries = false;
		bool addToBuildResource = false;
		bool addToResources = false;
		bool frameworksBuildPhase = false;
		bool isSrc = false;
		
		bool isGroupWithoutFolder = false;
	};
	
	string addFile(const fs::path & path, const fs::path & folder, const fileProperties & fp);
	void addCommand(const string & command);
	
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
	std::map <fs::path, string> folderUUID ;
	// Temporary
	std::map <string, fs::path> folderFromUUID ;
	
	string getFolderUUID(const fs::path & folder, bool isFolder = true, fs::path base = "");

	// TODO: Phase out relRoot. totally
	fs::path relRoot = "../../..";
	
	std::pair<string, string> rootReplacements;
	

	std::map<fs::path, string> extensionToFileType {
		{ ".framework" , "wrapper.framework" },
		{ ".xcframework" , "wrapper.xcframework" },
		{ ".dylib" , "compiled.mach-o.dylib" },
		
		{ ".cpp" , "sourcecode.cpp.cpp" },
		{ ".c" , "sourcecode.cpp.c" },
		{ ".h" , "sourcecode.cpp.h" },
		{ ".hpp" , "sourcecode.cpp.h" },
		{ ".mm" , "sourcecode.cpp.objcpp" },
		{ ".m" , "sourcecode.cpp.objcpp" },
		
		{ ".xib" , "file.xib" },
		{ ".metal" , "file.metal" },
		{ ".xcconfig" , "text.xcconfig" },

		{ ".entitlements" , "text.plist.entitlements" },
		{ ".plist" , "text.plist.xml" },
	};

};
