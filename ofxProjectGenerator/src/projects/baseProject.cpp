//
//  baseProject.cpp
//  projectGenerator
//
//  Created by molmol on 3/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "baseProject.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include "Utils.h"
#include "ofConstants.h"
using namespace std;

baseProject::baseProject(string _target){
    bLoaded = false;
    target = _target;
}

std::string baseProject::getPlatformTemplateDir(){
    return ofFilePath::join(getOFRoot(),"scripts/templates/" + target);
}

void recursiveTemplateCopy(const ofDirectory & templateDir, ofDirectory & projectDir){
    for(auto & f: templateDir){
        if(f.isDirectory()){
            ofDirectory templateSubDir(f.path());
            ofDirectory projectSubDir(ofFilePath::join(projectDir.path(),f.getFileName()));
            recursiveTemplateCopy(templateSubDir, projectSubDir);
        }else if(f.getFileName()!="template.config"){
            f.copyTo(ofFilePath::join(projectDir.path(),f.getFileName()),false,true);
        }
    }
}

bool isPlatformName(std::string file){
    for(int platform=OF_TARGET_OSX;platform<OF_TARGET_EMSCRIPTEN+1;platform++){
        if(file==getTargetString((ofTargetPlatform)platform)){
            return true;
        }
    }
    return false;
}

std::unique_ptr<baseProject::Template> baseProject::parseTemplate(const ofDirectory & templateDir){
    auto name = ofFilePath::getBaseName(templateDir.getOriginalDirectory());
    if(templateDir.isDirectory() && !isPlatformName(name)){
        ofBuffer templateconfig;
        ofFile templateconfigFile(ofFilePath::join(templateDir.path(), "template.config"));
        if(templateconfigFile.exists()){
            templateconfigFile >> templateconfig;
            auto supported = false;
            auto templateConfig = std::make_unique<Template>();
            templateConfig->dir = templateDir;
            templateConfig->name = name;
            for(auto line: templateconfig.getLines()){
                if(ofTrim(line).front() == '#') continue;
                auto varValue = ofSplitString(line,"+=",true,true);
                if(varValue.size() < 2) {
                    varValue = ofSplitString(line,"=",true,true);
                }
                if(varValue.size() < 2) continue;
                auto var = varValue[0];
                auto value = varValue[1];
                if(var=="PLATFORMS"){
                    auto platforms = ofSplitString(value," ",true,true);
                    for(auto platform: platforms){
                        if(platform==target){
                            supported = true;
                        }
                        templateConfig->platforms.push_back(platform);
                    }
                }else if(var=="DESCRIPTION"){
                    templateConfig->description = value;
                }else if(var=="RENAME"){
                    auto fromTo = ofSplitString(value,",");
                    if(fromTo.size()==2){
                        auto from = ofTrim(fromTo[0]);
                        auto to = ofTrim(fromTo[1]);
                        ofStringReplace(to,"${PROJECTNAME}",projectName);
                        templateConfig->renames[from] = to;
                    }
                }
            }
            if(supported){
                return templateConfig;
            }
        }
    }
    return std::unique_ptr<baseProject::Template>();
}

vector<baseProject::Template> baseProject::listAvailableTemplates(std::string target){
    vector<baseProject::Template> templates;
    ofDirectory templatesDir(ofFilePath::join(getOFRoot(),"scripts/templates"));
    for(auto & f: templatesDir.getSorted()){
        if(f.isDirectory()){
            auto templateConfig = parseTemplate(ofDirectory(f));
            if(templateConfig){
                templates.push_back(*templateConfig);
            }
        }
    }
    return templates;
}

