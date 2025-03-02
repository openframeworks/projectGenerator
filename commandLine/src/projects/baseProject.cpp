//
//  baseProject.cpp
//  projectGenerator
//
//  Created by molmol on 3/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "baseProject.h"
#include "ofLog.h"
#include "Utils.h"
#include "ofConstants.h"
#include "ofUtils.h"
#include <list>
#include <set>
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

bool baseProject::create(const fs::path & _path, string templateName){
//	alert("baseProject::create " + path.string() + " : " + templateName, 35);
	auto path = _path; // just because it is const
    fs::current_path(path);

	//if the files being added are inside the OF root folder, make them relative to the folder.

//	alert("getOFRoot() " + getOFRoot().string());
//	alert("getOFRoot() " + fs::weakly_canonical(fs::absolute(getOFRoot())).string());
//	alert("path " + path.string());
//	alert("path " + fs::weakly_canonical(fs::absolute(path)).string());
	
//	cout << endl;
//	ofLogNotice() << "create project " << path;

	// FIXME: Rewrite here
//	if (ofIsPathInPath(fs::absolute(path), getOFRoot())) {
////		alert ("bMakeRelative true", 35);
//		bMakeRelative = true;
//	} else {
////		alert ("bMakeRelative false", 35);
//	}

	addons.clear();
	extSrcPaths.clear();

	templatePath = normalizePath(getPlatformTemplateDir());
    ofLogNotice() << "templatePath: [" << templatePath << "]";
	auto projectPath = fs::canonical(fs::current_path() / path);
	
	projectDir = path;
	projectPath = normalizePath(projectPath);
	ofLogNotice() << "projectPath: [" << projectPath << "]";
	
	projectName = projectPath.filename().string();
	
	// we had this in some projects. if we decide to keep this is the place
	//	if (!fs::exists(projectDir)) {
	//		fs::create_directory(projectDir);
	//	}
	bool bDoesSrcDirExist = false;

	// it can be only "src"
	fs::path projectSrc { projectDir / "src" };
	
	if (fs::exists(projectSrc) && fs::is_directory(projectSrc)) {
		bDoesSrcDirExist = true;
	} else {
		for (auto & p : { fs::path("src"), fs::path("bin") }) {
			try {
				// think of exclusions to manually merge like plist
				fs::copy (templatePath / p, projectDir / p, fs::copy_options::recursive | (bOverwrite ? fs::copy_options::overwrite_existing : fs::copy_options::update_existing));
			} catch(fs::filesystem_error & e) {
				ofLogNotice() << "Can not copy: " << templatePath / p << " :: " << projectDir / p;
				ofLogNotice() << e.what();

			}
			
		}
	}
	bool ret = createProjectFile();
	if(!ret) return false;

//	cout << "after return : " << templateName << endl;
	if(!empty(templateName)){
//		cout << "templateName not empty " << templateName << endl;
//		return getOFRoot() / templatesFolder / target;

		fs::path templateDir = getOFRoot() / templatesFolder / templateName;
		ofLogNotice() << "templateDir: [" << templateDir << "]";
		templateDir = normalizePath(templateDir);
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
					ofLogWarning() << "Can not rename: " << from << " :: " << to;
					ofLogWarning() << e.what();
				}
			}
		} else {
			ofLogWarning() << "Cannot find " << templateName << " using platform template only";
		}
	}

	ret = loadProjectFile();

	if(!ret) return false;

	parseConfigMake();

	if (bDoesSrcDirExist){
		vector <fs::path> fileNames;

		// CWD is already on projectDir. so with this we get relative paths
		getFilesRecursively("src", fileNames);
		
		std::sort(fileNames.begin(), fileNames.end(), [](const fs::path & a, const fs::path & b) {
			return a.string() < b.string();
		});
		
		// FIXME: I think we should remove this logic and remove the files from default project.
		// only the files present are added to the project
		
		for (const auto & f : fileNames) {
			if (f != "src/ofApp.cpp" &&
				f != "src/ofApp.h" &&
				f != "src/main.cpp" &&
				f != "src/ofApp.mm" &&
				f != "src/main.mm") {
				addSrc(f, f.parent_path());
			} else {
			}
		}

		std::set<fs::path> uniquePaths;
		for (auto & f : fileNames) {
			uniquePaths.insert(f.parent_path());
		}
		
		for (auto & p : uniquePaths) {
			if (containsSourceFiles(p)) {
				ofLogVerbose() << "[prjFiles-addIncludeDir] contains src - Adding dir: " << p;
				addInclude(p);
			} else {
				ofLogVerbose() << "[prjFiles-addIncludeDir] no src - not adding: " << p;
			}
		}
	}
	return true;
}

