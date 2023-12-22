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
using std::string;
using std::vector;

const fs::path templatesFolder = "scripts/templates";

baseProject::baseProject(const string & _target) : target(_target) {
	bLoaded = false;
}

fs::path baseProject::getPlatformTemplateDir() {
	string folder { target };
	if ( target == "msys2"
		|| target == "linux"
		|| target == "linux64"
		|| target == "linuxarmv6l"
		|| target == "linuxarmv7l"
		|| target == "linuxaarch64"
	) {
		folder = "vscode";
	}
	
//	if ( target == "qtcreator" ) {
//		return getOFRoot()
//	}
	
	return getOFRoot() / templatesFolder / folder;
}


bool baseProject::isPlatformName(const string & platform) {
	return std::find(platformsOptions.begin(), platformsOptions.end(), platform) != platformsOptions.end();
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
//	alert("baseProject::create " + path.string() + " : " + templateName, 35);
//	auto path = _path; // just because it is const


	//if the files being added are inside the OF root folder, make them relative to the folder.

//	alert("getOFRoot() " + getOFRoot().string());
//	alert("getOFRoot() " + fs::weakly_canonical(fs::absolute(getOFRoot())).string());
//	alert("path " + path.string());
//	alert("path " + fs::weakly_canonical(fs::absolute(path)).string());

	// FIXME: Rewrite here
	if (ofIsPathInPath(fs::absolute(path), getOFRoot())) {
//		alert ("bMakeRelative true", 35);
		bMakeRelative = true;
	} else {
//		alert ("bMakeRelative false", 35);
	}

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
	} else {
		for (auto & p : { string("src") , string("bin") }) {
			fs::copy (templatePath / p, projectDir / p, fs::copy_options::recursive);
		}
	}

	bool ret = createProjectFile();
	if(!ret) return false;

