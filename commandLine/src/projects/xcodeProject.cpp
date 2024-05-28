#include "xcodeProject.h"
#include "Utils.h"
#include "ofUtils.h"
#if !defined(TARGET_MINGW)
	#include <json.hpp>
#else
	#include <nlohmann/json.hpp> // MSYS2 : use of system-installed include
#endif
#include <iostream>

using nlohmann::json;
using nlohmann::json_pointer;

xcodeProject::xcodeProject(const string & target) : baseProject(target){
	// TODO: remove unused variables
	if( target == "osx" ){
		folderUUID = {
			{ "src", 			"E4B69E1C0A3A1BDC003C02F2" },
			{ "addons", 		"BB4B014C10F69532006C3DED" },
			{ "openFrameworks", "191EF70929D778A400F35F26" },
			{ "", 				"E4B69B4A0A3A1720003C02F2" }
			// { "localAddons",	"6948EE371B920CB800B5AC1A" },
		};

		buildConfigurationListUUID = "E4B69B5A0A3A1756003C02F2";
		buildActionMaskUUID = "E4B69B580A3A1756003C02F2";

		projRootUUID    = "E4B69B4A0A3A1720003C02F2";
		resourcesUUID   = "";
		frameworksUUID  = "E7E077E715D3B6510020DFD4";   //PBXFrameworksBuildPhase
		afterPhaseUUID  = "928F60851B6710B200E2D791";
		buildPhasesUUID  = "E4C2427710CC5ABF004149E2";

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
			{ "src", 			"E4D8936A11527B74007E1F53" },
			{ "addons", 		"BB16F26B0F2B646B00518274" },
			{ "", 				"29B97314FDCFA39411CA2CEA" }
			// { "localAddons", 	"6948EE371B920CB800B5AC1A" },
		};

		buildConfigurationListUUID = "1D6058900D05DD3D006BFB54"; //check
		buildActionMaskUUID =	"1D60588E0D05DD3D006BFB54";
		projRootUUID    = "29B97314FDCFA39411CA2CEA"; //mainGroup — OK
		resourcesUUID   = 			"BB24DD8F10DA77E000E9C588";
		buildPhaseResourcesUUID = 	"1D60588D0D05DD3D006BFB54";
		frameworksUUID  = "1DF5F4E00D08C38300B7A737";   //PBXFrameworksBuildPhase  // todo: check this?
		afterPhaseUUID  = "928F60851B6710B200E2D791";
		buildPhasesUUID = "9255DD331112741900D6945E";
	}
};

