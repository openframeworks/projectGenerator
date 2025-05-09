#include "xcodeProject.h"
#include "Utils.h"
#include "ofUtils.h"
#include <nlohmann/json.hpp>
#ifdef __APPLE__
	#include <cstdlib> // std::system
//	#include <regex>
#endif
#include <fstream>
#include <iostream>

using nlohmann::json;

string xcodeProject::LOG_NAME = "xcodeProjectFile";

xcodeProject::xcodeProject(const string & target)
	: baseProject(target) {
	// TODO: remove unused variables
	if (target == "osx") {
		folderUUID = {
			{ "src", "E4B69E1C0A3A1BDC003C02F2" },
			{ "addons", "BB4B014C10F69532006C3DED" },
			{ "openFrameworks", "191EF70929D778A400F35F26" },
			{ "", "E4B69B4A0A3A1720003C02F2" }
			// { "localAddons",	"6948EE371B920CB800B5AC1A" },
		};

		buildConfigurationListUUID = "E4B69B5A0A3A1756003C02F2";
		buildActionMaskUUID = "E4B69B580A3A1756003C02F2";

		projRootUUID = "E4B69B4A0A3A1720003C02F2";
		resourcesUUID = "";
//		frameworksUUID = "E7E077E715D3B6510020DFD4"; //PBXFrameworksBuildPhase
		afterPhaseUUID = "928F60851B6710B200E2D791";
		buildPhasesUUID = "E4C2427710CC5ABF004149E2";
		uuidMap["linkBinaryWithLibraries"] = "E4B69B590A3A1756003C02F2";

	} else { // IOS

		buildConfigurations[0] = "1D6058940D05DD3E006BFB54"; // iOS Debug
		buildConfigurations[1] = "1D6058950D05DD3E006BFB54"; // iOS Release
		buildConfigurations[2] = "C01FCF4F08A954540054247B"; // iOS Debug
		buildConfigurations[3] = "C01FCF5008A954540054247B"; // iOS Release

		buildConfigs[0] = "1D6058940D05DD3E006BFB54"; // iOS Debug
		buildConfigs[1] = "1D6058950D05DD3E006BFB54"; // iOS Release

		//		buildConfigs[0] = "C01FCF4F08A954540054247B"; // iOS Debug
		//		buildConfigs[1] = "C01FCF5008A954540054247B"; // iOS Release

		folderUUID = {
			{ "src", "E4D8936A11527B74007E1F53" },
			{ "addons", "BB16F26B0F2B646B00518274" },
			{ "", "29B97314FDCFA39411CA2CEA" },
			{ "Frameworks", "901808C02053638E004A7774" }
			// { "localAddons", 	"6948EE371B920CB800B5AC1A" },
		};

		buildConfigurationListUUID = "1D6058900D05DD3D006BFB54"; //check
		buildActionMaskUUID = "1D60588E0D05DD3D006BFB54";
		projRootUUID = "29B97314FDCFA39411CA2CEA"; //mainGroup — OK
		resourcesUUID = "BB24DD8F10DA77E000E9C588";
		copyBundleResourcesUUID = "1D60588D0D05DD3D006BFB54";
//		frameworksUUID = "1DF5F4E00D08C38300B7A737"; //PBXFrameworksBuildPhase  // todo: check this?
		afterPhaseUUID = "928F60851B6710B200E2D791";
		buildPhasesUUID = "9255DD331112741900D6945E";
		uuidMap["linkBinaryWithLibraries"] = "1D60588F0D05DD3D006BFB54";
	}
};

