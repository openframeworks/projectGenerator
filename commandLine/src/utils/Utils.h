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

std::string generateUUID(std::string input);

fs::path getOFRoot();
void setOFRoot(const fs::path & path);
void findandreplace( std::string& tInput, std::string tFind, std::string tReplace );
void findandreplaceInTexfile (const of::filesystem::path & fileName, std::string tFind, std::string tReplace );

bool doesTagAndAttributeExist(pugi::xml_document & doc, std::string tag, std::string attribute, std::string newValue);
pugi::xml_node appendValue(pugi::xml_document & doc, std::string tag, std::string attribute, std::string newValue, bool addMultiple = false);



void getFoldersRecursively(const of::filesystem::path & path, std::vector < std::string > & folderNames, std::string platform);
void getFoldersRecursively(const of::filesystem::path & path, std::vector < of::filesystem::path > & folderNames, std::string platform);
void getFilesRecursively(const of::filesystem::path & path, std::vector < std::string > & fileNames);
void getLibsRecursively(const of::filesystem::path & path, std::vector < std::string > & libFiles, std::vector < LibraryBinary > & libLibs, std::string platform = "", std::string arch = "", std::string target = "");
void getFrameworksRecursively(const of::filesystem::path & path, std::vector < std::string > & frameworks,  std::string platform = "" );
void getPropsRecursively(const of::filesystem::path & path, std::vector < std::string > & props, const std::string & platform);
void getDllsRecursively(const of::filesystem::path & path, std::vector < std::string > & dlls, std::string platform);


void splitFromLast(std::string toSplit, std::string deliminator, std::string & first, std::string & second);
void splitFromFirst(std::string toSplit, std::string deliminator, std::string & first, std::string & second);

void fixSlashOrder(std::string & toFix);
std::string unsplitString (std::vector < std::string > strings, std::string deliminator );

// FIXME: FS
std::string getOFRelPath(const std::string & from);
of::filesystem::path getOFRelPathFS(const of::filesystem::path & from);

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



#endif /* UTILS_H_ */
