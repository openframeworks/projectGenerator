/*
 * zedProject.cpp
 *
 *  Created on: 04/01/2025
 *      Author: Dimitre Lima
 */

#include "zedProject.h"
#include "ofLog.h"
#include "Utils.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

std::string zedProject::LOG_NAME = "zedProject";

bool zedProject::createProjectFile(){
	try {
		fs::copy(templatePath / ".zed", projectDir / ".zed", fs::copy_options::update_existing | fs::copy_options::recursive);
	} catch(fs::filesystem_error& e) {
		ofLogError(LOG_NAME) << "error copying folder " << templatePath.string() << " : " << projectDir.string() << " : " << e.what();
		return false;
	}

	copyTemplateFiles.push_back({ fs::path { templatePath / "compile_flags.txt" },
		fs::path { projectDir / "compile_flags.txt" }
	});

	copyTemplateFiles.push_back({ fs::path { templatePath / "Makefile" },
		fs::path { projectDir / "Makefile" }
	});
	copyTemplateFiles.push_back({ fs::path { templatePath / "config.make" },
		fs::path { projectDir / "config.make" }
	});
	return true;
}


bool zedProject::loadProjectFile(){
	// workspace.load();
	// cppProperties.load();
	return true;
}

bool zedProject::saveProjectFile(){
    // workspace.data["openFrameworksProjectGeneratorVersion"] = getPGVersion();

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

void zedProject::addAddonBegin(const ofAddon& addon) {
	// alert("zedProject::addAddon() " + addon.name, 35);
//	std::string inc { "-I" + ofPathToString(addon.addonPath) };
//	copyTemplateFiles[0].appends.emplace_back(inc);
}


void zedProject::addSrc(const fs::path & srcName, const fs::path & folder, SrcType type){
//	alert ("addSrc " + srcName.string(), 33);
}

void zedProject::addInclude(const fs::path & includeName){
	// alert ("addInclude " + ofPathToString(includeName), 34);
	std::string inc { "-I" + ofPathToString(includeName) };
	copyTemplateFiles[0].appends.emplace_back(inc);
	// cppProperties.addToArray("/env/PROJECT_EXTRA_INCLUDES", includeName);
}

void zedProject::addLibrary(const LibraryBinary & lib){
//	alert ("addLibrary " + lib.path, 35);
}
