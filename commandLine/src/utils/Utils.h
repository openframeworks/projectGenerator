/*
 * Utils.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#pragma once

#include "pugixml.hpp"

#include "ofLog.h"
#include "ofSystemUtils.h"
#include "baseProject.h"
#include <unordered_set>
struct LibraryBinary;

std::string execute_popen(const char* cmd);

std::string getPlatformString();

namespace fs = of::filesystem;
using std::string;
using std::vector;
using std::cout;
using std::endl;

static bool backupProjectFiles = false;

static std::map <ofTargetPlatform, std::string> platformsToString {
	{ OF_TARGET_ANDROID, "android" },
//	{ OF_TARGET_EMSCRIPTEN, "" },
	{ OF_TARGET_IOS, "ios" },
    { OF_TARGET_MACOS, "macos" },
	{ OF_TARGET_LINUX, "linux" },
	{ OF_TARGET_LINUX64, "linux64" },
	{ OF_TARGET_LINUXARMV6L, "linuxarmv6l" },
	{ OF_TARGET_LINUXARMV7L, "linuxarmv7l" },
	{ OF_TARGET_LINUXAARCH64, "linuxaarch64" },
	{ OF_TARGET_MINGW, "msys2" },
	{ OF_TARGET_OSX, "osx" },
	{ OF_TARGET_WINVS, "vs" },
};

static std::vector < std::string > platformsOptions {
	"android",
	"ios",
	"linux",
	"linux64",
	"linuxarmv6l",
	"linuxarmv7l",
	"linuxaarch64",
	"msys2",
	"osx",
	"vs",
};

string generateUUID(const string & input);
string generateUUID(const fs::path & path);

fs::path getOFRoot();
void setOFRoot(const fs::path & path);

string convertStringToWindowsSeparator(string in);

void findandreplace( string& tInput, string tFind, string tReplace );
void findandreplaceInTexfile (const fs::path & fileName, string tFind, string tReplace );

bool doesTagAndAttributeExist(pugi::xml_document & doc, string tag, string attribute, string newValue);
pugi::xml_node appendValue(pugi::xml_document & doc, string tag, string attribute, string newValue, bool addMultiple = false);
int countSubdirectories(const fs::path &path);
void getFoldersRecursively(const fs::path & path, std::vector < fs::path > & folderNames, string platform);
void getFilesRecursively(const fs::path & path, std::vector < string > & fileNames);
void getFilesRecursively(const fs::path & path, std::vector < fs::path > & fileNames);
void getLibsRecursively(const fs::path & path, std::vector < fs::path > & libFiles, std::vector < LibraryBinary > & libLibs, string platform = "", string arch = "", string target = "");
void getFrameworksRecursively(const fs::path & path, std::vector < string > & frameworks,  string platform = "" );
void getXCFrameworksRecursively(const fs::path & path, std::vector<string> & xcframeworks, string platform = "");
void getPropsRecursively(const fs::path & path, std::vector < fs::path > & props, const string & platform);
void getDllsRecursively(const fs::path & path, std::vector < fs::path > & dlls, string platform);

void splitFromFirst(string toSplit, string deliminator, string & first, string & second);

void fixSlashOrder(string & toFix);
void fixSlashOrderPath(fs::path &toFix);
string unsplitString (std::vector < string > strings, string deliminator );

std::unique_ptr<baseProject> getTargetProject(const string & targ);

void messageError(const string & targ);
void messageReturn(const string & targ);
void messageReturn(const string & key, const string & value);
void messageExtra(const string & targ);

template <class T>
inline bool isInVector(T item, std::vector<T> & vec){
	bool bIsInVector = false;
	for(int i=0;i<vec.size();i++){
		if(vec[i] == item){
			bIsInVector = true;
			break;
		}
	}
	return bIsInVector;
}


inline void removeDuplicates(std::vector<std::string> & vec){
    std::unordered_set<std::string> seen;
    std::vector<std::string> output;
    
    for (const auto& value : vec) {
        if (seen.insert(value).second) { // If insertion is successful (element not seen before)
            output.push_back(value);
        }
    }
    vec = std::move(output);
}

inline void removeDuplicates(std::vector<fs::path> & vec){
    std::unordered_set<std::string> seen;
    std::vector<fs::path> output;
    
    for (const auto& value : vec) {
        if (seen.insert(value.string()).second) { // If insertion is successful (element not seen before)
            output.emplace_back(value);
        }
    }
    vec = std::move(output);
}

string colorText(const string & s, int color = 32);
void alert(string msg, int color=32);

vector <fs::path> dirList (const fs::path & path);
vector <fs::path> folderList (const fs::path & path);
vector <string> fileToStrings (const fs::path & file);
fs::path getUserHomeDir();
std::string getPGVersion();

fs::path makeRelative(const fs::path& from, const fs::path& to);

bool ofIsPathInPath(const fs::path & path, const fs::path & base);
void createBackup(const fs::path & path, const fs::path & backupPath);

std::string normalizePath(const std::string& path);

fs::path normalizePath(const fs::path& path);

bool containsSourceFiles(const fs::path& dir);
fs::path ofRelativeToOFPATH(const fs::path& path);

/*
 Idea: create an object to hold the origin and destination files, with renames where needed
 and string substitution, so we can avoid opening and writing multiple times the same file, less ssd wear.
 */