bool xcodeProject::createProjectFile(){
	fs::path xcodeProject = projectDir / ( projectName + ".xcodeproj" );
//	alert ("createProjectFile " + ofPathToString(xcodeProject), 35);

	if (fs::exists(xcodeProject)) {
		fs::remove_all(xcodeProject);
	}
	fs::create_directories(xcodeProject);

	if (!fs::equivalent(getOFRoot(), fs::path{"../../.."})) {
		string root { ofPathToString(getOFRoot()) };
		rootReplacements = { "../../..", root };
	}
	
	copyTemplateFiles.push_back({
		templatePath / "emptyExample.xcodeproj" / "project.pbxproj",
		xcodeProject / "project.pbxproj",
		{{ "emptyExample", projectName },
		rootReplacements }
	});
	
//	findandreplaceInTexfile(projectDir / (projectName + ".xcodeproj/project.pbxproj"), "../../..", root);


	copyTemplateFiles.push_back({
		templatePath / "Project.xcconfig",
		projectDir / "Project.xcconfig",
		{ rootReplacements }
	});
	
	if (target == "osx") {
		// TODO: TEST
		for (auto & f : { "openFrameworks-Info.plist", "of.entitlements" }) {
			copyTemplateFiles.push_back({ templatePath / f, projectDir / f });
		}
	} else {
		for (auto & f : { "ofxiOS-Info.plist", "ofxiOS_Prefix.pch" }) {
			copyTemplateFiles.push_back({ templatePath / f, projectDir / f });
		}

		fs::path from = templatePath / "mediaAssets";
		fs::path to = projectDir / "mediaAssets";
		if (!fs::exists(to)) {
			fs::copy(from, to, fs::copy_options::recursive);
		}
	}

	saveScheme();

	if(target == "osx"){
		saveMakefile();
	}
	
	for (auto & c : copyTemplateFiles) {
		c.run();
	}
	
	
//	if (!fs::equivalent(getOFRoot(), fs::path{"../../.."})) {
//		string root { ofPathToString(getOFRoot()) };
////		alert ("fs not equivalent to ../../.. root = " + root);
//		rootReplacements = { "../../..", root };
//		
////		findandreplaceInTexfile(projectDir / (projectName + ".xcodeproj/project.pbxproj"), "../../..", root);
////		findandreplaceInTexfile(projectDir / "Project.xcconfig", "../../..", root);
//		if( target == "osx" ){
//			findandreplaceInTexfile(projectDir / "Makefile", "../../..", root);
//			// MARK: not needed because baseProject::save() does the same
////			findandreplaceInTexfile(projectDir / "config.make", "../../..", root);
//		}
//	} else {
////		alert ("fs equivalent " + relRoot.string());
//	}
//	
	
	
	// NOW only files being copied
	
	fs::path projectDataDir { projectDir / "bin" / "data" };

	if (!fs::exists(projectDataDir)) {
		cout << "creating dataDir " << projectDataDir << endl;
		fs::create_directories(projectDataDir);
	}

	if (fs::exists(projectDataDir)) {
		// originally only on IOS
		//this is needed for 0.9.3 / 0.9.4 projects which have iOS media assets in bin/data/
		// TODO: Test on IOS
		fs::path templateDataDir { templatePath / "bin" / "data" };
		if (fs::exists(templateDataDir) && fs::is_directory(templateDataDir)) {
			baseProject::recursiveCopyContents(templateDataDir, projectDataDir);
		}
	}

	addCommand("# ---- PG VERSION " + getPGVersion());
	addCommand("Add :openFrameworksProjectGeneratorVersion string " + getPGVersion());

	fileProperties fp;
	addFile("App.xcconfig", "", fp);

	fp.absolute = true;
	addFile(fs::path{"bin"} / "data", "", fp);


	return true;
}

void xcodeProject::saveScheme(){
	auto schemeFolder = projectDir / ( projectName + ".xcodeproj" ) / "xcshareddata/xcschemes";
//	alert ("saveScheme " + schemeFolder.string());

	if (fs::exists(schemeFolder)) {
		fs::remove_all(schemeFolder);
	}
	fs::create_directories(schemeFolder);

	if (target == "osx") {
		for (auto & f : { "Release", "Debug" }) {
			copyTemplateFiles.push_back({
				templatePath / ("emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample " + string(f) + ".xcscheme"),
				schemeFolder / (projectName + " " +f+ ".xcscheme"),
				{{ "emptyExample", projectName }}
			});
		}

		copyTemplateFiles.push_back({
			projectDir / (projectName + ".xcodeproj/project.xcworkspace"),
			templatePath / "emptyExample.xcodeproj/project.xcworkspace"
		});
	} else {

		// MARK:- IOS sector;
		copyTemplateFiles.push_back({
			templatePath / "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample.xcscheme",
			schemeFolder / (projectName + ".xcscheme"),
			{{ "emptyExample", projectName }}
		});
	}
}

void xcodeProject::saveMakefile(){
//	alert ("saveMakefile " , 35);
	copyTemplateFiles.push_back({
		templatePath / "Makefile",
		projectDir / "Makefile",
		{ rootReplacements }
	});
	copyTemplateFiles.push_back({
		templatePath / "config.make",
		projectDir / "config.make"
	});


}

