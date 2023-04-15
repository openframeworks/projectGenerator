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
#include <set>

using std::string;
using std::vector;
namespace fs = of::filesystem;
// Temporary
using std::cout;
using std::endl;

const fs::path templatesFolder = "scripts/templates";

baseProject::baseProject(std::string _target){
	bLoaded = false;
	target = _target;
}

fs::path baseProject::getPlatformTemplateDir(){
	return getOFRoot() / templatesFolder / target;
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
						templateConfig->platforms.emplace_back(platform);
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

	std::set<fs::path> sorted;
	for (const auto & entry : fs::directory_iterator(getOFRoot() / templatesFolder)) {
		auto f = entry.path();
		if (fs::is_directory(f)) {
			sorted.insert(f);
		}
	}
	
	for (auto & s : sorted) {
		auto templateConfig = parseTemplate(s);
		if(templateConfig){
			templates.emplace_back(*templateConfig);
		}
	}

	return templates;
}

bool baseProject::create(const fs::path & _path, std::string templateName){
	auto path = _path; // just because it is const
	
	templatePath = getPlatformTemplateDir();
	addons.clear();
	extSrcPaths.clear();

	if(!path.is_absolute()){
		path = fs::current_path() / path;
	}
	projectDir = path;
	projectName = path.parent_path().filename();
	bool bDoesDirExist = false;

	fs::path project { projectDir / "src" };
	if (fs::exists(project) && fs::is_directory(project)) {
		bDoesDirExist = true;
	}else{
		// MARK: ofDirectory?
//		bool copyTo(const of::filesystem::path& path, bool bRelativeToData = true, bool overwrite = false);
		ofDirectory(templatePath / "src").copyTo(projectDir / "src");
		ofDirectory(templatePath / "bin").copyTo(projectDir / "bin");
	}

	bool ret = createProjectFile();
	if(!ret) return false;

	//MARK: -
	if(templateName!=""){
		fs::path templateDir = getOFRoot() / templatesFolder / templateName;

		// TODO: PORT
// !!!: asdf
//		templateDir.setShowHidden(true);
		auto templateConfig = parseTemplate(templateDir);
		if(templateConfig){
			recursiveTemplateCopy(templateDir, projectDir);
			for(auto & rename: templateConfig->renames){
				
				auto from = projectDir / rename.first;
				auto to = projectDir / rename.second;
//				auto to = projectDir / templateConfig->renames[rename.first];

//				moveTo(const of::filesystem::path& path, bool bRelativeToData = true, bool overwrite = false);
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
				addSrc(rel.string(), folder.string());
			} else {
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
	if (addonsCache.find(platform) == addonsCache.end()) return false;
	return addonsCache[platform].find(addonPath) != addonsCache[platform].end();
}

void baseProject::addAddon(std::string addonName){
	ofAddon addon;
	addon.pathToOF = getOFRelPath(projectDir.string());
	addon.pathToProject = ofFilePath::getAbsolutePath(projectDir);

	bool addonOK = false;
	bool inCache = isAddonInCache(addonName, target);
	
	fs::path addonPath { addonName };
	if (fs::exists(addonPath)) {
		addon.isLocalAddon = true;
	} else {
		addonPath = fs::path(getOFRoot()) / "addons" / addonName;
		addon.isLocalAddon = false;
	}
	
	if(!inCache){
		addonOK = addon.fromFS(addonPath, target);
	}else{
		addon = addonsCache[target][addonName];
		addonOK = true;
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
		for(auto & data : addon.data){
			std::string d = data;
			ofStringReplace(d, "data/", ""); // avoid to copy files at /data/data/*

			fs::path path { addon.addonPath / data };
			fs::path dest { projectDir / "bin" / "data" };
			
			if (addon.isLocalAddon) {
				path = addon.pathToProject / path;
			}
			
			if(fs::exists(path)){
				if (fs::is_regular_file(path)){
					// TODO: FS
					ofFile src(path);
					//	bool copyTo(const of::filesystem::path& path, bool bRelativeToData = true, bool overwrite = false) const;
					bool success = src.copyTo(dest / d, false, true);
					if(success){
						ofLogVerbose() << "adding addon data file: " << d;
					}else {
						ofLogWarning() << "Can not add addon data file: " << d;
					}
				}else if(fs::is_directory(path)){
					// TODO: FS
					ofDirectory dir(path);
//					bool copyTo(const of::filesystem::path& path, bool bRelativeToData = true, bool overwrite = false);
					bool success = dir.copyTo(dest / d, false, true);
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

// FIXME: FS parameter
void baseProject::addSrcRecursively(std::string srcPath){
	extSrcPaths.emplace_back(srcPath);
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

	// FIXME: - I've inspected this map and it is kinda silly because the key is always equal to the value (first = second)
	std::unordered_map <std::string, std::string> uniqueIncludeFolders;
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

	// FIXME: Test this, I suppose this is only invoked when an addon is added
	// from a dependency of another addon, and it has its own dependencies too.
	
	cout << "---> dependencies" << endl;
	for (auto & d : addon.dependencies) {
		bool found = false;
		for (auto & a : addons) {
			if (a.name == d) {
				found = true;
				break;
			}
		}
		if (!found) {
			alert(">>>> addaddon :: " + d, 35);
			addAddon(d);
		} else {
			ofLogVerbose() << "trying to add duplicated addon dependency! skipping: " << d;
		}
	}
	cout << "---> " << endl;

	addons.emplace_back(addon);

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
		addSrc(addon.srcFiles[i], addon.filesToFolders[addon.srcFiles[i]]);
	}
	for(int i=0;i<(int)addon.csrcFiles.size(); i++){
		ofLogVerbose() << "adding addon c srcFiles: " << addon.csrcFiles[i];
		addSrc(addon.csrcFiles[i], addon.filesToFolders[addon.csrcFiles[i]],C);
	}
	for(int i=0;i<(int)addon.cppsrcFiles.size(); i++){
		ofLogVerbose() << "adding addon cpp srcFiles: " << addon.cppsrcFiles[i];
		addSrc(addon.cppsrcFiles[i], addon.filesToFolders[addon.cppsrcFiles[i]],CPP);
	}
	for(int i=0;i<(int)addon.objcsrcFiles.size(); i++){
		ofLogVerbose() << "adding addon objc srcFiles: " << addon.objcsrcFiles[i];
		addSrc(addon.objcsrcFiles[i], addon.filesToFolders[addon.objcsrcFiles[i]],OBJC);
	}
	for(int i=0;i<(int)addon.headersrcFiles.size(); i++){
		ofLogVerbose() << "adding addon header srcFiles: " << addon.headersrcFiles[i];
		addSrc(addon.headersrcFiles[i], addon.filesToFolders[addon.headersrcFiles[i]],HEADER);
	}
	for (int i = 0; i<(int)addon.defines.size(); i++) {
		ofLogVerbose() << "adding addon defines: " << addon.defines[i];
		addDefine(addon.defines[i]);
	}
}

void baseProject::parseAddons(){
	alert("--- parseAddons");
	
	ofFile addonsMake(ofFilePath::join(projectDir,"addons.make"));
	ofBuffer addonsMakeMem;
	addonsMake >> addonsMakeMem;
	for(auto line: addonsMakeMem.getLines()){
		auto addon = ofTrim(line);
		if(addon[0] == '#') continue;
		if(addon == "") continue;
		addAddon(ofSplitString(addon, "#")[0]);
	}
	alert("--- end parseAddons");

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

void baseProject::recursiveTemplateCopy(const fs::path & srcDir, const fs::path & destDir){
	for (const auto & entry : fs::directory_iterator(srcDir)) {
		auto f = entry.path();
		auto destFile = destDir / f.filename();
		if (fs::is_directory(f)) {
			recursiveTemplateCopy(f, destFile);
		}
		else if (f.filename() != "template.config") {
			if (!fs::exists(destFile)) {
				fs::copy_file(f, destFile); // from, to
			}
		}
	}
}

void baseProject::recursiveCopyContents(const fs::path & srcDir, const fs::path & destDir){
	for (const auto & entry : fs::directory_iterator(srcDir)) {
		auto f = entry.path();
		auto destFile = destDir / f.filename();
		if (fs::is_directory(f)) {
			recursiveTemplateCopy(f, destFile);
		} else {
			if (!fs::exists(destFile)) {
				fs::copy_file(f, destFile);
			}
		}
	}
			
//void baseProject::recursiveCopyContents(const ofDirectory & srcDir, ofDirectory & destDir){
//	for(auto & f: srcDir){
//		if(f.isDirectory()){
//			ofDirectory srcSubDir(f.path());
//			ofDirectory destSubDir(ofFilePath::join(destDir.path(),f.getFileName()));
//			recursiveTemplateCopy(srcSubDir, destSubDir);
//		}else{
//			f.copyTo(ofFilePath::join(destDir.path(),f.getFileName()),false,true);
//		}
//	}
}