bool xcodeProject::createProjectFile() {
	fs::path xcodeProject = projectDir / (projectName + ".xcodeproj");
	try {
		fs::create_directories(xcodeProject);
	} catch (const std::exception & e) {
		std::cerr << "Error creating directories: " << e.what() << std::endl;
		return false;
	}

	// FIXME: rootReplacements can be empty.
	// if project is outside OF, rootReplacements is set to be used in XCode and make
	if (!fs::equivalent(getOFRoot(), normalizePath(fs::path { "../../.." }))) {
		string root { ofPathToString(getOFRoot()) };
		rootReplacements = { "../../..", root };
	}

	copyTemplateFiles.push_back({ normalizePath(templatePath / "emptyExample.xcodeproj" / "project.pbxproj"),
		normalizePath(xcodeProject / "project.pbxproj"),
		{ { "emptyExample", projectName },
			rootReplacements } });

	copyTemplateFiles.push_back({ normalizePath(templatePath / "Project.xcconfig"),
		normalizePath(projectDir / "Project.xcconfig"),
		{ rootReplacements } });

	if (target == "osx" || target == "macos") {
		for (auto & f : { "openFrameworks-Info.plist", "of.entitlements" }) {
			copyTemplateFiles.push_back({ normalizePath(templatePath / f), normalizePath(projectDir / f) });
		}
	} else if (target == "ios" || target == "tvos" || target == "visionos" || target == "catos" || target == "macos") {
		for (auto & f : { "ofxiOS-Info.plist", "ofxiOS_Prefix.pch" }) {
			copyTemplateFiles.push_back({ normalizePath(templatePath / f), normalizePath(projectDir / f) });
			try {
				fs::path from = normalizePath(templatePath / "mediaAssets");
				fs::path to = normalizePath(projectDir / "mediaAssets");
				if (!fs::exists(to)) {
					fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::update_existing);
				}
			} catch (const std::exception & e) {
				std::cerr << "Error copying template files: " << e.what() << std::endl;
				return false;
			}
		}
	}

	if (backupProjectFiles) {
		createBackup({ xcodeProject / "project.pbxproj" }, projectDir);
		createBackup({ projectDir / "openFrameworks-Info.plist" }, projectDir);
		createBackup({ projectDir / "Project.xcconfig" }, projectDir);
		createBackup({ projectDir / "of.entitlements" }, projectDir);
		createBackup({ projectDir / "addons.make" }, projectDir);
		createBackup({ projectDir / "config.make" }, projectDir);
		createBackup({ projectDir / "Makefile" }, projectDir);
	}

	saveScheme();

	if (target == "osx" || target == "macos") {
		saveMakefile();
	}

	// Execute all file copy and replacements, including ones in saveScheme, saveMakefile
	for (auto & c : copyTemplateFiles) {
		try {
			c.run();
		} catch (const std::exception & e) {
			std::cerr << "Error running copy template files: " << e.what() << std::endl;
			return false;
		}
	}

	// NOW only files being copied
	fs::path projectDataDir { projectDir / "bin" / "data" };

	if (!fs::exists(projectDataDir)) {
		cout << "creating dataDir " << projectDataDir << endl;
		fs::create_directories(projectDataDir);
	}

	if (fs::exists(projectDataDir)) {
		// originally only on IOS
		//this is needed for 0.9.3 / 0.9.4 projects which have iOS media assets in bin/data/
		fs::path templateBinDir { templatePath / "bin" };
		fs::path templateDataDir { templatePath / "bin" / "data" };
		if (fs::exists(templateDataDir) && fs::is_directory(templateDataDir)) {
			baseProject::recursiveCopyContents(templateDataDir, projectDataDir);
		}
	}
	return true;
}

void xcodeProject::saveScheme() {
	auto schemeFolder = projectDir / (projectName + ".xcodeproj") / "xcshareddata/xcschemes";

	if (fs::exists(schemeFolder)) {
		fs::remove_all(schemeFolder);
	}
	fs::create_directories(schemeFolder);

	if (target == "osx") {
		for (auto & f : { "Release", "Debug" }) {
			copyTemplateFiles.push_back({ templatePath / ("emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample " + string(f) + ".xcscheme"),
				schemeFolder / (projectName + " " + f + ".xcscheme"),
				{ { "emptyExample", projectName } } });
		}

		copyTemplateFiles.push_back({ projectDir / (projectName + ".xcodeproj/project.xcworkspace"),
			templatePath / "emptyExample.xcodeproj/project.xcworkspace" });
	} else {

		// MARK:- IOS sector;
		copyTemplateFiles.push_back({ templatePath / "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample.xcscheme",
			schemeFolder / (projectName + ".xcscheme"),
			{ { "emptyExample", projectName } } });
	}
}

void xcodeProject::saveMakefile() {
	copyTemplateFiles.push_back({ templatePath / "Makefile", projectDir / "Makefile",
		{ rootReplacements } });
	copyTemplateFiles.push_back({ templatePath / "config.make", projectDir / "config.make" });
}

bool xcodeProject::loadProjectFile() { //base
	addCommand("# ---- PG VERSION " + getPGVersion());
	addCommand("Add :_OFProjectGeneratorVersion string " + getPGVersion());

	// Rename Project
	addCommand("Set :objects:" + buildConfigurationListUUID + ":name string " + projectName);

	// Just OSX here, debug app naming.
	if (target == "osx") {
		// TODO: Hardcode to variable
		// FIXME: Debug needed in name?
		addCommand("Set :objects:E4B69B5B0A3A1756003C02F2:path string " + projectName + "Debug.app");
	}
	
	// Next block updates ofPath in xcode addons and openframeworks folder
	// only if it is not the usual ../../.. path
	
	bool updateOFPath = false;
	fs::path of = getOFRoot();
	if (!ofIsPathInPath(fs::current_path(), getOFRoot())) {
		updateOFPath = true;
		addCommand("Set :objects:" + folderUUID["openFrameworks"] + ":sourceTree string <absolute>");
		addCommand("Set :objects:" + folderUUID["addons"] + ":sourceTree string <absolute>");
	} else {
		// inside OF Root but not in usual depth
		if (!fs::equivalent(getOFRoot(), "../../..")) {
			updateOFPath = true;
			of = fs::relative(getOFRoot(), fs::current_path());
		}
	}
	if (updateOFPath) {
		addCommand("Set :objects:" + folderUUID["openFrameworks"] + ":path string " + of.string() + "/libs/openFrameworks");
		addCommand("Set :objects:" + folderUUID["addons"] + ":path string " + of.string() + "/addons");
	}

	return true;
}

