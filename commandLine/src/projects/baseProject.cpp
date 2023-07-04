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
#include <unordered_set>

const fs::path templatesFolder = "scripts/templates";

baseProject::baseProject(string _target){
	bLoaded = false;
	target = _target;
}

fs::path baseProject::getPlatformTemplateDir(){
	return getOFRoot() / templatesFolder / target;
}

bool isPlatformName(string file){
	for(int platform=OF_TARGET_OSX;platform<OF_TARGET_EMSCRIPTEN+1;platform++){
		if(file==getTargetString((ofTargetPlatform)platform)){
			return true;
		}
	}
	return false;
}

std::unique_ptr<baseProject::Template> baseProject::parseTemplate(const fs::path & templateDir){
	string name = templateDir.parent_path().filename().string();
	if (fs::is_directory(templateDir) && !isPlatformName(name)) {
		fs::path templateConfigFilePath = templateDir  / "template.config";

		if (fs::exists(templateConfigFilePath)) {
			auto supported = false;
			auto templateConfig = std::make_unique<Template>();
			templateConfig->dir = templateDir;
			templateConfig->name = name;
			
			for (auto & line : fileToStrings(templateConfigFilePath)) {
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

vector<baseProject::Template> baseProject::listAvailableTemplates(string target){
	vector<baseProject::Template> templates;
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

bool baseProject::create(const fs::path & path, string templateName){
//	alert("baseProject::create " + path.string() + " : " + templateName);
//	auto path = _path; // just because it is const
	addons.clear();
	extSrcPaths.clear();

	templatePath = getPlatformTemplateDir();
	projectDir = path;
	auto projectPath = fs::canonical(fs::current_path() / path);
	projectName = projectPath.filename().string();

//	cout << "templatePath " << templatePath << endl;
//	cout << "projectDir " << projectDir << endl;
//	cout << "projectPath " << projectPath << endl;
//	cout << "projectName = " << projectName << endl;
//	
	bool bDoesDirExist = false;

	fs::path project { projectDir / "src" };
	if (fs::exists(project) && fs::is_directory(project)) {
		bDoesDirExist = true;
	}else{
		for (auto & p : { "src" , "bin" }) {
			fs::path from = templatePath / p;
			fs::path to = projectDir / p;
			fs::copy (templatePath / p, projectDir / p, fs::copy_options::recursive);
		}
	}

	bool ret = createProjectFile();
	if(!ret) return false;
	
//	cout << "after return : " << templateName << endl;

	if(!empty(templateName)){
//		cout << "templateName not empty " << templateName << endl;
		fs::path templateDir = getOFRoot() / templatesFolder / templateName;
//		alert("templateDir " + templateDir.string());

		auto templateConfig = parseTemplate(templateDir);
		if(templateConfig){
			recursiveTemplateCopy(templateDir, projectDir);
			for(auto & rename: templateConfig->renames){
				
				auto from = projectDir / rename.first;
				auto to = projectDir / rename.second;
//				cout << "rename from to " << from << " : " << to << endl;
				
				if (fs::exists(to)) {
					fs::remove(to);
				}
				try {
					fs::rename(from, to);
				} catch(fs::filesystem_error& e) {
					ofLog() << "Can not rename: " << from << " :: " << to << " :: " << e.what() ;
				}
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
		getFilesRecursively(projectDir / "src", fileNames);

		for (auto & f : fileNames) {
			fs::path rel { fs::relative(f, projectDir) };
			fs::path folder { rel.parent_path() };

			string fileName = rel.string();

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
		vector < fs::path > paths;
		for (auto & f : fileNames) {
			auto dir = fs::path(f).parent_path().filename();
			if (std::find(paths.begin(), paths.end(), dir) == paths.end()) {
				paths.emplace_back(dir);
				addInclude(dir.string());
			}
		}

	}
	return true;
}

bool baseProject::save(){
	ofLog(OF_LOG_NOTICE) << "saving addons.make";
	
	std::ofstream addonsMake(projectDir / "addons.make");
	for (auto & a : addons) {
		if (a.isLocalAddon) {
			addonsMake << fs::path(a.addonPath).generic_string() << std::endl;
		} else {
			addonsMake << a.name << std::endl;
		}
	}

	//save out params which the PG knows about to config.make
	//we mostly use this right now for storing the external source paths
	// FIXME: Absolute or FS
	
	vector <string> lines = fileToStrings(projectDir / "config.make");
	std::ofstream saveConfig(projectDir / "config.make");

	for (auto & str : lines) {
		//add the of root path
		if( str.rfind("# OF_ROOT =", 0) == 0 || str.rfind("OF_ROOT =", 0) == 0){
			fs::path path = getOFRoot();
			if( projectDir.string().rfind(getOFRoot().string(), 0) == 0) {
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
		} else {
		   saveConfig << str << std::endl;
		}
	}
	return saveProjectFile();
}

bool baseProject::isAddonInCache(const string & addonPath, const string platform){
	if (addonsCache.find(platform) == addonsCache.end()) return false;
	return addonsCache[platform].find(addonPath) != addonsCache[platform].end();
}

void baseProject::addAddon(string addonName){
//	std::cout << "baseProject::addAddon " << addonName << std::endl;
	
	// FIXME : not target, yes platform.
	#ifdef TARGET_WIN32
		std::replace( addonName.begin(), addonName.end(), '/', '\\' );
	#endif
	

	ofAddon addon;
	// FIXME: Review this path here.
	addon.pathToOF = getOFRelPath(projectDir);
	addon.pathToProject = projectDir;

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
		//cache the addon so we dont have to be reading form disk all the time
		addonsCache[target][addonName] = addon;
	}
	
	addAddon(addon);

	// Process values from ADDON_DATA
	if(addon.data.size()){
		for(auto & data : addon.data){
			string d = data;
			ofStringReplace(d, "data/", ""); // avoid to copy files at /data/data/*

			fs::path from { addon.addonPath / data };
			fs::path dest { projectDir / "bin" / "data" };
			
			if(fs::exists(from)){
				fs::path to { dest / d };
				if (fs::is_regular_file(from)){
//					cout << "from is regular file" << endl;
					try {
						fs::copy_file(from, to, fs::copy_options::overwrite_existing);
						ofLogVerbose() << "adding addon data file: " << d << endl;
						ofLog() << "adding addon data file: " << d << endl;
					} catch(fs::filesystem_error& e) {
						ofLogWarning() << "Can not add addon data file: " << to << " :: " << e.what() << std::endl;;
						ofLog() << "Can not add addon data file: " << to << " :: " << e.what() << std::endl;;
					}

				} else if (fs::is_directory(from)) {
//					cout << "from is directory" << endl;
					if (!fs::exists(to)) {
						fs::create_directory(to);
					}
					
					try {
						fs::copy(from, to,
							fs::copy_options::overwrite_existing |
							fs::copy_options::recursive
						);
						ofLogVerbose() << "adding addon data file: " << d << endl;
						ofLog() << "adding addon data file: " << d << endl;
					} catch(fs::filesystem_error& e) {
						ofLogWarning() << "Can not add addon data file: " << to << " :: " << e.what() << std::endl;
						ofLog() << "Can not add addon data file: " << to << " :: " << e.what() << std::endl;
					}
				}
			} else {
				ofLogWarning() << "addon data file does not exist, skipping: " << d;
			}
		}
	}
}

void baseProject::addSrcRecursively(const fs::path & srcPath){
	alert("addSrcRecursively " + srcPath.string());
	fs::path base = srcPath.parent_path();
	alert("base = " + base.string());
	
	extSrcPaths.emplace_back(srcPath.string());
	vector < fs::path > srcFilesToAdd;

	getFilesRecursively(srcPath, srcFilesToAdd);

	//if the files being added are inside the OF root folder, make them relative to the folder.
	bool bMakeRelative = false;
	if (ofIsPathInPath(srcPath, getOFRoot())) {
		bMakeRelative = true;
	}

	// cout << "makeRelative " << bMakeRelative << endl;
	//need this for absolute paths so we can subtract this path from each file path
	//say we add this path: /user/person/documents/shared_of_code
	//we want folders added for shared_of_code/ and any subfolders, but not folders added for /user/ /user/person/ etc
//	string parentFolder = ofFilePath::getEnclosingDirectory(ofFilePath::removeTrailingSlash(srcPath));

	std::unordered_set<string> uniqueIncludeFolders;
	for( auto & src : srcFilesToAdd){
//		cout << "fileToAdd :: " << src << endl;
		//if it is an absolute path it is easy - add the file and enclosing folder to the project
		string includeFolder { "" };
		
		fs::path parent = src.parent_path();
		fs::path folder2 = parent.lexically_relative(base);
		
		if (src.is_absolute() && !bMakeRelative) {
			// TODO: rewrite
			/*
			string folder = ofFilePath::getEnclosingDirectory(src, false);
			string absFolder = folder;

			auto pos = folder.find_first_of(parentFolder);

			//just to be 100% sure - check if the parent folder path is at the beginning of the file path
			//then remove it so we just get the folder structure of the actual src files being added and not the full path
			if( pos == 0 && parentFolder.size() < folder.size() ){
				folder = folder.substr(parentFolder.size());
			}

			folder = ofFilePath::removeTrailingSlash(folder);

			ofLogVerbose() <<  " adding file " << src << " in folder " << folder << " to project ";
//			addSrc(src, folder);
			*/

			
			ofLog() <<  " adding file FIRST " << src << " in folder " << folder2 << " to project ";
			addSrc(src.string(), folder2.string());
			includeFolder = parent.string();
		} else {
			/*
			auto absPath = src;
			// cout << "SECOND " << endl;
			//if it is a realtive path make the file relative to the project folder
			if (!src.is_absolute()) {
//			if( !ofFilePath::isAbsolute(src) ){
				absPath = ofFilePath::getAbsolutePath( ofFilePath::join(ofFilePath::getCurrentExeDir(), src) );
				absPath = ofFilePath::getAbsolutePath( projectDir / src );
				fs::path f1 = projectDir / src;
				cout << ">>>> f1" << endl;
				cout << f1 << endl;
				cout << fs::absolute(f1) << endl;
				cout << f1.lexically_normal() << endl;
			}
//			cout << ">> will canonical " << absPath << endl;
			auto canPath = fs::canonical(absPath); //resolves the ./ and ../ to be the most minamlist absolute path
//			cout << ">> end canonical" << endl;
			//get the file path realtive to the project
			auto projectPath = ofFilePath::getAbsolutePath( projectDir );
			auto relPathPathToAdd = ofFilePath::makeRelative(projectPath, canPath);

			//get the folder from the path and clean it up
			string folder = ofFilePath::getEnclosingDirectory(relPathPathToAdd,false);
			includeFolder = folder;

			ofStringReplace(folder, "../", "");
#ifdef TARGET_WIN32
			ofStringReplace(folder, "..\\", ""); //do both just incase someone has used linux paths on windows
#endif
			folder =  ofFilePath::removeTrailingSlash(folder);

			*/
			


			// FIXME: revert back to ofLogVerbose
			ofLog() <<  " adding file SECOND " << src << " in folder " << folder2 << " to project ";
			addSrc(src.string(), folder2.string());
			includeFolder = parent.string();
		}
		
		if (includeFolder != "") {
			ofLog() <<  " uniqueIncludeFolders " << includeFolder ;
			uniqueIncludeFolders.insert(includeFolder);
		}
	}

	for(auto & i : uniqueIncludeFolders){
		ofLogVerbose() << " adding search include paths for folder " << i;
		addInclude(i);
	}
}

void baseProject::addAddon(ofAddon & addon){
	alert("baseProject::addAddon " + addon.name);
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
	fs::path parseFile { projectDir / "addons.make" };

	for (auto & line : fileToStrings(parseFile)) {
		auto addon = ofTrim(line);
		if(addon[0] == '#') continue;
		if(addon == "") continue;
		addAddon(ofSplitString(addon, "#")[0]);
	}
}

void baseProject::parseConfigMake(){
	fs::path parseFile { projectDir / "config.make" };

	for (auto & line : fileToStrings(parseFile)) {
		auto config = ofTrim(line);
		if(config[0] == '#') continue;
		if(config == "") continue;
		if(config.find("=")!=string::npos){
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
//				fs::copy(f, destFile);
				// FIXME: FS
				ofFile::copyFromTo(f, destFile, false, true); // from, to
			}
		}
	}
}

void baseProject::recursiveCopyContents(const fs::path & srcDir, const fs::path & destDir){
	for (const auto & entry : fs::directory_iterator(srcDir)) {
		auto f = entry.path();
		auto destFile = destDir / f.filename();
		if (fs::is_directory(f)) {
			recursiveCopyContents(f, destFile);
		} else {
			if (!fs::exists(destFile)) {
				try {
					fs::copy_file(f, destFile, fs::copy_options::overwrite_existing);
				} catch(fs::filesystem_error& e) {
					std::cout << "Could not copy " << f << " > " << destFile << " :: "  << e.what() << std::endl;
				}
			}
		}
	}
}
