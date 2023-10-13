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
	
	// only works for workspace
	void addPath(fs::path folder) {
		std::string path = folder.is_absolute() ? folder.string() : "${workspaceRoot}/../" + folder.string();
		json object;
		object["path"] = path;
		json::json_pointer p = json::json_pointer("/folders");
		data[p].emplace_back( object );
	}
	
	void addToArray(string pointer, fs::path value) {
		json::json_pointer p = json::json_pointer(pointer);
		if (!data[p].is_array()) {
			data[p] = json::array();
		}
		
		std::string path = fs::path(value).is_absolute() ? value.string() : "${workspaceRoot}/" + value.string();
		data[p].emplace_back( path );
	}
	
	void load() {
		std::ifstream ifs(fileName);
		try {
			data = json::parse(ifs);
		} catch (json::parse_error& ex) {
			ofLogError(VSCodeProject::LOG_NAME) << "JSON parse error at byte" << ex.byte;
		}
	}
	
	void save() {
//		alert ("saving now " + fileName.string(), 33);
//		std::cout << data.dump(1, '\t') << std::endl;
		std::ofstream jsonFile(fileName);
		try {
			jsonFile << data.dump(1, '\t');
		} catch(std::exception & e) {
			ofLogError(VSCodeProject::LOG_NAME) << "Error saving json to " << fileName << ": " << e.what();
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
//	alert("VSCodeProject::addAddon() " + addon.name, 35);

	workspace.addPath(addon.addonPath);
	
	// examples of how to add entries to json arrays
//	cppProperties.addToArray("/env/PROJECT_ADDON_INCLUDES", addon.addonPath);
//	cppProperties.addToArray("/env/PROJECT_EXTRA_INCLUDES", addon.addonPath);

}


bool VSCodeProject::saveProjectFile(){
//	alert("VSCodeProject::saveProjectFile() ");
//	alert("--- VSCodeProject::extSrcPaths() ");
//	for (auto & e : extSrcPaths) {
//		cout << e << endl;
//		workspace.addPath(e);
//
//	}
//	alert("--- VSCodeProject::extSrcPaths() ");

	
	workspace.data["openFrameworksProjectGeneratorVersion"] = getPGVersion();
	
	workspace.save();
	cppProperties.save();
	return true;
}


void VSCodeProject::addSrc(const fs::path & srcName, const fs::path & folder, SrcType type){
//	alert ("addSrc " + srcName.string(), 33);
}

void VSCodeProject::addInclude(std::string includeName){
//	alert ("addInclude " + includeName, 34);
	cppProperties.addToArray("/env/PROJECT_EXTRA_INCLUDES", fs::path(includeName));
}

void VSCodeProject::addLibrary(const LibraryBinary & lib){
//	alert ("addLibrary " + lib.path, 35);
}
