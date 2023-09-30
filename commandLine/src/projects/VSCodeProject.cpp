/*
 * VSCodeProject.cpp
 *
 *  Created on: 28/09/2023
 *      Author: Dimitre Lima
 */

#include "VSCodeProject.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include "Utils.h"

//#include <iostream>
//#include <nlohmann/json.hpp>

#include "json.hpp"
using json = nlohmann::json;

struct key_value_t
{
  std::string path;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(key_value_t, path);

json j;

std::string VSCodeProject::LOG_NAME = "VSCodeProject";
bool VSCodeProject::createProjectFile(){
//	alert("VSCodeProject::createProjectFile() ");

	auto projectWorkspace { projectDir / (projectName + ".code-workspace") };
	alert ("projectDir " + projectDir.string(), 33);
	alert ("templatePath " + templatePath.string(), 34);
	
	try {
		fs::copy(templatePath, projectDir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	} catch(fs::filesystem_error& e) {
		ofLogError(LOG_NAME) << "error copying folder " << templatePath << " : " << projectDir << " : " << e.what();
		return false;
	}

	try {
		fs::rename(projectDir / "emptyExample.code-workspace", projectWorkspace);
	} catch(fs::filesystem_error& e) {
		ofLogError(LOG_NAME) << "error renaming folder " << " : " << projectWorkspace << " : " << e.what();
		return false;
	}

	std::string contents = ofBufferFromFile(projectWorkspace).getData();
	j = json::parse(contents);
	return true;
}


bool VSCodeProject::loadProjectFile(){
//	alert("VSCodeProject::loadProjectFile() ");
	json::json_pointer p = json::json_pointer("/folders");
	return true;
}


void VSCodeProject::addAddon(ofAddon & addon) {
	key_value_t kv1{ "${workspaceRoot}/../" + addon.addonPath.string() };
	json::json_pointer p = json::json_pointer("/folders");
	j[p].emplace_back(kv1);
}


bool VSCodeProject::saveProjectFile(){
//	alert("VSCodeProject::saveProjectFile() ");
	std::cout << j.dump(1, '\t') << std::endl;
	
	auto fileName { projectDir / (projectName + ".code-workspace") };
	std::ofstream jsonFile(fileName);
	try{
		jsonFile << j.dump(1, '\t');
	}catch(std::exception & e){
		ofLogError(LOG_NAME) << "Error saving json to " << fileName << ": " << e.what();
		return false;
	}catch(...){
		ofLogError(LOG_NAME) << "Error saving json to " << fileName;
		return false;
	}
	
	return true;
}


//
//void VSCodeProject::addSrc(const fs::path & srcName, const fs::path & folder, SrcType type){
////	alert ("addSrc " + srcName.string(), 35);
//}
//
//void VSCodeProject::addInclude(std::string includeName){
////	alert ("addInclude", 35);
////	ofLogNotice() << "adding include " << includeName;
//}
//
//void VSCodeProject::addLibrary(const LibraryBinary & lib){
////	alert ("addLibrary", 35);
//}