fs::path getPathTo(fs::path path, string limit) {
	fs::path p;
	vector<fs::path> folders = std::vector(path.begin(), path.end());
	for (auto & f : folders) {
		p /= f;
		if (f.string() == limit) {
			//            alert("getPathTo "+  p.string(), 33);
			return p;
		}
	}
	return p;
}

string xcodeProject::getFolderUUID(const fs::path & folder, fs::path base) { //, bool isFolder, fs::path base) {
	//    alert ("xcodeProject::getFolderUUID "+folder.string());//+" : isfolder="+ofToString(isFolder)+" : base="+ base.string());

	//	TODO: Change key of folderUUID to base + folder, so "src" in additional source folders
	//	doesn't get confused with "src" from project.
	//	this can work but fullPath variable has to follow the same pattern

	auto fullPathFolder = folder;

	// If folder UUID exists just return it.
	if (folderUUID.find(fullPathFolder) != folderUUID.end()) { // NOT FOUND
		return folderUUID[fullPathFolder];
	} else {
		// in this case it is not found, so it creates UUID for the entire path

		vector<fs::path> folders = std::vector(folder.begin(), folder.end());
		string lastFolderUUID = projRootUUID;
		string lastFolder = "";

		if (folders.size()) {
			// Iterating every folder from full path

			for (std::size_t a = 0; a < folders.size(); a++) {
				fs::path fullPath { "" };

				std::vector<fs::path> joinFolders;
				joinFolders.reserve(a + 1); // Reserve / avoid reallocations

				for (std::size_t i = 0; i <= a; ++i) {
					joinFolders.push_back(folders[i]);
				}

				for (const auto & j : joinFolders) {
					fullPath /= j;
				}

				//                alert("xcodeProject::getFolderUUID fullpath: " + fullPath.string(),33);

				// Query if partial path is already stored. if not execute this following block
				if (folderUUID.find(fullPath) != folderUUID.end()) {
					lastFolderUUID = folderUUID[fullPath];
					lastFolder = ofPathToString(folderFromUUID[lastFolderUUID]);
				}

				else {

					string thisUUID = generateUUID(fullPath);
					folderUUID[fullPath] = thisUUID;
					folderFromUUID[thisUUID] = fullPath;

					addCommand("");
					string folderName = ofPathToString(folders[a]);
					addCommand("Add :objects:" + thisUUID + ":name string " + folderName);

					// FIXME: Inspect if this is really being used
					//					if (isFolder) {
					//						alert("getFolderUUID, isFolder INSIDE " , 31);
					//						fs::path filePath;
					//						fs::path filePath_full { relRoot / fullPath };
					//						// FIXME: known issue: doesn't handle files with spaces in name.
					//
					//						if (fs::exists(filePath_full)) {
					//							filePath = filePath_full;
					//						}
					//						if (fs::exists(fullPath)) {
					//							filePath = fullPath;
					//						}
					//
					//						if (!filePath.empty()) {
					//							addCommand("Add :objects:"+thisUUID+":path string " + ofPathToString(filePath));
					//						} else {
					//						}
					//					} else {
					////						alert("getFolderUUID isFolder false", 31);
					//					}

					addCommand("Add :objects:" + thisUUID + ":isa string PBXGroup");

					bool bFolderPathSet = false;

					if (folderName == "external_sources" || folderName == "local_addons") {

						addCommand("Add :objects:" + thisUUID + ":sourceTree string SOURCE_ROOT");
						addCommand("Add :objects:" + thisUUID + ":path string ");
						bFolderPathSet = true;
					} else {
						if (lastFolderUUID == projRootUUID) { //} ||
							//                            lastFolder == "external_sources" || lastFolder == "local_addons") { //

							// Base folders can be in a different depth,
							// so we cut folders to point to the right path
							// ROY: THIS hardly makes sense to me. I can see the purpose of it. base2 is never set to anything.
							fs::path base2 { "" };
							size_t diff = folders.size() - (a + 1);
							for (size_t x = 0; x < diff; x++) {
								base2 = base2.parent_path();
							}
							//                            alert ("external_sources base = " + ofPathToString(base2) + " UUID: " + thisUUID, 33);
							addCommand("Add :objects:" + thisUUID + ":sourceTree string SOURCE_ROOT");
							addCommand("Add :objects:" + thisUUID + ":path string " + ofPathToString(base2));
							bFolderPathSet = true;
						} else if (lastFolder == "external_sources" || lastFolder == "local_addons") { //
							addCommand("Add :objects:" + thisUUID + ":sourceTree string SOURCE_ROOT");
							//                            alert ("xxxx " + lastFolder + "  " + ofPathToString(base) + " UUID: " + thisUUID, 33);
							addCommand("Add :objects:" + thisUUID + ":path string " + ofPathToString(getPathTo(base, folderName)));
							bFolderPathSet = true;
						} else {
							addCommand("Add :objects:" + thisUUID + ":sourceTree string <group>");
							//							fs::path addonFolder { fs::path(fullPath).filename() };
							addCommand("Add :objects:" + thisUUID + ":path string " + ofPathToString(fullPath.filename()));
							bFolderPathSet = true;
						}
					}

					addCommand("Add :objects:" + thisUUID + ":children array");

					if (!bFolderPathSet) {
						if (folder.begin()->string() == "addons" || folder.begin()->string() == "src") { //} || folder.begin()->string() == "local_addons") {
							addCommand("Add :objects:" + thisUUID + ":sourceTree string <group>");
							//						fs::path addonFolder { fs::path(fullPath).filename() };
							addCommand("Add :objects:" + thisUUID + ":path string " + ofPathToString(fullPath.filename()));
							// alert ("group " + folder.string() + " : " + base.string() + " : " + addonFolder.string(), 32);
						} else {
							addCommand("Add :objects:" + thisUUID + ":sourceTree string SOURCE_ROOT");
						}
					}

					// Add this new folder to its parent, projRootUUID if root
					addCommand("Add :objects:" + lastFolderUUID + ":children: string " + thisUUID);

					// keep this UUID as parent for the next folder.
					lastFolderUUID = thisUUID;
					lastFolder = folderName;
				}
			}
		}
		return lastFolderUUID;
	}
}

