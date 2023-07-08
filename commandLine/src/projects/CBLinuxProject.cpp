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
	// FIXME: FS
	ofDirectory dir(projectDir);
	if(!dir.exists()) dir.create(true);

	// FIXME: FS
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

	// FIXME: FS
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

	// FIXME: FS
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

	// FIXME: FS
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


	if (!fs::equivalent(getOFRoot(), "../../..")) {
		string root = getOFRoot().string();
		std::string root2 = root;
		// TODO: check this
//		root2.erase(root2.end()-1);
		findandreplaceInTexfile(projectDir / "Makefile", "../../..", root2);
		findandreplaceInTexfile(projectDir / "config.make", "../../..", root2);
		
		findandreplaceInTexfile(projectDir / (projectName + ".workspace"), "../../../", root);
		findandreplaceInTexfile(projectDir / (projectName + ".cbp"), "../../../", root);
	}

	return true;
}