bool xcodeProject::loadProjectFile(){ //base
	renameProject();
	// MARK: just to return something.
	return true;
}

void xcodeProject::renameProject(){ //base
	// FIXME: review BUILT_PRODUCTS_DIR
	addCommand("Set :objects:"+buildConfigurationListUUID+":name " + projectName);

	// Just OSX here, debug app naming.
	if( target == "osx" ){
		// TODO: Hardcode to variable
		// FIXME: Debug needed in name?
		addCommand("Set :objects:E4B69B5B0A3A1756003C02F2:path " + projectName + "Debug.app");
	}
}

string xcodeProject::getFolderUUID(const fs::path & folder, bool isFolder, fs::path base) {
//	alert ("xcodeProject::getFolderUUID " + folder.string() + " : isfolder=" + ofToString(isFolder) + " : base=" + base.string());
//	alert ("xcodeProject::getFolderUUID " + folder.begin()->string() + " : base=" + base.string());
	/*
	TODO: Change key of folderUUID to base + folder, so "src" in additional source folders
	doesn't get confused with "src" from project.
	this can work but fullPath variable has to follow the same pattern
	fs::path keyFS = base / folder;
	 */

	string UUID { "" };

	// If folder UUID exists just return it.
	// in this case it is not found, so it creates UUID for the entire path
	if ( folderUUID.find(folder) == folderUUID.end() ) { // NOT FOUND
//		alert ("xcodeProject::getFolderUUID " + folder.string() + " : isfolder=" + ofToString(isFolder) + " : base=" + base.string());
		vector < string > folders = ofSplitString(ofPathToString(folder), "/", true);
		string lastFolderUUID = projRootUUID;

		if (folders.size()){
			for (std::size_t a=0; a<folders.size(); a++) {
				vector <string> joinFolders;
				joinFolders.assign(folders.begin(), folders.begin() + (a+1));
				string fullPath = ofJoinString(joinFolders, "/");

				// Query if path is already stored. if not execute this following block
				if ( folderUUID.find(fullPath) == folderUUID.end() ) {
					// cout << "creating" << endl;
					string thisUUID = generateUUID(fullPath);
					folderUUID[fullPath] = thisUUID;

					// here we add an UUID for the group (folder) and we initialize an array to receive children (files or folders inside)
					addCommand("");
					addCommand("Add :objects:"+thisUUID+":name string " + folders[a]);
					if (isFolder) {
						fs::path filePath;
						fs::path filePath_full { relRoot / fullPath };
						// FIXME: known issue: doesn't handle files with spaces in name.

						if (fs::exists(filePath_full)) {
							filePath = filePath_full;
						}
						if (fs::exists(fullPath)) {
							filePath = fullPath;
						}

						if (!filePath.empty()) {
							addCommand("Add :objects:"+thisUUID+":path string " + ofPathToString(filePath));
//							alert(commands.back(), 33);
						} else {
//							cout << ">>>>> filePath empty " << endl;
						}
					} else {
//						cout << "isFolder false" << endl;
					}

					addCommand("Add :objects:"+thisUUID+":isa string PBXGroup");
					addCommand("Add :objects:"+thisUUID+":children array");
					
					if (folder.begin()->string() == "addons") {
						addCommand("Add :objects:"+thisUUID+":sourceTree string <group>");
//						fs::path addonFolder = fs::relative(fullPath, "addons");
						fs::path addonFolder = fs::path(fullPath).filename();
						addCommand("Add :objects:"+thisUUID+":path string " + ofPathToString(addonFolder));
						// alert ("group " + folder.string() + " : " + base.string() + " : " + addonFolder.string(), 32);
					} else {
						addCommand("Add :objects:"+thisUUID+":sourceTree string SOURCE_ROOT");
					}

					// And this new object is cointained in parent hierarchy, or even projRootUUID
					addCommand("Add :objects:"+lastFolderUUID+":children: string " + thisUUID);

					// keep this UUID as parent for the next folder.
					lastFolderUUID = thisUUID;
				} else {
					lastFolderUUID = folderUUID[fullPath];
				}
			}
		}
		UUID = lastFolderUUID;
	} else {
		// Folder already exists, only return it.
		UUID = folderUUID[folder];
	}
	return UUID;
}