void xcodeProject::addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type) {
	//	alert ("xcodeProject::addSrc " + ofPathToString(srcFile) + " : " + ofPathToString(folder), 31);

	string ext = ofPathToString(srcFile.extension());

	//		.reference = true,
	//		.addToBuildPhase = true,
	//		.codeSignOnCopy = false,
	//		.copyFilesBuildPhase = false,
	//		.linkBinaryWithLibraries = false,
	//		.addToBuildResource = false,
	//		.addToResources = false,

	fileProperties fp;
	fp.addToBuildPhase = true;
	fp.isSrc = true;

	if (type == DEFAULT) {

		if (ext == ".h" || ext == ".hpp") {
			fp.addToBuildPhase = false;
		} else if (ext == ".xib") {
			fp.addToBuildPhase = false;
			fp.addToBuildResource = true;
			fp.addToResources = true;
		} else if (ext == ".metal") {
			fp.addToBuildResource = true;
			fp.addToResources = true;
		} else if (ext == ".entitlements") {
			fp.addToBuildResource = true;
			fp.addToResources = true;
		} else if (ext == ".info") {
			fp.addToBuildResource = true;
			fp.addToResources = true;
		} else if (ext == ".storyboard") {
			fp.addToBuildPhase = false;
			fp.copyBundleResources = true;
		} else if (ext == ".plist") {
			fp.addToBuildPhase = true;
			fp.copyBundleResources = true;
		} else if (ext == ".swift") {
			fp.addToBuildPhase = true;
			for (auto & c : buildConfigs) {
				addCommand("Add :objects:" + c + ":buildSettings:OTHER_SWIFT_FLAGS: string " + "-cxx-interoperability-mode=swift-5.9");
				// mark all Swift files as C++ interop available :)
			}
		} else if (ext == ".xcassets") {
			fp.addToBuildResource = true;
			fp.addToResources = true;
		} else if (ext == ".modulemap") {
			fp.addToBuildPhase = false;
		} else if (ext == ".bundle") {
			fp.addToBuildResource = true;
			fp.addToResources = true;
		} else if (ext == ".tbd") {
			fp.linkBinaryWithLibraries = true;
		}
	}
	string UUID {
		addFile(srcFile, folder, fp)
	};

	if (ext == ".mm" || ext == ".m") {
		addCompileFlagsForMMFile(srcFile);
	} else if (ext == ".cpp") {
		if (containsObjectiveCPlusPlus(srcFile)) {
			addCompileFlagsForMMFile(srcFile);
		}
	}
}

void xcodeProject::addCompileFlagsForMMFile(const fs::path & srcFile) {

	std::ifstream file(srcFile);
	if (!file.is_open()) return;

	bool requiresNoARC = false;

	// Named regex for detecting ARC-related function calls in Objective-C++
	const std::regex arcFunctionRegex(R"(\b(alloc|dealloc|retain|release|autorelease)\b)");

	for (std::string line; std::getline(file, line);) {
		if (std::regex_search(line, arcFunctionRegex)) {
			requiresNoARC = true;
			break;
		}
	}

	// Tag file as Objective-C++ in Xcode build settings
	for (auto & c : buildConfigs) {
		//			addCommand("Add :objects:" + c + ":buildSettings:OTHER_CPLUSPLUSFLAGS: string " + "-xobjective-c++");

		//			if (requiresNoARC) {
		//				addCommand("Add :objects:" + c + ":buildSettings:OTHER_CPLUSPLUSFLAGS: string " + "-fno-objc-arc");
		//			}
	}
}

bool xcodeProject::containsObjectiveCPlusPlus(const fs::path & filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) return false;

	// Objective-C++ specific keywords to check -- this will fix the really hard to debug objc++ linking obiguous
	std::vector<std::string> objcKeywords = {
		"#import", "@interface", "@implementation", "@property",
		"@synthesize", "@end"
	};

	for (std::string line; std::getline(file, line);) {
		for (const auto & keyword : objcKeywords) {
			if (line.find(keyword) != std::string::npos) {
				return true;
			}
		}
	}
	return false;
}