bool baseProject::save(){
	ofLog(OF_LOG_NOTICE) << "saving addons.make";

	std::ofstream addonsMake(projectDir / "addons.make");
	for (auto & a : addons) {
        addonsMake << a.addonMakeName << std::endl;
//		if (a.isLocalAddon) {
//			addonsMake << fs::path(a.addonPath).generic_string() << std::endl;
//		} else {
//			addonsMake << a.name << std::endl;
//		}
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
				path = fs::relative(getOFRoot(), projectDir);
			}
			saveConfig << "OF_ROOT = " << path.generic_string() << std::endl;
		}
		// replace this section with our external paths
		else if( extSrcPaths.size() && str.rfind("# PROJECT_EXTERNAL_SOURCE_PATHS =", 0) == 0 ){
			for(std::size_t d = 0; d < extSrcPaths.size(); d++){
				ofLog(OF_LOG_VERBOSE) << " adding PROJECT_EXTERNAL_SOURCE_PATHS to config" << extSrcPaths[d].generic_string() << std::endl;
				saveConfig << "PROJECT_EXTERNAL_SOURCE_PATHS" << (d == 0 ? " = " : " += ") << extSrcPaths[d].generic_string() << std::endl;
			}
		} else {
		   saveConfig << str << std::endl;
		}
	}
#ifdef OFADDON_OUTPUT_JSON_DEBUG
    saveAddonsToJson();
#endif
	return saveProjectFile();
}

bool baseProject::isAddonInCache(const string & addonPath, const string platform){
	if (addonsCache.find(platform) == addonsCache.end()) return false;
	return addonsCache[platform].find(addonPath) != addonsCache[platform].end();
}

void baseProject::addAddon(const std::string & _addonName){
    ofLogVerbose("baseProject::addAddon") << _addonName;
//	alert( "baseProject::addAddon " + _addonName );
    
//    auto addonName = ofAddon::cleanName(_addonName);
	auto addonName = _addonName;


    // FIXME : not target, yes platform.
//#ifdef TARGET_WIN32
//    //	std::replace( addonName.begin(), addonName.end(), '/', '\\' );
//    fixSlashOrder(addonName);
//#endif
//    
    
//    addon.addonMakeName = addonName;
//    
//    {
//        auto s = ofSplitString(addonName, "#");
//        if(s.size()){
//            addonName = s[0];
//        }
//    }
//    
    
    if(addonName.empty()){
        ofLogError("baseProject::addAddon") << "cant add addon with empty name";
        return;
    }
    
    //This should be the only instance where we check if the addon is either local or not.
    //being local just means that the addon name is a filepath and it starts with a dot.
    //otherwise it will look in the addons folder.
    //A local addon is not restricted to one that lives in folder with the name local_addons, should be any valid addon on the filesystem.
    //Parsing will generate the correct path to both OF and the project.
    //Everything else should be treated exactly in the same way, regardless of it being local or not.
//    if(addonName[0] == '.' && fs::exists( ofFilePath::join(projectDir, addonName))){
//        
//        addon.addonPath = normalizePath(ofFilePath::join(projectDir, addonName));
//        addon.isLocalAddon = true;
//        ofLogVerbose() << "Adding local addon: " << addonName;
//        //        addon.pathToProject = makeRelative(getOFRoot(), projectDir);
//        //        projectDir;
//    }else{
//        addon.addonPath = fs::path { getOFRoot() / "addons" / addonName };
//    }
//    addon.pathToOF = getOFRoot();
//    
//    
//    
//    addon.pathToOF = normalizePath(addon.pathToOF);
//    addon.addonPath = normalizePath(addon.addonPath);
//    
//    addon.pathToProject = projectDir;
    
    ofAddon addon;
    
//    bool addonOK = false;
//    bool inCache = isAddonInCache(addonName, target);
    
    //	fs::path addonPath { addonName };
    
    //	if (fs::exists(addonPath)) {
    //		addon.isLocalAddon = true;
    //	} else {
    //		addonPath = fs::path { getOFRoot() / "addons" / addonName };
    //		addon.isLocalAddon = false;
    //		addon.addonPath = addonPath;
    //	}
    
//    ofLogVerbose() << "addon.addonPath to: [" << addon.addonPath.string() << "]";
//    ofLogVerbose() << "addon.pathToOF: [" << addon.pathToOF.string() << "]";
    
    if(isAddonInCache(addonName, target)){
        addon = addonsCache[target][addonName];
    }else{
        if(addon.load(_addonName, projectDir, target)){
            addonsCache[target][addonName] = addon;
        }else{
            ofLogVerbose("baseProject::addAddon") << "Ignoring addon that doesn't seem to exist: " << _addonName;
            return; //if addon does not exist, stop early
        }
    }
//    if (!inCache) {
//        addonOK = addon.fromFS(addon.addonPath, target);
//    } else {
//        addon = addonsCache[target][addonName];
//        addonOK = true;
//    }
    
//    if(!addonOK){
//        ofLogVerbose() << "Ignoring addon that doesn't seem to exist: " << addonName;
//        return; //if addon does not exist, stop early
//    }
    
//    if(!inCache){
//        //cache the addon so we dont have to be reading form disk all the time
//        addonsCache[target][addonName] = addon;
//    }
//    
//    for (auto & a : addons) {
//        if (a.name == addon.name) return;
//    }
//    
//    
//    for (auto & d : addon.dependencies) {
//        bool found = false;
//        for (auto & a : addons) {
//            if (a.name == d) {
//                found = true;
//                break;
//            }
//        }
//        if (!found) {
//            baseProject::addAddon(d);
//        } else {
//            ofLogVerbose() << "trying to add duplicated addon dependency! skipping: " << d;
//        }
//    }
//    
    
    ofLogNotice() << "adding addon: " << addon.name;
    

    // MARK: - SPECIFIC for each project.
    // XCode and VS override the base addAddon. other templates will use baseproject::addAddon(ofAddon...
    addAddon(addon);
}

