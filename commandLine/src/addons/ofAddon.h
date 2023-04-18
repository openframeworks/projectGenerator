/*
 * ofAddon.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#ifndef OFADDON_H_
#define OFADDON_H_

<<<<<<< HEAD:commandLine/src/addons/ofAddon.h
#include "ofConstants.h"
#include "LibraryBinary.h"
#include <unordered_map>
=======
// FIXME: of::filesystem only
#include "ofConstants.h"
#include "LibraryBinary.h"
#include <map>
>>>>>>> 4f16f91 (removing unneded paths):ofxProjectGenerator/src/addons/ofAddon.h

// About Metadata

const std::string ADDON_NAME = "ADDON_NAME";
const std::string ADDON_DESCRIPTION = "ADDON_DESCRIPTION";
const std::string ADDON_AUTHOR = "ADDON_AUTHOR";
const std::string ADDON_TAGS = "ADDON_TAGS";
const std::string ADDON_URL = "ADDON_URL";

const std::vector<std::string> AddonMetaVariables = {
	ADDON_NAME,
	ADDON_DESCRIPTION,
	ADDON_AUTHOR,
	ADDON_TAGS,
	ADDON_URL,
};

// About Project settings

// About Build Settings
const std::string ADDON_DEPENDENCIES = "ADDON_DEPENDENCIES";
const std::string ADDON_INCLUDES = "ADDON_INCLUDES";
const std::string ADDON_CFLAGS = "ADDON_CFLAGS";
const std::string ADDON_CPPFLAGS = "ADDON_CPPFLAGS";
const std::string ADDON_LDFLAGS = "ADDON_LDFLAGS";
const std::string ADDON_LIBS = "ADDON_LIBS";
const std::string ADDON_DEFINES = "ADDON_DEFINES";

// About Source Codes
const std::string ADDON_SOURCES = "ADDON_SOURCES";
const std::string ADDON_HEADER_SOURCES = "ADDON_HEADER_SOURCES";
const std::string ADDON_C_SOURCES = "ADDON_C_SOURCES";
const std::string ADDON_CPP_SOURCES = "ADDON_CPP_SOURCES";
const std::string ADDON_OBJC_SOURCES = "ADDON_OBJC_SOURCES";

// About Exclude
const std::string ADDON_LIBS_EXCLUDE = "ADDON_LIBS_EXCLUDE";
const std::string ADDON_SOURCES_EXCLUDE = "ADDON_SOURCES_EXCLUDE";
const std::string ADDON_INCLUDES_EXCLUDE = "ADDON_INCLUDES_EXCLUDE";
const std::string ADDON_FRAMEWORKS_EXCLUDE = "ADDON_FRAMEWORKS_EXCLUDE";

const std::string ADDON_DATA = "ADDON_DATA";

// About Env Specific
const std::string ADDON_PKG_CONFIG_LIBRARIES = "ADDON_PKG_CONFIG_LIBRARIES";
const std::string ADDON_FRAMEWORKS = "ADDON_FRAMEWORKS";
const std::string ADDON_DLLS_TO_COPY = "ADDON_DLLS_TO_COPY";

const std::vector<std::string> AddonProjectVariables = {
	ADDON_DEPENDENCIES,

	ADDON_INCLUDES,
	ADDON_CFLAGS,
	ADDON_CPPFLAGS,
	ADDON_LDFLAGS,
	ADDON_LIBS,
	ADDON_DEFINES,

	ADDON_SOURCES,
	ADDON_HEADER_SOURCES,
	ADDON_C_SOURCES,
	ADDON_CPP_SOURCES,
	ADDON_OBJC_SOURCES,

	ADDON_LIBS_EXCLUDE,
	ADDON_SOURCES_EXCLUDE,
	ADDON_INCLUDES_EXCLUDE,
	ADDON_FRAMEWORKS_EXCLUDE,

	ADDON_DATA,

	ADDON_PKG_CONFIG_LIBRARIES,
	ADDON_FRAMEWORKS,
	ADDON_DLLS_TO_COPY,
};

class ofAddon {

public:

	ofAddon();

	bool fromFS(of::filesystem::path path, const std::string & platform);
//	void fromXML(std::string installXmlName);
	void clear();

	// this is source files:
	std::unordered_map < std::string, std::string > filesToFolders;      //the addons has had, for each file,
												//sometimes a listing of what folder to put it in, such as "addons/ofxOsc/src"

	std::vector < std::string > srcFiles;
	std::vector < std::string > csrcFiles;
	std::vector < std::string > cppsrcFiles;
	std::vector < std::string > headersrcFiles;
	std::vector < std::string > objcsrcFiles;
	std::vector < std::string > propsFiles;
	std::vector < LibraryBinary > libs;
	std::vector < std::string > dllsToCopy;
	std::vector < std::string > includePaths;

	// From addon_config.mk
	std::vector < std::string > dependencies;
	std::vector < std::string > cflags;   // C_FLAGS
	std::vector < std::string > cppflags; // CXX_FLAGS
	std::vector < std::string > ldflags;
	std::vector < std::string > pkgConfigLibs; 	// linux only
	std::vector < std::string > frameworks;		// osx only
	std::vector < std::string > data;
	std::vector < std::string > defines;

	// metadata
	std::string name;
	of::filesystem::path addonPath;
	std::string description;
	std::string author;
	std::vector<std::string> tags;
	std::string url;


	of::filesystem::path pathToOF;
	of::filesystem::path pathToProject;
	bool isLocalAddon; // set to true if the addon path is realtive to the project instead of in OF/addons/

	bool operator <(const ofAddon & addon) const{
		return addon.name < name;
	}

private:

	enum ConfigParseState{
		Meta,
		Common,
		Linux,
		Linux64,
		MinGW,
		VS,
		LinuxARMv6,
		LinuxARMv7,
		AndroidARMv5,
		AndroidARMv7,
		Androidx86,
		Emscripten,
		iOS,
		OSX,
		Unknown
	} currentParseState;

	void parseConfig();
	void parseVariableValue(std::string variable, std::string value, bool addToValue, std::string line, int lineNum);
	void addReplaceString(std::string & variable, std::string value, bool addToVariable);
	void addReplaceStringVector(std::vector<std::string> & variable, std::string value, std::string prefix, bool addToVariable);
	void addReplaceStringVector(std::vector<LibraryBinary> & variable, std::string value, std::string prefix, bool addToVariable);
	void exclude(std::vector<std::string> & variable, std::vector<std::string> exclusions);
	void exclude(std::vector<LibraryBinary> & variable, std::vector<std::string> exclusions);
	ConfigParseState stateFromString(std::string name);
	std::string stateName(ConfigParseState state);
	bool checkCorrectVariable(std::string variable, ConfigParseState state);
	bool checkCorrectPlatform(ConfigParseState state);

	std::string platform;

	std::vector<std::string> excludeLibs;
	std::vector<std::string> excludeSources;
	std::vector<std::string> excludeIncludes;
	std::vector<std::string> excludeFrameworks;
};

#endif /* OFADDON_H_ */