void xcodeProject::addFramework(const fs::path & path, const fs::path & folder, bool isRelativeToSDK) {
	ofLogVerbose() << "Adding framework " << ofPathToString(path) << "  folder: " << folder;
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);

	addCommand("# ----- addFramework path=" + ofPathToString(path) + " folder=" + ofPathToString(folder));

	fileProperties fp;
	fp.absolute = false;
	fp.codeSignOnCopy = !isRelativeToSDK;
	fp.copyFilesBuildPhase = !isRelativeToSDK;
	fp.isRelativeToSDK = isRelativeToSDK;
	fp.linkBinaryWithLibraries = !folder.empty();

	string UUID;
	if (isRelativeToSDK) {
		fs::path frameworkPath { "System/Library/Frameworks/" + ofPathToString(path) + ".framework" };
		UUID = addFile(frameworkPath, "Frameworks", fp);
	} else {
		UUID = addFile(path, folder, fp);
	}

	if (!isRelativeToSDK) {
		addCommand("# ----- FRAMEWORK_SEARCH_PATHS");
		string parent { ofPathToString(path.parent_path()) };

		for (auto & c : buildConfigs) {
			if (path.extension() == ".framework") {
				addCommand("Add :objects:" + c + ":buildSettings:FRAMEWORK_SEARCH_PATHS: string " + parent);
			}
			if (path.extension() == ".xcframework") {
				addCommand("Add :objects:" + c + ":buildSettings:XCFRAMEWORK_SEARCH_PATHS: string " + parent);
			}
		}
	}
}

void xcodeProject::addDylib(const fs::path & path, const fs::path & folder) {
	//	alert( "xcodeProject::addDylib " + ofPathToString(path) , 33);

	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);

	fileProperties fp;
	fp.addToBuildPhase = true;
	fp.codeSignOnCopy = true;
	fp.copyFilesBuildPhase = true;

	addFile(path, folder, fp);
}

void xcodeProject::addInclude(const fs::path & includeName) {
	for (auto & c : buildConfigs) {
		addCommand("Add :objects:" + c + ":buildSettings:HEADER_SEARCH_PATHS: string " + ofPathToString(includeName));
	}
}

void xcodeProject::addLibrary(const LibraryBinary & lib) {
	for (auto & c : buildConfigs) {
		addCommand("Add :objects:" + c + ":buildSettings:OTHER_LDFLAGS: string " + ofPathToString(fs::relative(lib.path)));
	}
}

void xcodeProject::addLDFLAG(const string & ldflag, LibType libType) {
	ofLogVerbose("xcodeProject::addLDFLAG") << ldflag;
	for (auto & c : buildConfigs) {
		addCommand("Add :objects:" + c + ":buildSettings:OTHER_LDFLAGS: string " + ldflag);
	}
}

void xcodeProject::addCFLAG(const string & cflag, LibType libType) {
	ofLogVerbose("xcodeProject::addCFLAG") << cflag;
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		addCommand("Add :objects:" + c + ":buildSettings:OTHER_CFLAGS: string " + cflag);
	}
}

void xcodeProject::addDefine(const string & define, LibType libType) {
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		addCommand("Add :objects:" + c + ":buildSettings:GCC_PREPROCESSOR_DEFINITIONS: string " + define);
	}
}

// FIXME: libtype is unused here
void xcodeProject::addCPPFLAG(const string & cppflag, LibType libType) {
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		addCommand("Add :objects:" + c + ":buildSettings:OTHER_CPLUSPLUSFLAGS: string " + cppflag);
	}
}

void xcodeProject::addAfterRule(const string & rule) {
	// return;
	//	cout << ">>>>>> addAfterRule " << rule << endl;
	addCommand("Add :objects:" + afterPhaseUUID + ":buildActionMask string 2147483647");
	// addCommand("Add :objects:"+afterPhaseUUID+":files array");
	// addCommand("Add :objects:"+afterPhaseUUID+":inputPaths array");
	addCommand("Add :objects:" + afterPhaseUUID + ":isa string PBXShellScriptBuildPhase");
	// addCommand("Add :objects:"+afterPhaseUUID+":outputPaths array");
	addCommand("Add :objects:" + afterPhaseUUID + ":runOnlyForDeploymentPostprocessing string 0");
	addCommand("Add :objects:" + afterPhaseUUID + ":shellPath string /bin/sh");
	addCommand("Add :objects:" + afterPhaseUUID + ":showEnvVarsInLog string 0");

	// ofStringReplace(rule, "\"", "\\\"");
	// addCommand("Add :objects:"+afterPhaseUUID+":shellScript string \"" + rule + "\"");
	addCommand("Add :objects:" + afterPhaseUUID + ":shellScript string " + rule);

	// adding this phase to build phases array
	// TODO: Check if nit needs another buildConfigurationListUUID for debug.
	addCommand("Add :objects:" + buildConfigurationListUUID + ":buildPhases: string " + afterPhaseUUID);
}

