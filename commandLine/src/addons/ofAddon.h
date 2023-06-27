/*
 * ofAddon.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#ifndef OFADDON_H_
#define OFADDON_H_

#include "ofConstants.h"
#include "LibraryBinary.h"
#include <unordered_map>
namespace fs = of::filesystem;
using std::string;
// #include <map>

// About Metadata

const string ADDON_NAME = "ADDON_NAME";
const string ADDON_DESCRIPTION = "ADDON_DESCRIPTION";
const string ADDON_AUTHOR = "ADDON_AUTHOR";
const string ADDON_TAGS = "ADDON_TAGS";
const string ADDON_URL = "ADDON_URL";

const std::vector<string> AddonMetaVariables = {
	ADDON_NAME,
	ADDON_DESCRIPTION,
	ADDON_AUTHOR,
	ADDON_TAGS,
	ADDON_URL,
};

// About Project settings

// About Build Settings
const string ADDON_DEPENDENCIES = "ADDON_DEPENDENCIES";
const string ADDON_INCLUDES = "ADDON_INCLUDES";
const string ADDON_CFLAGS = "ADDON_CFLAGS";
const string ADDON_CPPFLAGS = "ADDON_CPPFLAGS";
const string ADDON_LDFLAGS = "ADDON_LDFLAGS";
const string ADDON_LIBS = "ADDON_LIBS";
const string ADDON_DEFINES = "ADDON_DEFINES";

// About Source Codes
const string ADDON_SOURCES = "ADDON_SOURCES";
const string ADDON_HEADER_SOURCES = "ADDON_HEADER_SOURCES";
const string ADDON_C_SOURCES = "ADDON_C_SOURCES";
const string ADDON_CPP_SOURCES = "ADDON_CPP_SOURCES";
const string ADDON_OBJC_SOURCES = "ADDON_OBJC_SOURCES";

// About Exclude
const string ADDON_LIBS_EXCLUDE = "ADDON_LIBS_EXCLUDE";
const string ADDON_SOURCES_EXCLUDE = "ADDON_SOURCES_EXCLUDE";
const string ADDON_INCLUDES_EXCLUDE = "ADDON_INCLUDES_EXCLUDE";
const string ADDON_FRAMEWORKS_EXCLUDE = "ADDON_FRAMEWORKS_EXCLUDE";

const string ADDON_DATA = "ADDON_DATA";

// About Env Specific
const string ADDON_PKG_CONFIG_LIBRARIES = "ADDON_PKG_CONFIG_LIBRARIES";
const string ADDON_FRAMEWORKS = "ADDON_FRAMEWORKS";
const string ADDON_DLLS_TO_COPY = "ADDON_DLLS_TO_COPY";

const std::vector<string> AddonProjectVariables = {
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

	bool fromFS(fs::path path, const string & platform);
//	void fromXML(string installXmlName);
	void clear();

	// this is source files:
	std::unordered_map < string, string > filesToFolders;      //the addons has had, for each file,
												//sometimes a listing of what folder to put it in, such as "addons/ofxOsc/src"

	std::vector < string > srcFiles;
	std::vector < string > csrcFiles;
	std::vector < string > cppsrcFiles;
	std::vector < string > headersrcFiles;
	std::vector < string > objcsrcFiles;
//	std::vector < string > propsFiles;
	std::vector < fs::path > propsFiles;
	std::vector < LibraryBinary > libs;
	std::vector < string > dllsToCopy;
	std::vector < string > includePaths;

	// From addon_config.mk
	std::vector < string > dependencies;
	std::vector < string > cflags;   // C_FLAGS
	std::vector < string > cppflags; // CXX_FLAGS
	std::vector < string > ldflags;
	std::vector < string > pkgConfigLibs; 	// linux only
	std::vector < string > frameworks;		// osx only
	std::vector < string > data;
	std::vector < string > defines;

	// metadata
	string name;
	fs::path addonPath;
	string description;
	string author;
	std::vector<string> tags;
	string url;


	fs::path pathToOF;
	fs::path pathToProject;
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
		LinuxAArch64,
		AndroidARMv5,
		AndroidARMv7,
		Androidx86,
		Emscripten,
		iOS,
		OSX,
		Unknown
	} currentParseState;

	void parseConfig();
	void parseVariableValue(string variable, string value, bool addToValue, string line, int lineNum);
	void addReplaceString(string & variable, string value, bool addToVariable);
	void addReplaceStringVector(std::vector<string> & variable, string value, string prefix, bool addToVariable);
	void addReplaceStringVector(std::vector<LibraryBinary> & variable, string value, string prefix, bool addToVariable);
	void exclude(std::vector<string> & variable, std::vector<string> exclusions);
	void exclude(std::vector<LibraryBinary> & variable, std::vector<string> exclusions);
	ConfigParseState stateFromString(string name);
	string stateName(ConfigParseState state);
	bool checkCorrectVariable(string variable, ConfigParseState state);
	bool checkCorrectPlatform(ConfigParseState state);

	string platform;

	std::vector<string> excludeLibs;
	std::vector<string> excludeSources;
	std::vector<string> excludeIncludes;
	std::vector<string> excludeFrameworks;
};

#endif /* OFADDON_H_ */