bool baseProject::create(string path, std::string templateName){
    templatePath = getPlatformTemplateDir();
    addons.clear();

    if(!ofFilePath::isAbsolute(path)){
    	path = (std::filesystem::current_path() / std::filesystem::path(path)).string();
    }
    projectDir = ofFilePath::addTrailingSlash(path);
    projectName = ofFilePath::getFileName(path);
    bool bDoesDirExist = false;

    ofDirectory project(ofFilePath::join(projectDir,"src"));    // this is a directory, really?
    if(project.exists()){
        bDoesDirExist = true;
    }else{
        ofDirectory project(projectDir);
        ofDirectory(ofFilePath::join(templatePath,"src")).copyTo(ofFilePath::join(projectDir,"src"));
        ofDirectory(ofFilePath::join(templatePath,"bin")).copyTo(ofFilePath::join(projectDir,"bin"));
    }

    bool ret = createProjectFile();
    if(!ret) return false;

    if(templateName!=""){
        ofDirectory templateDir(ofFilePath::join(getOFRoot(),"scripts/templates/" + templateName));
        auto templateConfig = parseTemplate(templateDir);
        if(templateConfig){
            ofDirectory project(projectDir);
            recursiveTemplateCopy(templateDir,project);
            for(auto & rename: templateConfig->renames){
                auto from = (projectDir / rename.first).string();
                auto to = (projectDir / rename.second).string();
                ofFile(from).moveTo(to,true,true);
            }
        }else{
            ofLogWarning() << "Cannot find " << templateName << " using platform template only";
        }
    }

    ret = loadProjectFile();
    if(!ret) return false;

    parseConfigMake();

    if (bDoesDirExist){
        vector < string > fileNames;
        getFilesRecursively(ofFilePath::join(projectDir , "src"), fileNames);

        for (int i = 0; i < (int)fileNames.size(); i++){

            fileNames[i].erase(fileNames[i].begin(), fileNames[i].begin() + projectDir.length());

            string first, last;
#ifdef TARGET_WIN32
            splitFromLast(fileNames[i], "\\", first, last);
#else
            splitFromLast(fileNames[i], "/", first, last);
#endif
            if (fileNames[i] != "src/ofApp.cpp" &&
                fileNames[i] != "src/ofApp.h" &&
                fileNames[i] != "src/main.cpp" &&
                fileNames[i] != "src/ofApp.mm" &&
                fileNames[i] != "src/main.mm"){
                addSrc(fileNames[i], first);
            }
        }

//		if( target == "ios" ){
//			getFilesRecursively(ofFilePath::join(projectDir , "bin/data"), fileNames);
//
//	        for (int i = 0; i < (int)fileNames.size(); i++){
//				fileNames[i].erase(fileNames[i].begin(), fileNames[i].begin() + projectDir.length());
//
//				string first, last;
//				splitFromLast(fileNames[i], "/", first, last);
//				if (fileNames[i] != "Default.png" &&
//					fileNames[i] != "src/ofApp.h" &&
//					fileNames[i] != "src/main.cpp" &&
//					fileNames[i] != "src/ofApp.mm" &&
//					fileNames[i] != "src/main.mm"){
//					addSrc(fileNames[i], first);
//				}
//			}
//		}

        // get a unique list of the paths that are needed for the includes.
        list < string > paths;
        vector < string > includePaths;
        for (int i = 0; i < (int)fileNames.size(); i++){
            size_t found;
    #ifdef TARGET_WIN32
            found = fileNames[i].find_last_of("\\");
    #else
            found = fileNames[i].find_last_of("/");
    #endif
            paths.push_back(fileNames[i].substr(0,found));
        }

        paths.sort();
        paths.unique();
        for (list<string>::iterator it=paths.begin(); it!=paths.end(); ++it){
            includePaths.push_back(*it);
        }

        for (int i = 0; i < includePaths.size(); i++){
            addInclude(includePaths[i]);
        }
    }
    return true;
}

bool baseProject::save(){
    ofLog(OF_LOG_NOTICE) << "saving addons.make";
    ofFile addonsMake(ofFilePath::join(projectDir,"addons.make"), ofFile::WriteOnly);
    for(int i = 0; i < addons.size(); i++){
        if(addons[i].isLocalAddon){
            addonsMake << std::filesystem::path(addons[i].addonPath).generic_string() << endl;
        }else{
            addonsMake << addons[i].name << endl;
        }
    }

	return saveProjectFile();
}

void baseProject::addAddon(std::string addonName){
    ofAddon addon;
    addon.pathToOF = getOFRelPath(projectDir);
    addon.pathToProject = ofFilePath::getAbsolutePath(projectDir);
    

    
    auto localPath = ofFilePath::join(addon.pathToProject, addonName);
    
    if (ofDirectory(addonName).exists()){
        // if it's an absolute path, convert to relative...
        string relativePath = ofFilePath::makeRelative(addon.pathToProject, addonName);
        addonName = relativePath;
        addon.isLocalAddon = true;
        addon.fromFS(addonName, target);
    } else if(ofDirectory(localPath).exists()){
        addon.isLocalAddon = true;
        addon.fromFS(addonName, target);
    }else{
        addon.isLocalAddon = false;
        auto standardPath = ofFilePath::join(ofFilePath::join(getOFRoot(), "addons"), addonName);
        addon.fromFS(standardPath, target);
    }
    addAddon(addon);
}