//	cout << "after return : " << templateName << endl;
	if(!empty(templateName)){
//		cout << "templateName not empty " << templateName << endl;
//		return getOFRoot() / templatesFolder / target;

		fs::path templateDir = getOFRoot() / templatesFolder / templateName;
//		alert("templateDir " + templateDir.string());

		auto templateConfig = parseTemplate(templateDir);
		if(templateConfig){
			recursiveTemplateCopy(templateDir, projectDir);
			for(auto & rename: templateConfig->renames){
				auto from = projectDir / rename.first;
				auto to = projectDir / rename.second;
				if (fs::exists(to)) {
					fs::remove(to);
				}
				try {
					fs::rename(from, to);
				} catch(fs::filesystem_error& e) {
					ofLog() << "Can not rename: " << from << " :: " << to << " :: " << e.what() ;
				}
			}
		} else {
			ofLogWarning() << "Cannot find " << templateName << " using platform template only";
		}
	}

	ret = loadProjectFile();

	if(!ret) return false;

	parseConfigMake();

	if (bDoesDirExist){
		vector < string > fileNames;
		getFilesRecursively(projectDir / "src", fileNames);

		std::sort (fileNames.begin(), fileNames.end());

//		for (auto & f : fileNames) {
//			alert (f);
//		}

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
//		vector < fs::path > paths;
//		for (auto & f : fileNames) {
//			auto dir = fs::path(f).parent_path().filename();
//			if (std::find(paths.begin(), paths.end(), dir) == paths.end()) {
//				paths.emplace_back(dir);
//				addInclude(dir.string());
//			}
//		}

		// FIXME: Port to std::list, so no comparison is needed.
		// only add unique paths
		vector < fs::path > paths;
		for (auto & f : fileNames) {
			fs::path rel { fs::relative(fs::path(f).parent_path(), projectDir) };
			if (std::find(paths.begin(), paths.end(), rel) == paths.end()) {
				paths.emplace_back(rel);
//				alert ("rel = " + rel.string());
				addInclude(rel.string());
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

	vector <string> lines = fileToStrings(projectDir / "config.make");
	std::ofstream saveConfig(projectDir / "config.make");

	for (auto & str : lines) {
		//add the of root path
		if( str.rfind("# OF_ROOT =", 0) == 0 || str.rfind("OF_ROOT =", 0) == 0){
			fs::path path = getOFRoot();

			// FIXME: change to ofIsPathInPath
			if( projectDir.string().rfind(getOFRoot().string(), 0) == 0) {
				path = getOFRelPath(projectDir);
			}
			saveConfig << "OF_ROOT = " << path.generic_string() << std::endl;
		}
		// replace this section with our external paths
		else if( extSrcPaths.size() && str.rfind("# PROJECT_EXTERNAL_SOURCE_PATHS =", 0) == 0 ){
			for(int d = 0; d < extSrcPaths.size(); d++){
				ofLog(OF_LOG_VERBOSE) << " adding PROJECT_EXTERNAL_SOURCE_PATHS to config" << extSrcPaths[d].generic_string() << std::endl;
				saveConfig << "PROJECT_EXTERNAL_SOURCE_PATHS" << (d == 0 ? " = " : " += ") << extSrcPaths[d].generic_string() << std::endl;
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
//	alert( "baseProject::addAddon " + addonName );

	// FIXME : not target, yes platform.
	#ifdef TARGET_WIN32
//	std::replace( addonName.begin(), addonName.end(), '/', '\\' );
	fixSlashOrder(addonName);
	#endif

	ofAddon addon;
	// MARK: Review this path here. EDIT: I think it is finally good

	if (bMakeRelative) {
		addon.pathToOF = getOFRelPath(projectDir);
	} else {
		addon.pathToOF = getOFRoot();
	}

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


	if (!inCache) {
		addonOK = addon.fromFS(addonPath, target);
	} else {
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

	for (auto & a : addons) {
		if (a.name == addon.name) return;
	}


	for (auto & d : addon.dependencies) {
		bool found = false;
		for (auto & a : addons) {
			if (a.name == d) {
				found = true;
				break;
			}
		}
		if (!found) {
			baseProject::addAddon(d);
		} else {
			ofLogVerbose() << "trying to add duplicated addon dependency! skipping: " << d;
		}
	}


	ofLogNotice() << "adding addon: " << addon.name;
	addons.emplace_back(addon);

	for (auto & e : addon.includePaths) {
		ofLogVerbose() << "adding addon include path: " << e;
//		ofLog() << "adding addon include path: " << e;
		addInclude(e);
	}

	// It was part exclusive of visualStudioProject. now it is part of baseProject, so dlls are copied in VSCode project and .so files in linux
	for (auto & d : addon.dllsToCopy) {
		ofLogVerbose() << "adding addon dlls to bin: " << d;
		fs::path from { d };
		fs::path to { projectDir / "bin" / from.filename() };
		if (from.extension() == ".so") {
			fs::path folder { projectDir / "bin" / "libs" };
			if (!fs::exists(folder)) {
				try {
					fs::create_directory(folder);
				} catch(fs::filesystem_error& e) {
					ofLogError("baseProject::addAddon") << "error creating folder " << folder << e.what();
				}
			}
//			to = projectDir / "bin" / "libs" / from.filename();
			to = folder / from.filename();
		}

		try {
			fs::copy_file(from, to, fs::copy_options::overwrite_existing);
		} catch(fs::filesystem_error& e) {
			ofLogError("baseProject::addAddon") << "error copying template file " << from << endl << "to: " << to << endl <<  e.what();
		}
	}

	// MARK: - SPECIFIC for each project.
	// XCode and VS override the base addAddon. other templates will use baseproject::addAddon(ofAddon...
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
					try {
						fs::copy_file(from, to, fs::copy_options::overwrite_existing);
						ofLogVerbose() << "adding addon data file: " << d << endl;
					} catch(fs::filesystem_error& e) {
						ofLogWarning() << "Can not add addon data file: " << to << " :: " << e.what() << std::endl;;
					}

				} else if (fs::is_directory(from)) {
					if (!fs::exists(to)) {
						fs::create_directory(to);
					}

					try {
						fs::copy(from, to, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
						ofLogVerbose() << "adding addon data file: " << d << endl;
					} catch(fs::filesystem_error& e) {
						ofLogWarning() << "Can not add addon data file: " << to << " :: " << e.what() << std::endl;
					}
				}
			} else {
				ofLogWarning() << "addon data file does not exist, skipping: " << d;
			}
		}
	}
}


// MARK: - This function is only called by addon dependencies, when one addon is asking for another one to be included.
// this is only invoked by XCode and visualStudioProject, and I don't understand why as they are similar
void baseProject::addAddon(ofAddon & addon){
	alert("baseProject::addAddon ofAddon & addon :: " + addon.name);

	// FIXME: Duplicate Code
	for (auto & a : addons) {
		if (a.name == addon.name) {
			return;
		}
	}

	/*

	 MARK: Test this, I suppose this is only invoked when an addon is added
	 from a dependency of another addon, and it has its own dependencies too.

	 this is not possible to test easily using xcode or visualstudio project
	 because baseProject::addAddon is only invoked when there is already dependencies in xcode addons.
	 the only way to test this is other platforms like qbs?
	 unless there is one addon added which needs another, and it needs another.

	 */
	alert("---> dependencies");
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
	alert("---> dependencies");
	addons.emplace_back(addon);

	ofLogVerbose("baseProject") << "libs in addAddon " << addon.libs.size();
	for(auto & lib: addon.libs){
		ofLogVerbose("baseProject") << lib.path;
	}

	for (auto & a : addon.includePaths) {
		ofLogVerbose() << "adding addon include path: " << a;
		addInclude(a);
	}

	for (auto & a : addon.libs) {
		ofLogVerbose() << "adding addon libs: " << a.path;
		// FIXME: remove
		alert ("addlibrary " + a.path, 33);
		addLibrary(a);
	}

	for (auto & a : addon.cflags) {
		ofLogVerbose() << "adding addon cflags: " << a;
		addCFLAG(a);
	}

	for (auto & a : addon.cppflags) {
		ofLogVerbose() << "adding addon cppflags: " << a;
		addCPPFLAG(a);
	}

	for (auto & a : addon.ldflags) {
		ofLogVerbose() << "adding addon ldflags: " << a;
		addLDFLAG(a);
	}

	for (auto & a : addon.srcFiles) {
		ofLogVerbose() << "adding addon srcFiles: " << a;
		addSrc(a, addon.filesToFolders[a]);
	}

	for (auto & a : addon.csrcFiles) {
		ofLogVerbose() << "adding addon c srcFiles: " << a;
		addSrc(a, addon.filesToFolders[a], C);
	}

	for (auto & a : addon.cppsrcFiles) {
		ofLogVerbose() << "adding addon cpp srcFiles: " << a;
		addSrc(a, addon.filesToFolders[a],CPP);
	}

	for (auto & a : addon.objcsrcFiles) {
		ofLogVerbose() << "adding addon objc srcFiles: " << a;
		addSrc(a, addon.filesToFolders[a],OBJC);
	}

	for (auto & a : addon.headersrcFiles) {
		ofLogVerbose() << "adding addon header srcFiles: " << a;
		addSrc(a, addon.filesToFolders[a],HEADER);
	}

	for (auto & a : addon.defines) {
		ofLogVerbose() << "adding addon defines: " << a;
		addDefine(a);
	}
}


void baseProject::addSrcRecursively(const fs::path & srcPath){
	ofLog() << "using additional source folder " << srcPath.string();
//	alert("--");
//	alert("addSrcRecursively " + srcPath.string());
	fs::path base = srcPath.parent_path();
//	alert("base = " + base.string());

	extSrcPaths.emplace_back(srcPath);
	vector < fs::path > srcFilesToAdd;
	getFilesRecursively(srcPath, srcFilesToAdd);
//	bool isRelative = ofIsPathInPath(fs::absolute(srcPath), getOFRoot());

	std::unordered_set<string> uniqueIncludeFolders;

	for( auto & src : srcFilesToAdd){
		fs::path parent = src.parent_path();
		fs::path folder = parent.lexically_relative(base);

		//		alert ("addSrc file:" + src.string() + " -- folder:" + folder.string(), 35);
		addSrc(src.string(), folder.string());
		if (parent.string() != "") {
			uniqueIncludeFolders.insert(parent.string());
		}
	}

	for(auto & i : uniqueIncludeFolders){
		ofLogVerbose() << " adding search include paths for folder " << i;
		addInclude(i);
	}
}


void baseProject::parseAddons(){
	fs::path parseFile { projectDir / "addons.make" };

	for (auto & line : fileToStrings(parseFile)) {
		auto addon = ofTrim(line);
//		alert("line " + addon);
		if(addon[0] == '#') continue;
		if(addon == "") continue;
		auto s = ofSplitString(addon, "#")[0];
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
	if (recursiveCopy(srcDir, destDir)) {
		fs::path templateFile = destDir / "template.config";
		if (fs::exists(templateFile)) {
			fs::remove(templateFile);
		}
	}
}

void baseProject::recursiveCopyContents(const fs::path & srcDir, const fs::path & destDir){
	recursiveCopy(srcDir, destDir);
}

bool baseProject::recursiveCopy(const fs::path & srcDir, const fs::path & destDir){
	//	alert("recursiveCopy " + srcDir.string() + " : " + destDir.string(), 32);
	try {
		fs::copy(srcDir, destDir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
		return true;
	} catch (std::exception& e) {
		// TODO: ofLogWarning()
		std::cout << e.what();
		return false;
	}
}