void xcodeProject::addAddonLibs(const ofAddon & addon) {
	for (auto & e : addon.libs) {
		ofLogVerbose() << "adding addon libs: " << e.path;
		addLibrary(e);

		fs::path dylibPath { e.path };

		//		cout << "dylibPath " << dylibPath << endl;
		if (dylibPath.extension() == ".dylib") {

			if (addon.filesToFolders.find(dylibPath) == addon.filesToFolders.end()) {
				addDylib(dylibPath, dylibPath.parent_path().lexically_relative(addon.pathToOF));
			} else {
				addDylib(dylibPath, addon.filesToFolders.at(dylibPath));
			}
		}
	}
}

void xcodeProject::addAddonSrcFiles(ofAddon & addon) {
	std::sort(addon.srcFiles.begin(), addon.srcFiles.end(), [](const fs::path & a, const fs::path & b) {
		return a.string() < b.string();
	});
	for (auto & e : addon.srcFiles) {
		ofLogVerbose() << "adding addon srcFiles: " << e;
		if (addon.filesToFolders.find(e) == addon.filesToFolders.end()) {

			addSrc(e, "");
		} else {
			addSrc(e, addon.filesToFolders.at(e));
		}
	}
}

//-----------------------------------------------------------------------------------------------
void xcodeProject::addAddonFrameworks(const ofAddon & addon) {
	ofLogVerbose("xcodeProject::addAddonFrameworks") << addon.name;

	std::vector<std::string> allFrameworks;

	allFrameworks.reserve(addon.frameworks.size() + addon.xcframeworks.size());
	allFrameworks.insert(allFrameworks.end(), addon.frameworks.begin(), addon.frameworks.end());
	allFrameworks.insert(allFrameworks.end(), addon.xcframeworks.begin(), addon.xcframeworks.end());

	for (auto & f : allFrameworks) {
		//    for (auto & f : addon.frameworks) {
		ofLogVerbose() << "adding addon frameworks: " << f;
		//		alert ("ADD ADDON FRAMEWORKS " + f, 33);

		auto path = f;
		// The only addon I've found using fixed path to system Frameworks is ofxCoreLocation
		// https://github.com/robotconscience/ofxCoreLocation/blob/533ee4b0d380a4a1aafbe1c5923ae66c26b92d53/addon_config.mk#L32

		if (ofIsStringInString(f, "/System/Library/Frameworks")) {
			fs::path fullPath = f;
			path = ofPathToString(fullPath.filename());
		}

		bool isRelativeToSDK = false;
		size_t found = path.find('/');
		if (found == string::npos) {
			isRelativeToSDK = true;
		}
		fs::path folder = isRelativeToSDK ? "Frameworks" : addon.filesToFolders.at(f);
		addFramework(path, folder, isRelativeToSDK);
	}
}