void xcodeProject::addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type){
//	alert ("addSrc " + ofPathToString(srcFile) + " : " + ofPathToString(folder), 31);
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
	
	if( type == DEFAULT ){
		if (ext == ".h" || ext == ".hpp"){
			fp.addToBuildPhase = false;
		}
		else if (ext == ".xib"){
			fp.addToBuildPhase	= false;
			fp.addToBuildResource = true;
			fp.addToResources = true;
		}
		else if (ext == ".metal"){
			fp.addToBuildResource = true;
			fp.addToResources = true;
		}
		else if(ext == ".entitlements"){
			fp.addToBuildResource = true;
			fp.addToResources = true;
		}
		else if(ext == ".info"){
			fp.addToBuildResource = true;
			fp.addToResources = true;
		}
		else if( target == "ios" ){
			fp.addToBuildPhase	= false;
			fp.addToResources = true;
		}
	} 


	string UUID {
		addFile(srcFile, folder, fp)
	};
}

// FIXME: name not needed anymore.
void xcodeProject::addFramework(const fs::path & path, const fs::path & folder){
	// alert( "xcodeProject::addFramework " + ofPathToString(path) + " : " + ofPathToString(folder) , 33);
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);

	addCommand("# ----- addFramework path=" + ofPathToString(path) + " folder=" + ofPathToString(folder));
	
	bool isSystemFramework = true;
	if (!folder.empty() && !ofIsStringInString(ofPathToString(path), "/System/Library/Frameworks")
		&& target != "ios"){
		isSystemFramework = false;
	}
	
	fileProperties fp;
	fp.codeSignOnCopy = !isSystemFramework;
	fp.copyFilesBuildPhase = !isSystemFramework;
	fp.frameworksBuildPhase = (target != "ios" && !folder.empty());

	string UUID {
		addFile(path, folder, fp)
	};

	addCommand("# ----- FRAMEWORK_SEARCH_PATHS");
	string parent { ofPathToString(path.parent_path()) };

	for (auto & c : buildConfigs) {
		addCommand("Add :objects:" + c + ":buildSettings:FRAMEWORK_SEARCH_PATHS: string " + parent);
	}
}


void xcodeProject::addXCFramework(const fs::path & path, const fs::path & folder) {
	//	alert( "xcodeProject::addFramework " + path.string() + " : " + folder.string() , 33);
	
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);
	
	addCommand("# ----- addXCFramework path=" + ofPathToString(path) + " folder=" + ofPathToString(folder));
	
	bool isSystemFramework = false;
	if (!folder.empty() && !ofIsStringInString(ofPathToString(path), "/System/Library/Frameworks")
		&& target != "ios"){
		isSystemFramework = true;
	}
	
	fileProperties fp;
//	fp.addToBuildPhase = true;
	fp.codeSignOnCopy = !isSystemFramework;
	fp.copyFilesBuildPhase = !isSystemFramework;
	fp.frameworksBuildPhase = (target != "ios" && !folder.empty());
	
	string UUID {
		addFile(path, folder, fp)
	};

	addCommand("# ----- XCFRAMEWORK_SEARCH_PATHS");
	string parent { ofPathToString(path.parent_path()) };

	for (auto & c : buildConfigs) {
		addCommand("Add :objects:" + c + ":buildSettings:FRAMEWORK_SEARCH_PATHS: string " + parent);
	}
}


