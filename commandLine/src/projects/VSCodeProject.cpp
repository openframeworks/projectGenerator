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


std::string VSCodeProject::LOG_NAME = "VSCodeProject";
bool VSCodeProject::createProjectFile(){

	auto projectPath = projectDir / (projectName + ".code-workspace");
	cout << projectPath << endl;
	cout << "templatePath " << templatePath << endl;
	
	std::string contents = R"({
 "folders": [
		{
			"path": "."
		},
		{
			"path": "${workspaceRoot}/../../../../libs/openFrameworks"
		},
		{
			"path": "${workspaceRoot}/../../../../addons"
		}
	],
	"settings": {}
})";
	
	json j = json::parse(contents);
	json::json_pointer p = json::json_pointer("/folders");
//	string valor = ;
	for (int a=0; a<3; a++) {
		key_value_t kv1{ "${workspaceRoot}/../../../../ARWIL" + ofToString(a) };
		j[p].emplace_back(kv1);
	}

	std::cout << j.dump(1, '\t') << std::endl;

	return true;
}

bool VSCodeProject::loadProjectFile(){
	cout << "VSCodeProject::loadProjectFile() " << endl;
	return true;
//	fs::path project { projectDir / (projectName + ".cbp") };
//	if (!fs::exists(project)) {
//		ofLogError(LOG_NAME) << "error loading" << project << "doesn't exist";
//		return false;
//	}
//	pugi::xml_parse_result result = doc.load_file(project.c_str());
//	bLoaded =result.status==pugi::status_ok;
//	return bLoaded;
	
}

bool VSCodeProject::saveProjectFile(){
	cout << "VSCodeProject::saveProjectFile() " << endl;
	return true;

//	auto workspace = projectDir / (projectName + ".workspace");
//	findandreplaceInTexfile(workspace, "emptyExample", projectName);
//	pugi::xpath_node_set title = doc.select_nodes("//Option[@title]");
//	if(!title.empty()){
//		if(!title[0].node().attribute("title").set_value(projectName.c_str())){
//			ofLogError(LOG_NAME) << "can't set title";
//		}
//	}
//	fs::path project { projectDir / (projectName + ".cbp") };
//	return doc.save_file(project.c_str());
}

void VSCodeProject::addSrc(const fs::path & srcName, const fs::path & folder, SrcType type){
	alert ("addSrc " + srcName.string(), 35);

//	pugi::xml_node node = appendValue(doc, "Unit", "filename", srcName.string());
//	if(!node.empty()){
//		node.child("Option").attribute("virtualFolder").set_value(folder.c_str());
//	}
}

void VSCodeProject::addInclude(std::string includeName){
	alert ("addInclude", 35);
	ofLogNotice() << "adding include " << includeName;
//	appendValue(doc, "Add", "directory", includeName);
}

void VSCodeProject::addLibrary(const LibraryBinary & lib){
	alert ("addLibrary", 35);
//	appendValue(doc, "Add", "library", lib.path, true);
	// overwriteMultiple for a lib if it's there (so libsorder.make will work)
	// this is because we might need to say libosc, then ws2_32
}

//std::string VSCodeProject::getName(){
//	return projectName;
//}
//
//fs::path VSCodeProject::getPath(){
//	return projectDir;
//}