//-----------------------------------------------------------------------------------------------
string xcodeProject::addFile(const fs::path & path, const fs::path & folder, const fileProperties & fp) {
	string UUID { "" };

	//	cout << "will check if exists " << (projectDir / path) << endl;
	//	if (fs::exists( projectDir / path ))
	{
		//		cout << "OK exists" << endl;
		//		bool isFolder = false;
		string fileType { "file" };
		fileType = extensionToFileType[path.extension()];

		if (fileType == "") {
			if (fs::is_directory(path) || fp.isGroupWithoutFolder) {
				fileType = "folder";
				//				isFolder = true;
			} else {
				// Break here if fileType is not set. and it is not a folder
				return {};
			}
		}

		UUID = generateUUID(path);

		addCommand("");
		addCommand("# -- addFile " + ofPathToString(path));

		// encoding may be messing up for frameworks... so I switched to a pbx file ref without encoding fields

		//		if (fp.reference) {
		//		} else {
		//			addCommand("Add :objects:"+UUID+":isa string PBXGroup");
		//		}

		// This is adding a file. any file.
		addCommand("Add :objects:" + UUID + ":fileEncoding string 4");
		if (fp.isGroupWithoutFolder) {
			addCommand("Add :objects:" + UUID + ":isa string PBXGroup");
		} else {
			addCommand("Add :objects:" + UUID + ":isa string PBXFileReference");
		}
		addCommand("Add :objects:" + UUID + ":lastKnownFileType string " + fileType);
		addCommand("Add :objects:" + UUID + ":name string " + ofPathToString(path.filename()));

		if (fp.absolute) {
			addCommand("Add :objects:" + UUID + ":sourceTree string SOURCE_ROOT");
			if (fs::exists(projectDir / path)) {
				addCommand("Add :objects:" + UUID + ":path string " + ofPathToString(path));
			}
		} else {

			if (folder.begin()->string() == "local_addons" || folder.begin()->string() == "external_sources") {
				//                if(path.is_absolute()){
				addCommand("Add :objects:" + UUID + ":path string " + ofPathToString(path));
				addCommand("Add :objects:" + UUID + ":sourceTree string SOURCE_ROOT");
				//                }else{
				//                    if (fs::exists( projectDir / path )) {
				//                        addCommand("Add :objects:"+UUID+":path string " + ofPathToString(projectDir /path));
				//                    }
				//                }
			} else {
				if (fp.isRelativeToSDK) {
					addCommand("Add :objects:" + UUID + ":path string " + ofPathToString(path));
					addCommand("Add :objects:" + UUID + ":sourceTree string SDKROOT");
				} else {
					addCommand("Add :objects:" + UUID + ":sourceTree string <group>");
				}
			}
		}

		//		string folderUUID;
		//		auto rootDir = folder.root_directory();
		//		if (rootDir != "addons" && rootDir != "src") {
		////			alert("addFile path:" + ofPathToString(path) + " folder:" + ofPathToString(folder) , 31);
		//			auto base = path.parent_path();
		//			folderUUID = getFolderUUID(folder, isFolder, base);
		//
		//		} else {
		//			folderUUID = getFolderUUID(folder, isFolder);
		//		}

		// Eventually remove isFolder and base parameter
		std::string folderUUID { getFolderUUID(folder, path) };
		//, isFolder) };

		addCommand("# ---- addFileToFolder UUID : " + ofPathToString(folder));
		addCommand("Add :objects:" + folderUUID + ":children: string " + UUID);

		string buildUUID { generateUUID(ofPathToString(path) + "-build") };
		// If any other option is true, add buildUUID entries.
		if (
			fp.addToBuildPhase || fp.codeSignOnCopy || fp.copyFilesBuildPhase || fp.addToBuildResource || fp.addToResources || fp.copyBundleResources
		) {
			addCommand("# ---- addFileToBuildUUID " + buildUUID);
			addCommand("Add :objects:" + buildUUID + ":isa string PBXBuildFile");
			addCommand("Add :objects:" + buildUUID + ":fileRef string " + UUID);
		}

		if (fp.addToBuildPhase) { // Compile Sources
			// Not sure if it applies to everything, applies to srcFile.
			addCommand("# ---- addToBuildPhase");
			addCommand("Add :objects:" + buildActionMaskUUID + ":files: string " + buildUUID);
		}

		if (fp.copyBundleResources) {
			addCommand("# ---- copyBundleResources");
			addCommand("Add :objects:" + copyBundleResourcesUUID + ":files: string " + buildUUID);
		}

		if (fp.copyFilesBuildPhase) {
			//			// If we are going to add xcframeworks to copy files -> destination frameworks, we should include here
			if (path.extension() == ".framework") {
				addCommand("# ---- copyPhase Frameworks " + buildUUID);
				addCommand("Add :objects:E4C2427710CC5ABF004149E2:files: string " + buildUUID);
			}
		}
//			//			if (path.extension() == ".framework" || path.extension() == ".xcframework") {
//			// This now includes both .framework and .xcframework
//			if (fileType == "wrapper.framework" || fileType == ".xcframework") {
//				// copy to frameworks
//				addCommand("# ---- copyPhase Frameworks " + buildUUID);
//				addCommand("Add :objects:E4C2427710CC5ABF004149E2:files: string " + buildUUID);
//			} else {
//				// copy to executables
//				addCommand("# ---- copyPhase Executables " + buildUUID);
//				addCommand("Add :objects:E4A5B60F29BAAAE400C2D356:files: string " + buildUUID);
//			}
//		}

		if (fp.codeSignOnCopy) {
			addCommand("# ---- codeSignOnCopy " + buildUUID);
			addCommand("Add :objects:" + buildUUID + ":settings:ATTRIBUTES array");
			addCommand("Add :objects:" + buildUUID + ":settings:ATTRIBUTES: string CodeSignOnCopy");
		}

		if (fp.addToBuildResource) {
			string mediaAssetsUUID { "9936F60E1BFA4DEE00891288" };
			addCommand("# ---- addToBuildResource");
			addCommand("Add :objects:" + mediaAssetsUUID + ":files: string " + UUID);
		}

		if (fp.addToResources) {
			// FIXME: test if it is working on iOS
			if (resourcesUUID != "") {
				addCommand("# ---- addToResources (IOS only) ?" + buildUUID);
				addCommand("Add :objects:" + resourcesUUID + ": string " + buildUUID);
			}
		}

		 if (fp.linkBinaryWithLibraries) { // Link Binary With Libraries
			auto tempUUID = generateUUID(ofPathToString(path) + "-InFrameworks");
			addCommand("Add :objects:" + tempUUID + ":fileRef string " + UUID);
			addCommand("Add :objects:" + tempUUID + ":isa string PBXBuildFile");
			addCommand("# --- linkBinaryWithLibraries PBXFrameworksBuildPhase");
			//			 uuidMap["linkBinaryWithLibraries"] = "E4B69B590A3A1756003C02F2";
			addCommand("Add :objects:" + uuidMap["linkBinaryWithLibraries"] + ":files: string " + tempUUID);
			alert (commands.back(), 96);
		 }
	}
	return UUID;
}