void baseProject::copyAddonData(ofAddon& addon){
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
						fs::copy_file(from, to, fs::copy_options::overwrite_existing); // this is addon files
						ofLogVerbose() << "adding addon data file: " << d << endl;
					} catch(fs::filesystem_error& e) {
						ofLogWarning() << "Can not add addon data file: " << to.string() << " :: " << e.what() << std::endl;;
					}

				} else if (fs::is_directory(from)) {
					if (!fs::exists(to)) {
						try {
						   fs::create_directory(to);
						} catch (const std::exception& e) {
						   std::cerr << "Error creating directories: " << e.what() << std::endl;
						}
					}

					try {
						fs::copy(from, to, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
						ofLogVerbose() << "adding addon data file: " << d << endl;
					} catch(fs::filesystem_error& e) {
						ofLogWarning() << "Can not add addon data file: " << to.string() << " :: " << e.what() << std::endl;
					}
				}
			} else {
				ofLogWarning() << "addon data file does not exist, skipping: " << d;
			}
		}
	}
}

void baseProject::addAddonDllsToCopy(ofAddon& addon){
    // It was part exclusive of visualStudioProject. now it is part of baseProject, so dlls are copied in VSCode project and .so files in linux
    for (auto & d : addon.dllsToCopy) {
        ofLogVerbose() << "adding addon dlls to bin: " << d;
        fs::path from { d };
        fs::path to { projectDir / "bin" / from.filename() };
        if (from.extension() == ".so") {
            fs::path folder { projectDir / "bin" / "libs" };
            if (!fs::exists(folder)) {
                try {
                    fs::create_directories(folder);
                } catch(fs::filesystem_error& e) {
                    ofLogError("baseProject::addAddon") << "error creating folder " << folder;
                    ofLogError() << e.what();
                }
            }
//            to = projectDir / "bin" / "libs" / from.filename();
            to = folder / from.filename();
        }
        if (from.extension() == ".dll") {
            if (d.string().find("x64") != std::string::npos) {
                to = fs::path { projectDir / "dll/x64" / from.filename() };
                ofLogVerbose() << "adding addon dlls to dll/x64: " << d;
            } else if (d.string().find("ARM64EC") != std::string::npos) {
                to = fs::path { projectDir / "dll/ARM64EC" / from.filename()};
                ofLogVerbose() << "adding addon dlls to dll/ARM64EC: " << d;
            } else if (d.string().find("ARM64") != std::string::npos) {
                to = fs::path { projectDir / "dll/ARM64" / from.filename() };
                ofLogVerbose() << "adding addon dlls to dll/ARM64: " << d;
            } else {
                // Default case if architecture is not found
                to = fs::path { projectDir / "bin" / from.filename() };
                ofLogVerbose() << "adding addon dlls to bin: " << d;
            }
        }

        try {
            fs::copy_file(from, to, fs::copy_options::overwrite_existing); // always overwrite DLLS
        } catch(fs::filesystem_error& e) {
            ofLogError("baseProject::addAddon") << "error copying template file " << from << endl << "to: " << to << endl <<  e.what();
        }
    }
}


