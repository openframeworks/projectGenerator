/*
 * ofAddon.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#pragma once

#include "ofConstants.h"
#include "LibraryBinary.h"
#include <unordered_map>

namespace fs = of::filesystem;
using std::string;
using std::vector;
// #include <map>
// About Metadata

const vector<string> AddonMetaVariables {
	"ADDON_NAME",
	"ADDON_DESCRIPTION",
	"ADDON_AUTHOR",
	"ADDON_TAGS",
	"ADDON_URL",
};

const vector<string> parseStates {
	"meta",
	"common",
	"linux",
	"linux64",
	"msys2",
	"vs",
	"linuxarmv6l",
	"linuxarmv7l",
	"linuxaarch64",
	"android/armeabi",
	"android/armeabi-v7a",
	"android/arm64-v8a",
	"android/x86",
	"android/x86_64",
	"emscripten",
	"ios",
	"osx",
	"tvos",
	"watchos",
	"visionos",
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

const vector<string> AddonProjectVariables = {
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

	bool fromFS(const fs::path & path, const string & platform);
	void parseLibsPath(const fs::path & path, const fs::path & parentFolder);
	vector <fs::path> additionalLibsFolder;
	vector <fs::path> libFiles;

//	void fromXML(string installXmlName);
	void clear();

	// this is source files:
	// FIXME: map using fs::path, fs::path
	std::unordered_map < string, string > filesToFolders;      //the addons has had, for each file,
												//sometimes a listing of what folder to put it in, such as "addons/ofxOsc/src"

	vector < string > srcFiles;
	vector < string > csrcFiles;
	vector < string > cppsrcFiles;
	vector < string > headersrcFiles;
	vector < string > objcsrcFiles;
//	vector < string > propsFiles;
	vector < fs::path > propsFiles;
	vector < LibraryBinary > libs;
	vector < string > dllsToCopy;
	vector < string > includePaths;

	// From addon_config.mk
	vector < string > dependencies;
	vector < string > cflags;   // C_FLAGS
	vector < string > cppflags; // CXX_FLAGS
	vector < string > ldflags;
	vector < string > pkgConfigLibs; 	// linux only
	vector < string > frameworks;		// osx only
	vector < string > data;
	vector < string > defines;

	// metadata
	string name;
	fs::path addonPath;
	string description;
	string author;
	vector<string> tags;
	string url;


	fs::path pathToOF;
	fs::path pathToProject;
	bool isLocalAddon; // set to true if the addon path is realtive to the project instead of in OF/addons/

	bool operator <(const ofAddon & addon) const{
		return addon.name < name;
	}

private:
	
	string currentParseState { "" };

	void preParseConfig();
	void parseConfig();
	void parseVariableValue(const string & variable, const string & value, bool addToValue, const string & line, int lineNum);
	void addReplaceString(string & variable, string value, bool addToVariable);
	void addReplaceStringVector(vector<string> & variable, string value, string prefix, bool addToVariable);
	void addReplaceStringVector(vector<LibraryBinary> & variable, string value, string prefix, bool addToVariable);
	void exclude(vector<string> & variable, vector<string> exclusions);
	void exclude(vector<LibraryBinary> & variable, vector<string> exclusions);
	bool checkCorrectVariable(const string & variable, const string & state);
	bool checkCorrectPlatform(const string & state);

	string platform;

	vector<string> excludeLibs;
	vector<string> excludeSources;
	vector<string> excludeIncludes;
	vector<string> excludeFrameworks;

	fs::path fixPath(const fs::path & path);
};
