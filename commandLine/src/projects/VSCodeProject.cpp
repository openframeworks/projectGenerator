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

#include "json.hpp"
using json = nlohmann::json;

struct fileJson {
	fs::path fileName;
	json data;
	
	void load() {
	
		std::ifstream ifs(fileName);
		
		// this cause a bizarre issue.
//		std::string contents = ofBufferFromFile(fileName).getData();
		alert ("loading " + fileName.string(), 35);
		
		try {
			data = json::parse(ifs);
//			data = json::parse(contents);
		} catch (json::parse_error& ex) {
			ofLogError(VSCodeProject::LOG_NAME) << "JSON parse error at byte" << ex.byte;
		}
	}
	
	void save() {
		alert ("saving now " + fileName.string(), 33);
		std::cout << data.dump(1, '\t') << std::endl;
		
		std::ofstream jsonFile(fileName);
		try{
			jsonFile << data.dump(1, '\t');
		}catch(std::exception & e){
			ofLogError(VSCodeProject::LOG_NAME) << "Error saving json to " << fileName << ": " << e.what();
		}catch(...){
			ofLogError(VSCodeProject::LOG_NAME) << "Error saving json to " << fileName;
		}
	}
};


fileJson workspace;
fileJson cppProperties;

std::string VSCodeProject::LOG_NAME = "VSCodeProject";
bool VSCodeProject::createProjectFile(){
	workspace.fileName = projectDir / (projectName + ".code-workspace");
	cppProperties.fileName = projectDir / ".vscode/c_cpp_properties.json";
	
	// Copy all files from template, recursively
	try {
		fs::copy(templatePath, projectDir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	} catch(fs::filesystem_error& e) {
		ofLogError(LOG_NAME) << "error copying folder " << templatePath << " : " << projectDir << " : " << e.what();
		return false;
	}

	// Rename Project Workspace
	try {
		fs::rename(projectDir / "emptyExample.code-workspace", workspace.fileName);
	} catch(fs::filesystem_error& e) {
		ofLogError(LOG_NAME) << "error renaming folder " << " : " << workspace.fileName << " : " << e.what();
		return false;
	}
	return true;
}


bool VSCodeProject::loadProjectFile(){
	workspace.load();
	cppProperties.load();
	return true;
}


void VSCodeProject::addAddon(ofAddon & addon) {
	alert("VSCodeProject::addAddon() " + addon.name, 35);
	
	json object;
	object["path"] = "${workspaceRoot}/../" + addon.addonPath.string();
	json::json_pointer p = json::json_pointer("/folders");
	workspace.data[p].emplace_back( object );
	
//	alert ("will point to pointer", 36);
	json::json_pointer p2 = json::json_pointer("/env/PROJECT_ADDON_INCLUDES");
	if (!cppProperties.data[p2].is_array()) {
		cppProperties.data[p2] = json::array();
	}
	cppProperties.data[p2].emplace_back( "${workspaceRoot}/../" + addon.addonPath.string() );

	json::json_pointer p3 = json::json_pointer("/env/PROJECT_EXTRA_INCLUDES");
	if (!cppProperties.data[p3].is_array()) {
		cppProperties.data[p3] = json::array();
	}
	cppProperties.data[p3].emplace_back( "${workspaceRoot}/../" + addon.addonPath.string() );

	//	std::cout << cppProperties.data[p2].dump(1, '\t') << std::endl;
}


bool VSCodeProject::saveProjectFile(){
	alert("VSCodeProject::saveProjectFile() ");
	workspace.save();
	cppProperties.save();
	return true;
}


void VSCodeProject::addSrc(const fs::path & srcName, const fs::path & folder, SrcType type){
	alert ("addSrc " + srcName.string(), 33);
}

void VSCodeProject::addInclude(std::string includeName){
	alert ("addInclude " + includeName, 34);
//	ofLogNotice() << "adding include " << includeName;
}

void VSCodeProject::addLibrary(const LibraryBinary & lib){
	alert ("addLibrary " + lib.path, 35);
}