void xcodeProject::addDylib(const fs::path & path, const fs::path & folder){
	//	alert( "xcodeProject::addDylib " + ofPathToString(path) , 33);

	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);

	fileProperties fp;
	fp.addToBuildPhase = true;
	fp.codeSignOnCopy = true;
	fp.copyFilesBuildPhase = true;
	
	addFile(path, folder, fp);
}


void xcodeProject::addInclude(string includeName){
	//alert("addInclude " + includeName);
	for (auto & c : buildConfigs) {
		addCommand("Add :objects:"+c+":buildSettings:HEADER_SEARCH_PATHS: string " + includeName);
	}
}

void xcodeProject::addLibrary(const LibraryBinary & lib){
//	alert( "xcodeProject::addLibrary " + lib.path , 33);
	for (auto & c : buildConfigs) {
		addCommand("Add :objects:"+c+":buildSettings:OTHER_LDFLAGS: string " + lib.path);
	}
}

void xcodeProject::addLDFLAG(string ldflag, LibType libType){
//	alert( "xcodeProject::addLDFLAG " + ldflag , 34);
	for (auto & c : buildConfigs) {
		addCommand("Add :objects:"+c+":buildSettings:OTHER_LDFLAGS: string " + ldflag);
	}
}

void xcodeProject::addCFLAG(string cflag, LibType libType){
	//alert("xcodeProject::addCFLAG " + cflag);
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		addCommand("Add :objects:"+c+":buildSettings:OTHER_CFLAGS: string " + cflag);
	}
}

void xcodeProject::addDefine(string define, LibType libType){
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		addCommand("Add :objects:"+c+":buildSettings:GCC_PREPROCESSOR_DEFINITIONS: string " + define);
	}
}

// FIXME: libtype is unused here
void xcodeProject::addCPPFLAG(string cppflag, LibType libType){
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		addCommand("Add :objects:"+c+":buildSettings:OTHER_CPLUSPLUSFLAGS: string " + cppflag);
	}
}

void xcodeProject::addAfterRule(string rule){
	// return;
//	cout << ">>>>>> addAfterRule " << rule << endl;
	addCommand("Add :objects:"+afterPhaseUUID+":buildActionMask string 2147483647");
	// addCommand("Add :objects:"+afterPhaseUUID+":files array");
	// addCommand("Add :objects:"+afterPhaseUUID+":inputPaths array");
	addCommand("Add :objects:"+afterPhaseUUID+":isa string PBXShellScriptBuildPhase");
	// addCommand("Add :objects:"+afterPhaseUUID+":outputPaths array");
	addCommand("Add :objects:"+afterPhaseUUID+":runOnlyForDeploymentPostprocessing string 0");
	addCommand("Add :objects:"+afterPhaseUUID+":shellPath string /bin/sh");
	addCommand("Add :objects:"+afterPhaseUUID+":showEnvVarsInLog string 0");

	// ofStringReplace(rule, "\"", "\\\"");
	// addCommand("Add :objects:"+afterPhaseUUID+":shellScript string \"" + rule + "\"");
	addCommand("Add :objects:"+afterPhaseUUID+":shellScript string " + rule);

	// adding this phase to build phases array
	// TODO: Check if nit needs another buildConfigurationListUUID for debug.
	addCommand("Add :objects:"+buildConfigurationListUUID+":buildPhases: string " + afterPhaseUUID);
}

