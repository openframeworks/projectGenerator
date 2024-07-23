/*
 * VSCodeProject.cpp
 *
 *  Created on: 28/09/2023
 *      Author: Dimitre Lima
 */

#include "VSCodeProject.h"
#include "ofLog.h"
#include "Utils.h"
#if !defined(TARGET_MINGW)
	#include <json.hpp>
#else
	#include <nlohmann/json.hpp> // MSYS2 : use of system-installed include
#endif


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
		if (!fs::exists(fileName)) {
			ofLogError(VSCodeProject::LOG_NAME) << "JSON file not found " << fileName.string();
			return;
		}
		
		std::ifstream ifs(fileName);
		try {
			data = json::parse(ifs);
		} catch (json::parse_error& ex) {
			ofLogError(VSCodeProject::LOG_NAME) << "JSON parse error at byte" << ex.byte;
			ofLogError(VSCodeProject::LOG_NAME) << "fileName" << fileName.string();
		}
	}

	void save() {
//		alert ("saving now " + fileName.string(), 33);
//		std::cout << data.dump(1, '\t') << std::endl;
		std::ofstream jsonFile(fileName);
		try {
			jsonFile << data.dump(1, '\t');
		} catch(std::exception & e) {
			ofLogError(VSCodeProject::LOG_NAME) << "Error saving json to " << fileName.string() << ": " << e.what();
		}
	}
};

fileJson workspace;
fileJson cppProperties;
std::string VSCodeProject::LOG_NAME = "VSCodeProject";

bool VSCodeProject::createProjectFile(){

#if defined(__MINGW32__) || defined(__MINGW64__)
	try {
		fs::remove_all(projectDir / ".vscode");
	} catch(fs::filesystem_error& e) {
		ofLogError(LOG_NAME) << "error removing folder .vscode " << e.what();
		return false;
	}
#endif
	
	createBackup(projectDir / ".vscode");
	try {
		fs::copy(templatePath / ".vscode", projectDir / ".vscode", fs::copy_options::update_existing | fs::copy_options::recursive);
	} catch(fs::filesystem_error& e) {
		ofLogError(LOG_NAME) << "error copying folder " << templatePath.string() << " : " << projectDir.string() << " : " << e.what();
		return false;
	}
	
	
	workspace.fileName = fs::path {
		projectDir / (projectName + ".code-workspace")};
	cppProperties.fileName = fs::path {
		projectDir / ".vscode/c_cpp_properties.json"
	};
	
	copyTemplateFiles.push_back({ fs::path { templatePath / "Makefile" },
		fs::path { projectDir / "Makefile" }
	});
	copyTemplateFiles.push_back({ fs::path { templatePath / "config.make" },
		fs::path { projectDir / "config.make" }
	});
	copyTemplateFiles.push_back({ fs::path { templatePath / "emptyExample.code-workspace" },
		fs::path { projectDir / workspace.fileName }
	});

	for (auto & c : copyTemplateFiles) {
		try {
			c.run();
		} catch (const std::exception& e) {
			std::cerr << "Error running copy template files: " << e.what() << std::endl;
			return false;
		}
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
	workspace.data["openFrameworksProjectGeneratorVersion"] = getPGVersion();
	workspace.save();
	cppProperties.save();
	return true;
}


void VSCodeProject::addSrc(const fs::path & srcName, const fs::path & folder, SrcType type){
//	alert ("addSrc " + srcName.string(), 33);
}

void VSCodeProject::addInclude(const fs::path & includeName){
//	alert ("addInclude " + includeName, 34);
	cppProperties.addToArray("/env/PROJECT_EXTRA_INCLUDES", includeName);
}

void VSCodeProject::addLibrary(const LibraryBinary & lib){
//	alert ("addLibrary " + lib.path, 35);
}