void baseProject::addAddon(ofAddon & addon){
//	alert("baseProject::addAddon ofAddon & addon :: " + addon.name);

	// FIXME: Duplicate Code
	for (auto & a : addons) {
		if (a.name == addon.name) {
			return;
		}
	}
	
	addon.prepareForWrite();

	/*

	 MARK: Test this, I suppose this is only invoked when an addon is added
	 from a dependency of another addon, and it has its own dependencies too.

	 this is not possible to test easily using xcode or visualstudio project
	 because baseProject::addAddon is only invoked when there is already dependencies in xcode addons.
	 the only way to test this is other platforms like qbs?
	 unless there is one addon added which needs another, and it needs another.

	 */
//	alert("---> dependencies");
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
//	alert("---> dependencies");
	addons.emplace_back(addon);
	

	//ofLogVerbose("baseProject") << "libs in addAddon " << addon.libs.size();

    addAddonBegin(addon);

    addAddonDllsToCopy(addon);
    
    addAddonLibsPaths(addon);
	addAddonIncludePaths(addon);
	addAddonLibs(addon);
	addAddonCflags(addon);
	addAddonCppflags(addon);
	addAddonLdflags(addon);
	addAddonSrcFiles(addon);
	addAddonCsrcFiles(addon);
	addAddonCppsrcFiles(addon);
	addAddonObjcsrcFiles(addon);
	addAddonHeadersrcFiles(addon);


	addAddonDefines(addon);
	addAddonFrameworks(addon);
    copyAddonData(addon);
    addAddonProps(addon);
}


void  baseProject::addAddonLibsPaths(const ofAddon& addon){
	if(addon.libsPaths.size()){
		ofLogWarning("baseProject::addAddonLibsPaths") << "this is not implemented!";
	}
    for (auto & lib: addon.libsPaths){
        ofLogVerbose("adding lib paths") << lib.string();
    }
}


void  baseProject::addAddonIncludePaths(const ofAddon& addon){
    for (auto & e : addon.includePaths) {
        ofLogVerbose("baseProject") << "----------------------------------------------------------------";
        fs::path normalizedDir = normalizePath(projectDir);
        ofLogVerbose("baseProject") << "[addon.includePaths] adding addon include path: [" << e.string() << "]";
        if (containsSourceFiles(normalizedDir)) {
            normalizedDir = makeRelative(projectDir, e);
            ofLogVerbose() << "[addon.includePaths] contains src - Adding dir: [" << normalizedDir.string() << "]";
            // fs::path ofpathChanged = ofRelativeToOFPATH(projectDir);
            // ofLogVerbose() << "[addon.includePaths] OFPATH: rel include dir: [" << ofpathChanged.string() << "]";
            
            addInclude(normalizedDir);
        } else {
            ofLogVerbose() << "[addon.includePaths] no src - not adding: [" << normalizedDir.string() << "]";
        }
    }
    
    
//    for (auto & a : addon.includePaths) {
//        fs::path normalizedDir = makeRelative(projectDir, a);
//        ofLogVerbose() << "adding addon include path: [" << normalizedDir.string() + "]";
//        addInclude(normalizedDir);
//    }
}

void baseProject::addAddonDefines(const ofAddon& addon) {
	for (auto & a : addon.defines) {
		ofLogVerbose() << "adding addon defines: [" << a << "]";
		addDefine(a);
	}
}

void  baseProject::addAddonLibs(const ofAddon& addon){
    for (auto & a : addon.libs) {
        ofLogVerbose("baseProject") << "adding addon libs: " << a.path.string();
        addLibrary(a);
    }
}