void xcodeProject::addCommand(const string & command) {
	if (debugCommands) {
		alert(command, 31);
	}
	commands.emplace_back(command);
}

bool xcodeProject::saveProjectFile() {

	//	debugCommands = true;

	fileProperties fp;
	//	fp.isGroupWithoutFolder = true;
	//	addFile("additionalSources", "", fp);
	//	fp.isGroupWithoutFolder = false;
	//	addFile("openFrameworks-Info.plist", "", fp);
	//	addFile("of.entitlements", "", fp);
	//	addFile("Project.xcconfig", "", fp);
	if (fs::exists(projectDir / "App.xcconfig")) {
		addFile("App.xcconfig", "", fp);
	}
	fp.absolute = true;
	//	addFile("../../../libs/openframeworks", "", fp);
	addFile(fs::path { "bin" } / "data", "", fp);

	//	debugCommands = false;

	fs::path fileName { projectDir / (projectName + ".xcodeproj/project.pbxproj") };
	bool usePlistBuddy = false;

	if (usePlistBuddy) {
		//	PLISTBUDDY - Mac only
		string command = "/usr/libexec/PlistBuddy " + ofPathToString(fileName);
		string allCommands = "";
		for (auto & c : commands) {
			command += " -c \"" + c + "\"";
			allCommands += c + "\n";
		}
		cout << ofSystem(command) << endl;
	} else {
		// JSON Block - Multiplatform

		std::ifstream contents(fileName);
		//		std::cout << contents.rdbuf() << std::endl;
		json j;
		try {
			j = json::parse(contents);

			// Ugly hack to make nlohmann json work with v 3.11.3
			//			auto dump = j.dump(1, '	');
			//			if (dump[0] == '[') {
			//				j = j[0];
			//			}

		} catch (json::parse_error & ex) {
			ofLogError(xcodeProject::LOG_NAME) << "JSON parse error at byte" << ex.byte;
			ofLogError(xcodeProject::LOG_NAME) << "fileName" << fileName;
		}

		contents.close();

		for (auto & c : commands) {
			//			alert (c, 31);
			// readable comments enabled now.
			if (c != "" && c[0] != '#') {
				vector<string> cols { ofSplitString(c, " ") };
				string thispath { cols[1] };
				ofStringReplace(thispath, ":", "/");

				if (thispath.substr(thispath.length() - 1) != "/") {
					//if (cols[0] == "Set") {
					try {
						json::json_pointer p { json::json_pointer(thispath) };
//						alert (thispath, 95);

						if (cols[2] == "string") {
							// find position after find word
							auto stringStart { c.find("string ") + 7 };
							try {
								j[p] = c.substr(stringStart);
							} catch (std::exception & e) {

								ofLogError() << "substr " << c.substr(stringStart) << "\n"
											 << "pointer " << p << "\n"
											 << e.what();
							}
							// j[p] = cols[3];
						} else if (cols[2] == "array") {
							try {
								//								j[p] = {};
								j[p] = json::array({});
							} catch (std::exception & e) {
								ofLogError() << "array " << e.what();
							}
						}
					} catch (std::exception & e) {
						cout << "pointer " << thispath;
						ofLogError(xcodeProject::LOG_NAME) << "first json error ";
						ofLogError() << e.what();
						ofLogError() << thispath;
						ofLogError() << "-------------------------";
					}

				} else {
					thispath = thispath.substr(0, thispath.length() - 1);
//					alert (thispath, 95);
					json::json_pointer p = json::json_pointer(thispath);
					try {
						// Fixing XCode one item array issue
						if (!j[p].is_array()) {
							//							cout << endl;
							//							alert (c, 31);
							//							cout << "this is not array, creating" << endl;
							//							cout << thispath << endl;
							auto v { j[p] };
							j[p] = json::array();
							if (!v.is_null()) {
								//								cout << "thispath " << thispath << endl;
								j[p].emplace_back(v);
							}
						}
						//						alert (c, 31);
						//						alert ("emplace back " + cols[3] , 32);
						j[p].emplace_back(cols[3]);

					} catch (std::exception & e) {
						ofLogError(xcodeProject::LOG_NAME) << "json error ";
						ofLogError() << e.what();
						ofLogError() << thispath;
						ofLogError() << "-------------------------";
					}
				}
				//				alert("-----", 32);
			}
		}

		std::ofstream jsonFile(fileName);

		// This is not pretty but address some differences in nlohmann json 3.11.2 to 3.11.3
		auto dump = j.dump(1, '	');
		if (dump[0] == '[') {
			dump = j[0].dump(1, '	');
		}

		try {
			jsonFile << dump;
		} catch (std::exception & e) {
			ofLogError("xcodeProject::saveProjectFile") << "Error saving json to " << fileName << ": " << e.what();
			return false;
		} catch (...) {
			ofLogError("xcodeProject::saveProjectFile") << "Error saving json to " << fileName;
			return false;
		}
		jsonFile.close();
	}

	//	for (auto & c : commands) cout << c << endl;
	return true;
}