void xcodeProject::addAddon(ofAddon & addon){
	//	alert("xcodeProject addAddon string :: " + addon.name, 31);

	// Files listed alphabetically on XCode navigator.
	std::sort(addon.srcFiles.begin(), addon.srcFiles.end(), std::less<string>());

	for (auto & e : addon.libs) {
		ofLogVerbose() << "adding addon libs: " << e.path;
		addLibrary(e);

		fs::path dylibPath { e.path };
		fs::path folder = dylibPath.parent_path().lexically_relative(addon.pathToOF);
//		cout << "dylibPath " << dylibPath << endl;
		if (dylibPath.extension() == ".dylib") {
			addDylib(dylibPath, folder);
		}
	}

	for (auto & e : addon.cflags) {
		ofLogVerbose() << "adding addon cflags: " << e;
		addCFLAG(e);
	}

	for (auto & e : addon.cppflags) {
		ofLogVerbose() << "adding addon cppflags: " << e;
		addCPPFLAG(e);
	}

	for (auto & e : addon.ldflags) {
		ofLogVerbose() << "adding addon ldflags: " << e;
//		alert("addon ldflags " + e, 31 );
		addLDFLAG(e);
	}

	for (auto & e : addon.srcFiles) {
		ofLogVerbose() << "adding addon srcFiles: " << e;
		addSrc(e,addon.filesToFolders[e]);
	}

	for (auto & e : addon.defines) {
		ofLogVerbose() << "adding addon defines: " << e;
		addDefine(e);
	}

	for (auto & f : addon.frameworks) {
		ofLogVerbose() << "adding addon frameworks: " << f;

		size_t found=f.find('/');
		if (found==string::npos){
			fs::path folder = fs::path{ "addons" } / addon.name / "frameworks";
//			fs::path folder = addon.filesToFolders[f];

			if (target == "ios"){
				addFramework(  "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/System/Library/Frameworks/" +
					f + ".framework",
					folder);
			} else {
				if (addon.isLocalAddon) {
					folder = addon.addonPath / "frameworks";
				}
				addFramework( "/System/Library/Frameworks/" + f + ".framework", folder);
			}
		} else {
			if (ofIsStringInString(f, "/System/Library")){
				addFramework(f, "addons/" + addon.name + "/frameworks");

			} else {
				addFramework(f, addon.filesToFolders[f]);
			}
		}
	}


	for (auto & f : addon.xcframeworks) {
		//		alert ("xcodeproj addon.xcframeworks : " + f);
		ofLogVerbose() << "adding addon xcframeworks: " << f;

		size_t found = f.find('/');
		if (found == string::npos) {
			fs::path folder = fs::path { "addons" } / addon.name / "xcframeworks";
			//			fs::path folder = addon.filesToFolders[f];

			if (addon.isLocalAddon) {
				folder = addon.addonPath / "xcframeworks";
			}
			// MARK: Is this ok to call .framework?
			addXCFramework("/System/Library/Frameworks/" + f + ".framework", folder);
			
		} else {
			if (ofIsStringInString(f, "/System/Library")) {
				addFramework(f, "addons/" + addon.name + "/frameworks");

			} else {
				addFramework(f, addon.filesToFolders[f]);
			}
		}
	}
}

bool xcodeProject::saveProjectFile(){
	fs::path fileName = projectDir / (projectName + ".xcodeproj/project.pbxproj");
//	alert("xcodeProject::saveProjectFile() begin " + fileName.string());
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
		json j = json::parse(contents);
		contents.close();

		for (auto & c : commands) {
			// readable comments enabled now.
			if (c != "" && c[0] != '#') {
				vector<string> cols = ofSplitString(c, " ");
				string thispath = cols[1];
				ofStringReplace(thispath, ":", "/");

				if (thispath.substr(thispath.length() -1) != "/") {
					//if (cols[0] == "Set") {
					json::json_pointer p = json::json_pointer(thispath);
					if (cols[2] == "string") {
						// find position after find word
						auto stringStart = c.find("string ") + 7;
						j[p] = c.substr(stringStart);
						// j[p] = cols[3];
					}
					else if (cols[2] == "array") {
						j[p] = {};
					}
				}
				else {
					thispath = thispath.substr(0, thispath.length() -1);
//					cout << thispath << endl;
					json::json_pointer p = json::json_pointer(thispath);
					try {
						// Fixing XCode one item array issue
						if (!j[p].is_array()) {
							auto v = j[p];
							j[p] = json::array();
							if (!v.is_null()) {
								j[p].emplace_back(v);
							}
						}
						j[p].emplace_back(cols[3]);

					} catch (std::exception & e) {
						cout << "json error " << endl;
						cout << e.what() << endl;
					}
				}
			}
		}

		std::ofstream jsonFile(fileName);
		try{
			jsonFile << j.dump(1, '	');
		}catch(std::exception & e){
			ofLogError("xcodeProject::saveProjectFile") << "Error saving json to " << fileName << ": " << e.what();
			return false;
		}catch(...){
			ofLogError("xcodeProject::saveProjectFile") << "Error saving json to " << fileName;
			return false;
		}
	}

