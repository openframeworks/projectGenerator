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
#include <list>

using std::string;
using std::vector;
//using std::cout;
//using std::endl;
namespace fs = of::filesystem;

const std::string templatesFolder = "scripts/templates/";

baseProject::baseProject(std::string _target){
	bLoaded = false;
	target = _target;
}

std::string baseProject::getPlatformTemplateDir(){
	return ofFilePath::join(getOFRoot(),templatesFolder + target);
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
	auto name = fs::path(templateDir.getOriginalDirectory()).parent_path().filename();
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

std::vector<baseProject::Template> baseProject::listAvailableTemplates(std::string target){
	std::vector<baseProject::Template> templates;

	ofDirectory templatesDir(ofFilePath::join(getOFRoot(),templatesFolder));
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

bool baseProject::create(const fs::path & _path, std::string templateName){
	templatePath = getPlatformTemplateDir();
	addons.clear();
	extSrcPaths.clear();
	auto path = _path;

	if(!ofFilePath::isAbsolute(path)){
		path = (fs::current_path() / fs::path(path)).string();
	}
	projectDir = path;

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
		auto name = ofFilePath::join(getOFRoot(),templatesFolder + templateName);
		ofDirectory templateDir(ofFilePath::join(getOFRoot(),templatesFolder + templateName));
		templateDir.setShowHidden(true);
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

		std::vector < string > fileNames;
		getFilesRecursively(projectDir / "src", fileNames);

		for (auto & f : fileNames) {
			fs::path rel { fs::relative(f, projectDir) };
			fs::path folder { rel.parent_path() };

			std::string fileName = rel.string();

			if (fileName != "src/ofApp.cpp" &&
				fileName != "src/ofApp.h" &&
				fileName != "src/main.cpp" &&
				fileName != "src/ofApp.mm" &&
				fileName != "src/main.mm") {
//				cout << "add filename:: " << rel << " :: " << folder << endl;
				addSrc(rel.string(), folder.string());
			} else {
//				cout << "not adding filename:: " << rel << " :: " << folder << endl;
			}
		}

		// only add unique paths
		std::vector < fs::path > paths;
		for (auto & f : fileNames) {
			auto dir = fs::path(f).parent_path().filename();
			if (std::find(paths.begin(), paths.end(), dir) == paths.end()) {
				paths.emplace_back(dir);
//				cout << "addInclude " << dir << endl;
				addInclude(dir.string());
			}
		}

	}
	return true;
}

bool baseProject::save(){
	ofLog(OF_LOG_NOTICE) << "saving addons.make";
	ofFile addonsMake(ofFilePath::join(projectDir,"addons.make"), ofFile::WriteOnly);
	for(int i = 0; i < addons.size(); i++){
		if(addons[i].isLocalAddon){
			addonsMake << fs::path(addons[i].addonPath).generic_string() << std::endl;
		}else{
			addonsMake << addons[i].name << std::endl;
		}
	}

	//save out params which the PG knows about to config.make
	//we mostly use this right now for storing the external source paths
	auto buffer = ofBufferFromFile(ofFilePath::join(projectDir,"config.make"));
	if( buffer.size() ){
		ofFile saveConfig(ofFilePath::join(projectDir,"config.make"), ofFile::WriteOnly);

		for(auto line : buffer.getLines()){
			string str = line;

			//add the of root path
			if( str.rfind("# OF_ROOT =", 0) == 0 ){
   
                            auto path = getOFRoot();
                            if( projectDir.string().rfind(getOFRoot(),0) == 0 ){
                                path = getOFRelPath(projectDir);
                            }
                            
                            saveConfig << "OF_ROOT = " << path << std::endl;
			}
			// replace this section with our external paths
			else if( extSrcPaths.size() && str.rfind("# PROJECT_EXTERNAL_SOURCE_PATHS =", 0) == 0 ){

				for(int d = 0; d < extSrcPaths.size(); d++){
					ofLog(OF_LOG_VERBOSE) << " adding PROJECT_EXTERNAL_SOURCE_PATHS to config" << extSrcPaths[d] << std::endl;
					saveConfig << "PROJECT_EXTERNAL_SOURCE_PATHS" << (d == 0 ? " = " : " += ") << extSrcPaths[d] << std::endl;
				}

			}else{
			   saveConfig << str << std::endl;
			}
		}
	}

	return saveProjectFile();
}

bool baseProject::isAddonInCache(const std::string & addonPath, const std::string platform){
	auto it = addonsCache.find(platform);
	if (it == addonsCache.end()) return false;
	auto it2 = it->second.find(addonPath);
	return it2 != it->second.end();
}

using std::cout;
using std::endl;
void baseProject::addAddon(std::string addonName){
	ofAddon addon;
//	cout << projectDir << endl;
	addon.pathToOF = getOFRelPath(projectDir.string());
	addon.pathToProject = ofFilePath::getAbsolutePath(projectDir);

	bool addonOK = false;

	bool inCache = isAddonInCache(addonName, target);
	//inCache = false; //to test no-cache scenario

	if (fs::exists(addonName)) {
		addon.isLocalAddon = true;
		if(!inCache){

			addonOK = addon.fromFS(addonName, target);
		}else{
			addon = addonsCache[target][addonName];
			addonOK = true;
		}
	}
	
	
	else{
		addon.isLocalAddon = false;
		auto standardPath = ofFilePath::join(ofFilePath::join(getOFRoot(), "addons"), addonName);
		if(!inCache){
			addonOK = addon.fromFS(standardPath, target);
		}else{
			addon = addonsCache[target][addonName];
			addonOK = true;
		}
	}
	if(!addonOK){
		ofLogVerbose() << "Ignoring addon that doesn't seem to exist: " << addonName;
		return; //if addon does not exist, stop early
	}

	if(!inCache){
		addonsCache[target][addonName] = addon; //cache the addon so we dont have to be reading form disk all the time
	}
	addAddon(addon);

	// Process values from ADDON_DATA
	if(addon.data.size()){

		for(auto& d : addon.data){

			fs::path path(ofFilePath::join(addon.addonPath, d));

			if(fs::exists(path)){
				if (fs::is_regular_file(path)){
					ofFile src({path});
					string dest = ofFilePath::join(projectDir, "bin/data/");
					ofStringReplace(d, "data/", ""); // avoid to copy files at /data/data/*
					bool success = src.copyTo(ofFilePath::join(dest, d), false, true);
					if(success){
						ofLogVerbose() << "adding addon data file: " << d;
					}else {
						ofLogWarning() << "Can not add addon data file: " << d;
					}
				}else if(fs::is_directory(path)){
					ofDirectory dir({path});
					string dest = ofFilePath::join(projectDir, "bin/data/");
					ofStringReplace(d, "data/", ""); // avoid to copy files at /data/data/*
					bool success = dir.copyTo(ofFilePath::join(dest, d), false, true);
					if(success){
						ofLogVerbose() << "adding addon data folder: " << d;
					}else{
						ofLogWarning() << "Can not add addon data folder: " << d;
					}
				}
			}else{
				ofLogWarning() << "addon data file does not exist, skipping: " << d;
			}
		}
	}
}

void baseProject::addSrcRecursively(std::string srcPath){
	extSrcPaths.push_back(srcPath);
	vector < string > srcFilesToAdd;

	//so we can just pass through the file paths
	ofDisableDataPath();
	getFilesRecursively(srcPath, srcFilesToAdd);
	ofEnableDataPath();

	//if the files being added are inside the OF root folder, make them relative to the folder.
	bool bMakeRelative = false;
	if( srcPath.find_first_of(getOFRoot()) == 0 ){
		bMakeRelative = true;
	}

	//need this for absolute paths so we can subtract this path from each file path
	//say we add this path: /user/person/documents/shared_of_code
	//we want folders added for shared_of_code/ and any subfolders, but not folders added for /user/ /user/person/ etc
	string parentFolder = ofFilePath::getEnclosingDirectory(ofFilePath::removeTrailingSlash(srcPath));

	std::map <std::string, std::string> uniqueIncludeFolders;
	for( auto & fileToAdd : srcFilesToAdd){
//		cout << "fileToAdd :: " << fileToAdd << endl;
		//if it is an absolute path it is easy - add the file and enclosing folder to the project
		if( ofFilePath::isAbsolute(fileToAdd) && !bMakeRelative ){
			string folder = ofFilePath::getEnclosingDirectory(fileToAdd,false);
			string absFolder = folder;

			auto pos = folder.find_first_of(parentFolder);

			//just to be 100% sure - check if the parent folder path is at the beginning of the file path
			//then remove it so we just get the folder structure of the actual src files being added and not the full path
			if( pos == 0 && parentFolder.size() < folder.size() ){
				folder = folder.substr(parentFolder.size());
			}

			folder = ofFilePath::removeTrailingSlash(folder);

			ofLogVerbose() <<  " adding file " << fileToAdd << " in folder " << folder << " to project ";
			addSrc(fileToAdd, folder);
			uniqueIncludeFolders[absFolder] = absFolder;
		}else{

			auto absPath = fileToAdd;

			//if it is a realtive path make the file relative to the project folder
			if( !ofFilePath::isAbsolute(absPath) ){
				absPath = ofFilePath::getAbsolutePath( ofFilePath::join(ofFilePath::getCurrentExeDir(), fileToAdd) );
			}
			auto canPath = fs::canonical(absPath); //resolves the ./ and ../ to be the most minamlist absolute path

			//get the file path realtive to the project
			auto projectPath = ofFilePath::getAbsolutePath( projectDir );
			auto relPathPathToAdd = ofFilePath::makeRelative(projectPath, canPath);

			//get the folder from the path and clean it up
			string folder = ofFilePath::getEnclosingDirectory(relPathPathToAdd,false);
			string includeFolder = folder;

			ofStringReplace(folder, "../", "");
#ifdef TARGET_WIN32
			ofStringReplace(folder, "..\\", ""); //do both just incase someone has used linux paths on windows
#endif
			folder =  ofFilePath::removeTrailingSlash(folder);

			ofLogVerbose() <<  " adding file " << fileToAdd << " in folder " << folder << " to project ";

			addSrc(relPathPathToAdd, folder);
			uniqueIncludeFolders[includeFolder] = includeFolder;
		}
		}

	//do it this way so we don't try and add a include folder for each file ( as it checks if they are already added ) so should be faster
	for(auto & includeFolder : uniqueIncludeFolders){
		ofLogVerbose() << " adding search include paths for folder " << includeFolder.second;
//		cout << "includeFolder.second " << includeFolder.second << endl;
		addInclude(includeFolder.second);
	}
}

void baseProject::addAddon(ofAddon & addon){

	for(int i=0;i<(int)addons.size();i++){
		if(addons[i].name==addon.name){
			return;
		}
	}

	for(int i=0;i<addon.dependencies.size();i++){
		for(int j=0;j<(int)addons.size();j++){
			if(addon.dependencies[i] != addons[j].name){ //make sure dependencies of addons arent already added to prj
				addAddon(addon.dependencies[i]);
			}else{
				ofLogVerbose() << "trying to add duplicated addon dependency! skipping: " << addon.dependencies[i];
			}
		}
	}


	addons.push_back(addon);

	ofLogVerbose("baseProject") << "libs in addAddon " << addon.libs.size();
	for(auto & lib: addon.libs){
		ofLogVerbose("baseProject") << lib.path;
	}

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
	for (int i = 0; i<(int)addon.defines.size(); i++) {
		ofLogVerbose() << "adding addon defines: " << addon.defines[i];
		addDefine(addon.defines[i]);
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
		addAddon(ofSplitString(addon, "#")[0]);
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

void baseProject::recursiveCopyContents(const ofDirectory & srcDir, ofDirectory & destDir){
	for(auto & f: srcDir){
		if(f.isDirectory()){
			ofDirectory srcSubDir(f.path());
			ofDirectory destSubDir(ofFilePath::join(destDir.path(),f.getFileName()));
			recursiveTemplateCopy(srcSubDir, destSubDir);
		}else{
			f.copyTo(ofFilePath::join(destDir.path(),f.getFileName()),false,true);
		}
	}
}