void baseProject::addAddon(ofAddon & addon){
    for(int i=0;i<(int)addons.size();i++){
		if(addons[i].name==addon.name) return;
	}

	addons.push_back(addon);

    for(int i=0;i<(int)addon.includePaths.size();i++){
        ofLogVerbose() << "adding addon include path: " << addon.includePaths[i];
        addInclude(addon.includePaths[i]);
    }
    for(int i=0;i<(int)addon.libs.size();i++){
        ofLogVerbose() << "adding addon libs: " << addon.libs[i].path;
        addLibrary(addon.libs[i]);
    }
    for(int i=0;i<(int)addon.cflags.size();i++){
        ofLogVerbose() << "adding addon cflags: " << addon.cflags[i];
        addCFLAG(addon.cflags[i]);
    }
	for(int i=0;i<(int)addon.cppflags.size();i++){
	    ofLogVerbose() << "adding addon cppflags: " << addon.cppflags[i];
        addCPPFLAG(addon.cppflags[i]);
    }
    for(int i=0;i<(int)addon.ldflags.size();i++){
        ofLogVerbose() << "adding addon ldflags: " << addon.ldflags[i];
        addLDFLAG(addon.ldflags[i]);
    }
    for(int i=0;i<(int)addon.srcFiles.size(); i++){
        ofLogVerbose() << "adding addon srcFiles: " << addon.srcFiles[i];
        addSrc(addon.srcFiles[i],addon.filesToFolders[addon.srcFiles[i]]);
    }
    for(int i=0;i<(int)addon.csrcFiles.size(); i++){
        ofLogVerbose() << "adding addon c srcFiles: " << addon.csrcFiles[i];
        addSrc(addon.csrcFiles[i],addon.filesToFolders[addon.csrcFiles[i]],C);
    }
    for(int i=0;i<(int)addon.cppsrcFiles.size(); i++){
        ofLogVerbose() << "adding addon cpp srcFiles: " << addon.cppsrcFiles[i];
        addSrc(addon.cppsrcFiles[i],addon.filesToFolders[addon.cppsrcFiles[i]],CPP);
    }
    for(int i=0;i<(int)addon.objcsrcFiles.size(); i++){
        ofLogVerbose() << "adding addon objc srcFiles: " << addon.objcsrcFiles[i];
        addSrc(addon.objcsrcFiles[i],addon.filesToFolders[addon.objcsrcFiles[i]],OBJC);
    }
    for(int i=0;i<(int)addon.headersrcFiles.size(); i++){
        ofLogVerbose() << "adding addon header srcFiles: " << addon.headersrcFiles[i];
        addSrc(addon.headersrcFiles[i],addon.filesToFolders[addon.headersrcFiles[i]],HEADER);
    }
}

void baseProject::parseAddons(){
	ofFile addonsMake(ofFilePath::join(projectDir,"addons.make"));
	ofBuffer addonsMakeMem;
	addonsMake >> addonsMakeMem;
	for(auto line: addonsMakeMem.getLines()){
	    auto addon = ofTrim(line);
	    if(addon[0] == '#') continue;
        if(addon == "") continue;
        addAddon(addon);
	}
}

void baseProject::parseConfigMake(){
    ofFile configMake(ofFilePath::join(projectDir,"config.make"));
    ofBuffer configMakeMem;
    configMake >> configMakeMem;
    for(auto line: configMakeMem.getLines()){
        auto config = ofTrim(line);
        if(config[0] == '#') continue;
        if(config == "") continue;
        if(config.find("=")!=std::string::npos){
            auto varValue = ofSplitString(config,"=",true,true);
            if(varValue.size()>1){
				auto var = ofTrim(varValue[0]);
				auto value = ofTrim(varValue[1]);
				if (var=="PROJECT_AFTER_OSX" && target=="osx"){
					addAfterRule(value);
				}
            }
        }
    }

}