//	for (auto & c : commands) cout << c << endl;

	return true;
}


void xcodeProject::addCommand(const string & command) {
	if (debugCommands) {
		alert(command);
	}
	commands.emplace_back(command);
}

string xcodeProject::addFile(const fs::path & path, const fs::path & folder, const fileProperties & fp) {
//	alert("addFile " + ofPathToString(path) + " : " + ofPathToString(folder) , 31);
//	alert("reference " + ofToString(fp.reference));
//	alert("addToBuildPhase " + ofToString(fp.addToBuildPhase));
//	alert("codeSignOnCopy " + ofToString(fp.codeSignOnCopy));
//	alert("copyFilesBuildPhase " + ofToString(fp.copyFilesBuildPhase));

	string UUID = "";
	std::map<fs::path, string> extensionToFileType {
		{ ".framework" , "wrapper.framework" },
		{ ".dylib" , "compiled.mach-o.dylib" },
		
		{ ".cpp" , "sourcecode.cpp.cpp" },
		{ ".c" , "sourcecode.cpp.c" },
		{ ".h" , "sourcecode.cpp.h" },
		{ ".hpp" , "sourcecode.cpp.h" },
		{ ".mm" , "sourcecode.cpp.objcpp" },
		{ ".m" , "sourcecode.cpp.objcpp" },
		
		{ ".xib" , "file.xib" },
		{ ".metal" , "file.metal" },
		{ ".entitlements" , "text.plist.entitlements" },
		{ ".info" , "text.plist.xml" },
		{ ".xcconfig" , "text.xcconfig" },
	};
	
//	cout << "will check if exists " << (projectDir / path) << endl;
	if (fs::exists( projectDir / path )) {
//		cout << "OK exists" << endl;
		bool isFolder = false;
		string fileType = "file";
		fileType = extensionToFileType[path.extension()];
		if (fileType == "") {
			if (fs::is_directory(path)) {
				fileType = "folder";
				isFolder = true;
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
		addCommand("Add :objects:"+UUID+":fileEncoding string 4");
		addCommand("Add :objects:"+UUID+":isa string PBXFileReference");
		addCommand("Add :objects:"+UUID+":lastKnownFileType string " + fileType);
		addCommand("Add :objects:"+UUID+":name string " + ofPathToString(path.filename()));
		if (fp.absolute) {
			addCommand("Add :objects:"+UUID+":sourceTree string SOURCE_ROOT");
			addCommand("Add :objects:"+UUID+":path string " + ofPathToString(path));
		} else {
			addCommand("Add :objects:"+UUID+":sourceTree string <group>");
		}

		string folderUUID;
		if (fp.isSrc) {
//			alert("fp isSrc!", 31);
//			addCommand("Add :objects:"+UUID+":name string " + ofPathToString(path.filename()));
		
//			fs::path base;
//			fs::path src { path };
//			fs::path folderFS { folder };
//
//			if (!fs::exists(folderFS)) {
//				// cout << "folder doesn't exist " << folderFS << endl;
//				fs::path parent = src.parent_path();
//				auto nit = folderFS.end();
//
//				base = parent;
//				fs::path folderFS2 = folderFS;
//
//				while(base.filename() == folderFS2.filename() && base.filename() != "" && folderFS2.filename() != "") {
//					base = base.parent_path();
//					folderFS2 = folderFS2.parent_path();
//				}
//			}
//
//			folderUUID = getFolderUUID(folder, true, base);
//			alert("isSrc " + ofPathToString(folder) + " : " + ofPathToString(base), 33);
		} else {
			folderUUID = getFolderUUID(folder, isFolder);
		}
//		isFolder = true;
		folderUUID = getFolderUUID(folder, isFolder);
		addCommand("# ---- addFileToFolder UUID " + ofPathToString(folder));
		addCommand("Add :objects:" + folderUUID + ":children: string " + UUID);
		
		
		string buildUUID { generateUUID(ofPathToString(path) + "-build") };
		// If any other option is true, add buildUUID entries.
		if (
				fp.addToBuildPhase ||
				fp.codeSignOnCopy ||
				fp.copyFilesBuildPhase ||
				fp.addToBuildResource ||
				fp.addToResources 
				//|| fp.frameworksBuildPhase ~ I've just removed this one, favoring -InFrameworks
			) {
			addCommand("# ---- addToBuildPhase " + buildUUID);
			addCommand("Add :objects:"+buildUUID+":isa string PBXBuildFile");
			addCommand("Add :objects:"+buildUUID+":fileRef string "+UUID);
		}
		
		if (fp.addToBuildPhase) { // Compile Sources
			// Not sure if it applies to everything, applies to srcFile.
			addCommand("# ---- addToBuildPhase");
			addCommand("Add :objects:"+buildActionMaskUUID+":files: string " + buildUUID);
		}
		
		if (fp.copyFilesBuildPhase) {
			if (path.extension() == ".framework") {
				// copy to frameworks
				addCommand("# ---- copyPhase Frameworks " + buildUUID);
				addCommand("Add :objects:E4C2427710CC5ABF004149E2:files: string " + buildUUID);
			} else {
				// copy to executables
				addCommand("# ---- copyPhase Executables " + buildUUID);
				addCommand("Add :objects:E4A5B60F29BAAAE400C2D356:files: string " + buildUUID);
			}
		}
		
		if (fp.codeSignOnCopy) {
			addCommand("# ---- codeSignOnCopy " + buildUUID);
			addCommand("Add :objects:"+buildUUID+":settings:ATTRIBUTES array");
			addCommand("Add :objects:"+buildUUID+":settings:ATTRIBUTES: string CodeSignOnCopy");
		}
		
		if (fp.addToBuildResource) {
			string mediaAssetsUUID { "9936F60E1BFA4DEE00891288" };
			addCommand("# ---- addToBuildResource");
			addCommand("Add :objects:"+mediaAssetsUUID+":files: string " + UUID);
		}
		
		if (fp.addToResources) {
			// FIXME: test if it is working on iOS
			if (resourcesUUID != "") {
				addCommand("# ---- addToResources (IOS only) ?" + buildUUID);
				addCommand("Add :objects:"+resourcesUUID+": string " + buildUUID);
			}
		}
		
		if (fp.frameworksBuildPhase) { // Link Binary With Libraries
			auto tempUUID = generateUUID(ofPathToString(path) + "-InFrameworks");
			addCommand("Add :objects:" + tempUUID + ":fileRef string " + UUID);
			addCommand("Add :objects:" + tempUUID + ":isa string PBXBuildFile");

			addCommand("# --- PBXFrameworksBuildPhase");
			addCommand("Add :objects:E4B69B590A3A1756003C02F2:files: string " + tempUUID);
		}
		
		if (path.extension() == ".framework") {
			addCommand("# ---- Frameworks Folder " + UUID);
			addCommand("Add :objects:901808C02053638E004A7774:children: string " + UUID);

			addCommand("# ---- PBXFrameworksBuildPhase " + buildUUID);
			addCommand("Add :objects:1D60588F0D05DD3D006BFB54:files: string " + buildUUID);
		}
		debugCommands = false;
	}
	return UUID;
}
