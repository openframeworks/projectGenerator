/*
 * CBLinuxProject.cpp
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#include "CBLinuxProject.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include "Utils.h"

std::string CBLinuxProject::LOG_NAME = "CBLinuxProject";

bool CBLinuxProject::createProjectFile(){
	ofDirectory dir(projectDir);
	if(!dir.exists()) dir.create(true);

	ofFile project(ofFilePath::join(projectDir, projectName + ".cbp"));
	std::string src =  ofFilePath::join(templatePath,"emptyExample_" + target + ".cbp");
	std::string dst = project.path();
	bool ret;

	if(!project.exists()){
		ret = ofFile::copyFromTo(src,dst);
		if(!ret){
			ofLogError(LOG_NAME) << "error copying cbp template from " << src << " to " << dst;
			return false;
		}else{
			findandreplaceInTexfile(dst, "emptyExample", projectName);
		}
	}

	ofFile workspace(ofFilePath::join(projectDir, projectName + ".workspace"));
	if(!workspace.exists()){
		src = ofFilePath::join(templatePath,"emptyExample_" + target + ".workspace");
		dst = workspace.path();
		ret = ofFile::copyFromTo(src,dst);
		if(!ret){
			ofLogError(LOG_NAME) << "error copying workspace template from "<< src << " to " << dst;
			return false;
		}else{
			findandreplaceInTexfile(dst, "emptyExample", projectName);
		}
	}

	ofFile makefile(ofFilePath::join(projectDir,"Makefile"));
	if(!makefile.exists()){
		src = ofFilePath::join(templatePath,"Makefile");
		dst = makefile.path();
		ret = ofFile::copyFromTo(src,dst);
		if(!ret){
			ofLogError(LOG_NAME) << "error copying Makefile template from " << src << " to " << dst;
			return false;
		}
	}

	ofFile config(ofFilePath::join(projectDir,"config.make"));
	if(!config.exists()){
		src = ofFilePath::join(templatePath,"config.make");
		dst = config.path();
		ret = ofFile::copyFromTo(src,dst);
		if(!ret){
			ofLogError(LOG_NAME) << "error copying config.make template from " << src << " to " << dst;
			return false;
		}
	}


	// handle the relative roots.
	// FIXME: FS
	fs::path relRoot = getOFRelPath(projectDir);
	if (!fs::equivalent(relRoot, "../../..")) {
		std::string relPath2 = relRoot.string();
		relPath2.erase(relPath2.end()-1);
		findandreplaceInTexfile(projectDir / "Makefile", "../../..", relPath2);
		findandreplaceInTexfile(projectDir / "config.make", "../../..", relPath2);
		findandreplaceInTexfile(ofFilePath::join(projectDir , projectName + ".workspace"), "../../../", relRoot.string());
		findandreplaceInTexfile(ofFilePath::join(projectDir , projectName + ".cbp"), "../../../", relRoot.string());
	}

	return true;
}
