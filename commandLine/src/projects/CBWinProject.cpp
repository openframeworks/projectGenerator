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


	ofFile::copyFromTo(ofFilePath::join(templatePath,"emptyExample.cbp"),project, false, true);

	ofFile::copyFromTo(ofFilePath::join(templatePath,"emptyExample.workspace"),workspace, false, true);
	ofFile::copyFromTo(ofFilePath::join(templatePath,"icon.rc"), projectDir / "icon.rc", false, true);

	//let's do some renaming:
	// FIXME: FS
	std::string relRoot = getOFRelPath(ofFilePath::removeTrailingSlash(projectDir));

	if (relRoot != "../../../"){

		std::string relRootWindows = relRoot;
		// let's make it windows friendly:
		for(int i = 0; i < relRootWindows.length(); i++) {
			if( relRootWindows[i] == '/' )
				relRootWindows[i] = '\\';
		}

		findandreplaceInTexfile(workspace, "../../../", relRoot);
		findandreplaceInTexfile(project, "../../../", relRoot);

		findandreplaceInTexfile(workspace, "..\\..\\..\\", relRootWindows);
		findandreplaceInTexfile(project, "..\\..\\..\\", relRootWindows);
	}

	return true;
}

bool CBWinProject::loadProjectFile(){

	//project.open(ofFilePath::join(projectDir , projectName + ".cbp"));

	ofFile project(projectDir / (projectName + ".cbp"));
	if(!project.exists()){
		ofLogError(LOG_NAME) << "error loading" << project.path() << "doesn't exist";
		return false;
	}
	pugi::xml_parse_result result = doc.load(project);
	bLoaded =result.status==pugi::status_ok;
	return bLoaded;
}

bool CBWinProject::saveProjectFile(){

	findandreplaceInTexfile(ofFilePath::join(projectDir , projectName + ".workspace"),"emptyExample",projectName);
	pugi::xpath_node_set title = doc.select_nodes("//Option[@title]");
	if(!title.empty()){
		if(!title[0].node().attribute("title").set_value(projectName.c_str())){
			ofLogError(LOG_NAME) << "can't set title";
		}
	}
	return doc.save_file((projectDir / (projectName + ".cbp")).c_str());
}

void CBWinProject::addSrc(std::string srcName, std::string folder, SrcType type){
	pugi::xml_node node = appendValue(doc, "Unit", "filename", srcName);
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

std::string CBWinProject::getName(){
	return projectName;
}

of::filesystem::path CBWinProject::getPath(){
	return projectDir;
}