void  baseProject::addAddonCflags(const ofAddon& addon){
    for (auto & a : addon.cflags) {
        ofLogVerbose("baseProject") << "adding addon cflags: " << a;
        addCFLAG(a);
    }
}

void  baseProject::addAddonCppflags(const ofAddon& addon){
    for (auto & a : addon.cppflags) {
        ofLogVerbose("baseProject") << "adding addon cppflags: " << a;
        addCPPFLAG(a);
    }
}

void  baseProject::addAddonLdflags(const ofAddon& addon){
    for (auto & a : addon.ldflags) {
        ofLogVerbose("baseProject") << "adding addon ldflags: " << a;
        addLDFLAG(a);
    }
}

void baseProject::addSrcFiles(ofAddon& addon, const vector<fs::path> &filepaths, SrcType type, bool bFindInFilesToFolder){
	for (auto &s : filepaths) {			
		if (bFindInFilesToFolder && (addon.filesToFolders.find(s) == addon.filesToFolders.end())) {
			addon.filesToFolders[s] = fs::path{""};
		}
		fs::path normalizedDir = makeRelative(getOFRoot(), s);
		ofLogVerbose("baseProject::addSrcFiles") << "Adding addon " << toString(type) << " source file: [" << s.string() << "] folder:[" << addon.filesToFolders[s].string() << "]";
		addSrc(normalizedDir, addon.filesToFolders[s]);
	}
}

void  baseProject::addAddonSrcFiles( ofAddon& addon){
	addSrcFiles(addon, addon.srcFiles, DEFAULT);

    // for (auto & a : addon.srcFiles) {
    //     fs::path normalizedDir = makeRelative(getOFRoot(), a);
    //     ofLogVerbose("baseProject") << "adding addon srcFiles: " << normalizedDir.string();
    //     addSrc(normalizedDir, addon.filesToFolders.at(a));
    // }
}

void  baseProject::addAddonCsrcFiles(ofAddon& addon){
    // for (auto & a : addon.csrcFiles) {
    //     fs::path normalizedDir = makeRelative(getOFRoot(), a);
    //     ofLogVerbose("baseProject") << "adding addon c srcFiles: " << normalizedDir.string();
    //     addSrc(normalizedDir, addon.filesToFolders.at(a), C);
    // }
	addSrcFiles(addon, addon.csrcFiles, C);
}



void baseProject::addAddonCppsrcFiles(ofAddon& addon) {
	addSrcFiles(addon, addon.cppsrcFiles, CPP);
}
void baseProject::addAddonObjcsrcFiles(ofAddon& addon) {
	addSrcFiles(addon, addon.objcsrcFiles, OBJC);
}
void baseProject::addAddonHeadersrcFiles(ofAddon& addon) {
	addSrcFiles(addon, addon.headersrcFiles, HEADER);
}

// void  baseProject::addAddonCppsrcFiles(const ofAddon& addon){
//     for (auto & a : addon.cppsrcFiles) {
//         fs::path normalizedDir = makeRelative(getOFRoot(), a);
//         ofLogVerbose("baseProject") << "adding addon cpp srcFiles: " << normalizedDir.string();
//         addSrc(normalizedDir, addon.filesToFolders.at(a),CPP);
//     }
// }

// void  baseProject::addAddonObjcsrcFiles(const ofAddon& addon){
//     for (auto & a : addon.objcsrcFiles) {
//         fs::path normalizedDir = makeRelative(getOFRoot(), a);
//         ofLogVerbose("baseProject") << "adding addon objc srcFiles: " << normalizedDir.string();
//         addSrc(normalizedDir, addon.filesToFolders.at(a),OBJC);
//     }
// }

// void  baseProject::addAddonHeadersrcFiles(const ofAddon& addon){
//     for (auto & a : addon.headersrcFiles) {
//         fs::path normalizedDir = makeRelative(getOFRoot(), a);
//         ofLogVerbose("baseProject") << "adding addon header srcFiles: [" << normalizedDir.string() << "]";
//         addSrc(normalizedDir, addon.filesToFolders.at(a),HEADER);
//     }
// }








