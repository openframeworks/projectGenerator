/*
 * ofAddon.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#pragma once

#include "ofConstants.h"
#include "LibraryBinary.h"
#include <map>
#include <iostream>
#include <filesystem>


namespace fs = of::filesystem;
using std::string;
using std::vector;



const vector<string> parseStates {
	"meta",
	"common",
	"linux",
	"linux64",
	"linux/64",
	"msys2",
	"vs",
	"linuxarmv6l",
	"linuxarmv7l",
	"linuxaarch64",
	"linux/armv6l",
	"linux/armv7l",
	"linux/aarch64",
	"linux/arm64",
	"android/armeabi",
	"android/armeabi-v7a",
	"android/arm64-v8a",
	"android/x86",
	"android/x86_64",
	"emscripten",
	"emscripten/32",
	"emscripten/64",
	"android",
	"ios",
	"osx",
	"tvos",
	"macos",
	"watchos",
	"visionos",
	"catos",
};


// About Build Settings
const string ADDON_DEPENDENCIES = "ADDON_DEPENDENCIES";
const string ADDON_INCLUDES = "ADDON_INCLUDES";
const string ADDON_CFLAGS = "ADDON_CFLAGS";
const string ADDON_CPPFLAGS = "ADDON_CPPFLAGS";
const string ADDON_LDFLAGS = "ADDON_LDFLAGS";
const string ADDON_LIBS = "ADDON_LIBS";
const string ADDON_DEFINES = "ADDON_DEFINES";
const string ADDON_ADDITIONAL_LIBS = "ADDON_ADDITIONAL_LIBS";

// About Source Codes
const string ADDON_SOURCES = "ADDON_SOURCES";
const string ADDON_HEADER_SOURCES = "ADDON_HEADER_SOURCES";
const string ADDON_C_SOURCES = "ADDON_C_SOURCES";
const string ADDON_CPP_SOURCES = "ADDON_CPP_SOURCES";
const string ADDON_OBJC_SOURCES = "ADDON_OBJC_SOURCES";

// About Exclude
const string ADDON_LIBS_EXCLUDE = "ADDON_LIBS_EXCLUDE";
const string ADDON_LIBS_DIR = "ADDON_LIBS_DIR";
const string ADDON_SOURCES_EXCLUDE = "ADDON_SOURCES_EXCLUDE";
const string ADDON_INCLUDES_EXCLUDE = "ADDON_INCLUDES_EXCLUDE";
const string ADDON_FRAMEWORKS_EXCLUDE = "ADDON_FRAMEWORKS_EXCLUDE";

const string ADDON_DATA = "ADDON_DATA";

// About Env Specific
const string ADDON_PKG_CONFIG_LIBRARIES = "ADDON_PKG_CONFIG_LIBRARIES";
const string ADDON_FRAMEWORKS = "ADDON_FRAMEWORKS";
const string ADDON_XCFRAMEWORKS = "ADDON_XCFRAMEWORKS";
const string ADDON_DLLS_TO_COPY = "ADDON_DLLS_TO_COPY";

// About Metadata
const string ADDON_NAME = "ADDON_NAME";
const string ADDON_DESCRIPTION = "ADDON_DESCRIPTION";
const string ADDON_AUTHOR = "ADDON_AUTHOR";
const string ADDON_TAGS = "ADDON_TAGS";
const string ADDON_URL = "ADDON_URL";

const vector<string> AddonMetaVariables {
	ADDON_NAME,
	ADDON_DESCRIPTION,
	ADDON_AUTHOR,
	ADDON_TAGS,
	ADDON_URL,
};

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
	ADDON_LIBS_DIR,
	ADDON_SOURCES_EXCLUDE,
	ADDON_INCLUDES_EXCLUDE,
	ADDON_FRAMEWORKS_EXCLUDE,

	ADDON_DATA,

	ADDON_PKG_CONFIG_LIBRARIES,
	ADDON_FRAMEWORKS,
	ADDON_DLLS_TO_COPY,
	ADDON_ADDITIONAL_LIBS,
};

class ofAddon {

public:

	ofAddon() = default;
	ofAddon(const ofAddon& other);

	void getFrameworksRecursively(const fs::path & path, string platform, string packageTarget = "");

	static string cleanName(const string& name);

	bool load(string addonName, const fs::path& projectDir, const string& targetPlatform);

	void prepareForWrite();

	void clear();

	vector <fs::path> additionalLibsFolder;
	vector <fs::path> libFiles;
	// this is source files:
	std::map < fs::path, fs::path > filesToFolders;      //the addons has had, for each file,
												//sometimes a listing of what folder to put it in, such as "addons/ofxOsc/src"

	vector < fs::path > srcFiles;
	vector < fs::path > csrcFiles;
	vector < fs::path > cppsrcFiles;
	vector < fs::path > headersrcFiles;
	vector < fs::path > objcsrcFiles;
//	vector < string > propsFiles;
	vector < fs::path > propsFiles;
	vector < LibraryBinary > libs;
	vector < fs::path > dllsToCopy;
	vector < fs::path > includePaths;
	vector < fs::path > libsPaths;

	// From addon_config.mk
	vector < string > dependencies;
	vector < string > cflags;   // C_FLAGS
	vector < string > cppflags; // CXX_FLAGS
	vector < string > ldflags;
	vector < string > pkgConfigLibs; 	// linux only
	vector < string > frameworks;		// osx only
	vector < string > xcframeworks;		// osx only
	vector < string > data;
	vector < string > defines;

	vector < string > definesCMAKE;

	// metadata
	string name;
	fs::path addonPath;
	string description;
	string author;
	vector<string> tags;
	string url;

	fs::path pathToOF = fs::path { "../../../ "};
	fs::path pathToProject = fs::path { "." };
	bool isLocalAddon = false; // set to true if the addon path is realtive to the project instead of in OF/addons/

	bool operator <(const ofAddon & addon) const{
		return addon.name < name;
	}
	string addonMakeName;



private:

	void parseLibsPath(const fs::path & path, const fs::path & parentFolder);

	void addToFolder(const fs::path& path, const fs::path & parentFolder);

	string currentParseState { "" };
	string emptyString = { "" };
//	void preParseConfig();
	void parseConfig();
	void parseVariableValue(const string & variable, const string & value, bool addToValue, const string & line, int lineNum);

	void addReplaceString(std::string &variable, const std::string &value, bool addToVariable);
//	void addReplaceStringPath(fs::path &variable, const std::string & value, bool addToVariable);
	void addReplaceStringVector(std::vector<std::string> &variable, const std::string &value, const std::string &prefix, bool addToVariable);
//	void addReplaceStringVectorPre(std::vector<std::string> &variable, const std::string &value, fs::path &prefix, bool addToVariable);
	void addReplaceStringVectorPathStr(std::vector<fs::path> & variable, fs::path & value, const std::string & prefix, bool addToVariable);

	void addReplaceStringVectorPath(std::vector<fs::path> &variable, const std::string &value, const std::string &prefix, bool addToVariable);
//	void addReplaceStringVectorPathAll(std::vector<fs::path> & variable, fs::path & value, fs::path & prefix, bool addToVariable);
//	void addReplaceStringVectorPathPrefix(std::vector<fs::path> &variable, const std::string &value, fs::path &prefix, bool addToVariable);

//	void addReplaceStringVectorLibrary(std::vector<LibraryBinary> & variable, const std::string & value, const std::string & prefix, bool addToVariable);

	void addReplaceStringVectorPath(std::vector<LibraryBinary> &variable, const std::string &value,  const fs::path &prefix, bool addToVariable);

//	void exclude(vector<string> & variable, vector<string> exclusions);
//	void excludePathStr(vector<fs::path> & variable, vector<string> exclusions);
//	void excludePath(vector<fs::path> & variable, vector<fs::path> exclusions);
//	void excludeLibrary(vector<LibraryBinary> & variable, vector<string> exclusions);
	bool checkCorrectVariable(const string & variable, const string & state);
	bool checkCorrectPlatform(const string & state);

	string platform;

	vector<string> excludeLibs;
	vector<string> excludeSources;
	vector<string> excludeIncludes;
	vector<string> excludeFrameworks;
	vector<string> excludeXCFrameworks;

	fs::path fixPath(const fs::path & path);

	fs::path normalizePath(const fs::path& path) {
		try {
//			auto value = fs::weakly_canonical(path);
			auto value = fs::relative(path);
			return value;
		} catch (const std::exception& ex) {
			std::cout << "Canonical path for [" << path << "] threw exception:\n"
					  << ex.what() << '\n';
			return fs::path("");
		}
	}

#ifdef OFADDON_OUTPUT_JSON_DEBUG
public:

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ofAddon, additionalLibsFolder, libFiles, filesToFolders, srcFiles, csrcFiles, cppsrcFiles, headersrcFiles, objcsrcFiles, propsFiles, libs, dllsToCopy, includePaths, libsPaths, dependencies, cflags,    cppflags,  ldflags, pkgConfigLibs,      frameworks, xcframeworks, data, defines, definesCMAKE, name, addonPath, description, author, tags, url, pathToOF, pathToProject, isLocalAddon, addonMakeName, currentParseState, platform, excludeLibs, excludeSources, excludeIncludes, excludeFrameworks, excludeXCFrameworks)
#endif


};
