/*
 * Utils.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#ifndef UTILS_H_
#define UTILS_H_

#include "pugixml.hpp"

#include "ofLog.h"
#include "ofSystemUtils.h"
#include "baseProject.h"
struct LibraryBinary;

namespace fs = of::filesystem;
using std::string;
using std::vector;
using std::cout;
using std::endl;

std::string generateUUID(std::string input);

fs::path getOFRoot();
void setOFRoot(const fs::path & path);
void findandreplace( std::string& tInput, std::string tFind, std::string tReplace );
void findandreplaceInTexfile (const fs::path & fileName, std::string tFind, std::string tReplace );

bool doesTagAndAttributeExist(pugi::xml_document & doc, std::string tag, std::string attribute, std::string newValue);
pugi::xml_node appendValue(pugi::xml_document & doc, std::string tag, std::string attribute, std::string newValue, bool addMultiple = false);



void getFoldersRecursively(const fs::path & path, std::vector < std::string > & folderNames, std::string platform);
void getFoldersRecursively(const fs::path & path, std::vector < fs::path > & folderNames, std::string platform);
void getFilesRecursively(const fs::path & path, std::vector < std::string > & fileNames);
void getFilesRecursively(const fs::path & path, std::vector < fs::path > & fileNames);
void getLibsRecursively(const fs::path & path, std::vector < std::string > & libFiles, std::vector < LibraryBinary > & libLibs, std::string platform = "", std::string arch = "", std::string target = "");
void getFrameworksRecursively(const fs::path & path, std::vector < std::string > & frameworks,  std::string platform = "" );
void getPropsRecursively(const fs::path & path, std::vector < fs::path > & props, const std::string & platform);
void getDllsRecursively(const fs::path & path, std::vector < std::string > & dlls, std::string platform);


void splitFromLast(std::string toSplit, std::string deliminator, std::string & first, std::string & second);
void splitFromFirst(std::string toSplit, std::string deliminator, std::string & first, std::string & second);

void fixSlashOrder(std::string & toFix);
std::string unsplitString (std::vector < std::string > strings, std::string deliminator );

// FIXME: FS
//std::string getOFRelPath(const std::string & from);
//fs::path getOFRelPathFS(const fs::path & from);
fs::path getOFRelPath(const fs::path & from);

bool checkConfigExists();
bool askOFRoot();
std::string getOFRootFromConfig();

std::string getTargetString(ofTargetPlatform t);

std::unique_ptr<baseProject> getTargetProject(ofTargetPlatform targ);

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

std::string colorText(const std::string & s, int color = 32);
void alert(std::string msg, int color=32);

bool ofIsPathInPath(const fs::path& basePath, const fs::path& subPath);


// fs::path subtract(const fs::path & base, const fs::path & sub) {

// }

#endif /* UTILS_H_ */