void baseProject::addSrcRecursively(const fs::path & srcPath){
//	alert("addSrcRecursively " + srcPath.string(), 32);
	ofLog() << "[addSrcRecursively] using additional source folder " << srcPath.string();
	extSrcPaths.emplace_back(srcPath);
	vector < fs::path > srcFilesToAdd;
	getFilesRecursively(srcPath, srcFilesToAdd);
//	bool isRelative = ofIsPathInPath(fs::absolute(srcPath), getOFRoot());

	std::set<fs::path> uniqueIncludeFolders;
	fs::path base = srcPath.parent_path();

	for( auto & src : srcFilesToAdd){
		fs::path parent = src.parent_path();
		fs::path folder = fs::path("external_sources") / parent.lexically_relative(base);
//		fs::path folder = parent.lexically_relative(base);
		ofLogVerbose() << "[addSrcRecursively] srcFilesToAdd:[" << src.string() << "]";
		addSrc(src, folder);
		if (parent.string() != "") {
			uniqueIncludeFolders.insert(parent);
		}
	}
	
	for(auto & i : uniqueIncludeFolders){
		fs::path normalizedDir = normalizePath(projectDir);
		ofLogVerbose() << "[addSrcRecursively] search include paths for folder: [" << normalizedDir.string() << "]";
		if (containsSourceFiles(normalizedDir)) {
			normalizedDir = makeRelative(projectDir, i);
			ofLogVerbose() << "[addSrcRecursively] uniqueIncludeFolders: contains src - Adding dir: [" << normalizedDir.string() << "]";
		   addInclude(normalizedDir);
	   } else {
		   ofLogVerbose() << "[addSrcRecursively] uniqueIncludeFolders: no src - not adding: [" << normalizedDir.string() << "]";
	   }
	}
}


void baseProject::parseAddons(){
	fs::path parseFile { fs::relative(projectDir / "addons.make") };
//	alert (parseFile.string(), 31);
	
	for (auto & line : fileToStrings(parseFile)) {
		auto addon = ofTrim(line);
//		alert("line " + addon);
		if(addon[0] == '#') continue;
		if(addon == "") continue;
//		auto s = ofSplitString(addon, "#")[0];
//		addAddon(ofSplitString(addon, "#")[0]);
        //we want to keep the comments in the addons.make file since those are use in the package manage
        addAddon(addon);
	}
}

void baseProject::parseConfigMake(){
	fs::path parseFile { fs::relative(projectDir / "config.make") };

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
//	if (fs::is_empty(fs::relative(srcDir))) {
//		alert("recursiveCopy false " + srcDir.string(), 34 );
//		return false;
//	}
//	alert("recursiveCopy " + fs::relative(srcDir).string() + " : " + fs::relative(destDir).string(), 32);
	try {
		fs::copy(fs::relative(srcDir), fs::relative(destDir), (bOverwrite ? fs::copy_options::overwrite_existing : fs::copy_options::update_existing) | fs::copy_options::recursive);
		return true;
	} catch (std::exception& e) {
		ofLogError() << "baseProject::recursiveCopy " << e.what();
		return false;
	}
}

// FIXME: Avoid copying duplicate files like Makefile / config.make when using multiple templates (ex. vscode / osx)
bool baseProject::copyTemplateFile::run() {
	from = fs::relative(from);
	to = fs::relative(to);
	
	// needed for mingw only. maybe a ifdef here.
	if (fs::exists(from)) {
		ofLogVerbose() << "copyTemplateFile from: " << from << " to: " << to;

		if (findReplaces.size()) {
			// Load file, replace contents, write to destination.
			
			std::ifstream fileFrom(from);
			std::string contents((std::istreambuf_iterator<char>(fileFrom)), std::istreambuf_iterator<char>());
			fileFrom.close();

			for (auto & f : findReplaces) {
				// Avoid processing empty pairs
				if (empty(f.first) && empty(f.second)) {
					continue;
				}
				replaceAll(contents, f.first, f.second);
				ofLogVerbose() << "└─ Replacing " << f.first << " : " << f.second;
			}
			
			std::ofstream fileTo(to);
			try{
				fileTo << contents;
			}catch(std::exception & e){
				ofLogError() << "Error saving to " << to;
				ofLogError() << e.what();
				return false;
			}catch(...){
				ofLogError() << "Error saving to " << to;

				return false;
			}
			
			
		} else {
			// straight copy
			try {
				fs::copy(from, to, fs::copy_options::update_existing);
			}
			catch(fs::filesystem_error & e) {
				ofLogError() << "error copying template file " << from << " : " << to ;
				ofLogError() << e.what();
				return false;
			}
		}
	} else {
		return false;
	}
	return true;
}
