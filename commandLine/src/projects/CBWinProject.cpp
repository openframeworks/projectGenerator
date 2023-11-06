/*
 * CBWinProject.cpp
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#include "CBWinProject.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include "Utils.h"

std::string CBWinProject::LOG_NAME = "CBWinProject";

bool CBWinProject::createProjectFile(){

	auto project = projectDir / (projectName + ".cbp");
	auto workspace = projectDir / (projectName + ".workspace");

	vector < std::pair <fs::path, fs::path > > fromTo {
		{ templatePath / "emptyExample.cbp",   		projectDir / (projectName + ".cbp") },
		{ templatePath / "emptyExample.workspace", 	projectDir / (projectName + ".workspace") },
		{ templatePath / "icon.rc", 	projectDir / "icon.rc" },
	};

	for (auto & p : fromTo) {
		try {
			fs::copy_file(p.first, p.second, fs::copy_options::overwrite_existing);
		} catch(fs::filesystem_error& e) {
			ofLogError(LOG_NAME) << "error copying template file " << p.first << " : " << p.second << e.what();
			return false;
		}
	}

	// Calculate OF Root in relation to each project (recursively);
	auto relRoot = fs::relative((fs::current_path() / getOFRoot()), projectDir);

	if (!fs::equivalent(relRoot, "../../..")) {
		string root = relRoot.string();

		// let's make it windows friendly:
		std::string relRootWindows = convertStringToWindowsSeparator(root);

		findandreplaceInTexfile(workspace, "../../../", root);
		findandreplaceInTexfile(project, "../../../", root);

		findandreplaceInTexfile(workspace, "..\\..\\..\\", relRootWindows);
		findandreplaceInTexfile(project, "..\\..\\..\\", relRootWindows);
	}
	return true;
}

bool CBWinProject::loadProjectFile(){
	fs::path project { projectDir / (projectName + ".cbp") };
	if (!fs::exists(project)) {
		ofLogError(LOG_NAME) << "error loading" << project << "doesn't exist";
		return false;
	}
	pugi::xml_parse_result result = doc.load_file(project.c_str());
	bLoaded =result.status==pugi::status_ok;
	return bLoaded;
}

bool CBWinProject::saveProjectFile(){
	auto workspace = projectDir / (projectName + ".workspace");
	findandreplaceInTexfile(workspace, "emptyExample", projectName);
	pugi::xpath_node_set title = doc.select_nodes("//Option[@title]");
	if(!title.empty()){
		if(!title[0].node().attribute("title").set_value(projectName.c_str())){
			ofLogError(LOG_NAME) << "can't set title";
		}
	}
	fs::path project { projectDir / (projectName + ".cbp") };
	return doc.save_file(project.c_str());
}

void CBWinProject::addSrc(const fs::path & srcName, const fs::path & folder, SrcType type){
	pugi::xml_node node = appendValue(doc, "Unit", "filename", srcName.string());
	if(!node.empty()){
		node.child("Option").attribute("virtualFolder").set_value(folder.c_str());
	}
}

void CBWinProject::addInclude(std::string includeName){
	ofLogNotice() << "adding include " << includeName;
	appendValue(doc, "Add", "directory", includeName);
}

void CBWinProject::addLibrary(const LibraryBinary & lib){
	appendValue(doc, "Add", "library", lib.path, true);
	// overwriteMultiple for a lib if it's there (so libsorder.make will work)
	// this is because we might need to say libosc, then ws2_32
}

