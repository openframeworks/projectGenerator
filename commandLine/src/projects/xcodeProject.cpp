#include "xcodeProject.h"
#include "Utils.h"
#include "json.hpp"
#include <iostream>

using nlohmann::json;
using nlohmann::json_pointer;

xcodeProject::xcodeProject(const string & target) : baseProject(target){
	// TODO: remove unused variables
	if( target == "osx" ){
		folderUUID = {
			{ "src", 			"E4B69E1C0A3A1BDC003C02F2" },
			{ "addons", 		"BB4B014C10F69532006C3DED" },
			{ "openFrameworks", 		"191EF70929D778A400F35F26" },
			// { "localAddons",	"6948EE371B920CB800B5AC1A" },
			{ "", 				"E4B69B4A0A3A1720003C02F2" }
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
			// { "localAddons", 	"6948EE371B920CB800B5AC1A" },
			{ "", 				"29B97314FDCFA39411CA2CEA" }
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

	fs::path fileFrom = templatePath / "emptyExample.xcodeproj" / "project.pbxproj";
	fs::path fileTo = xcodeProject / "project.pbxproj";
	try {
		fs::copy_file(fileFrom, fileTo, fs::copy_options::overwrite_existing);
	} catch(fs::filesystem_error& e) {
		std::cout << "Could not copy " << fileFrom << " > " << fileTo << " :: " << e.what() << std::endl;
	}
	findandreplaceInTexfile(fileTo, "emptyExample", projectName);


	fileFrom = templatePath / "Project.xcconfig";
	fileTo = projectDir / "Project.xcconfig";
	try {
		fs::copy_file(fileFrom, fileTo, fs::copy_options::overwrite_existing);
	} catch(fs::filesystem_error& e) {
		std::cout << "Could not copy " << fileFrom << " > " << fileTo << " :: "  << e.what() << std::endl;
	}

	fs::path binDirectory { projectDir / "bin" };
	fs::path dataDir { binDirectory / "data" };

	if (!fs::exists(binDirectory)) {
		cout << "creating dataDir " << dataDir << endl;
		fs::create_directories(dataDir);
	}

	if (fs::exists(binDirectory)) {
		// originally only on IOS
		//this is needed for 0.9.3 / 0.9.4 projects which have iOS media assets in bin/data/
		// TODO: Test on IOS
		fs::path srcDataDir { templatePath / "bin" / "data" };
		if (fs::exists(srcDataDir) && fs::is_directory(srcDataDir)) {
			baseProject::recursiveCopyContents(srcDataDir, dataDir);
		}
	}

	if( target == "osx" ){
		// TODO: TEST
		for (auto & f : { "openFrameworks-Info.plist", "of.entitlements" }) {
			fs::copy(templatePath / f, projectDir / f, fs::copy_options::overwrite_existing);
		}
	}else{
		for (auto & f : { "ofxiOS-Info.plist", "ofxiOS_Prefix.pch" }) {
			fs::copy(templatePath / f, projectDir / f, fs::copy_options::overwrite_existing);
		}

		fs::path from = templatePath / "mediaAssets";
		fs::path to = projectDir / "mediaAssets";
		if (!fs::exists(to)) {
			fs::copy(from, to, fs::copy_options::recursive);
		}
	}

	saveScheme();

	if(target=="osx"){
		saveMakefile();
	}

	// Calculate OF Root in relation to each project (recursively);
//	fs::path relRoot { getOFRoot() };

//	// FIXME: maybe not needed anymore
//	if (ofIsPathInPath(projectDir, getOFRoot())) {
//		setOFRoot(fs::relative(getOFRoot(), projectDir));
////		relRoot = fs::relative(getOFRoot(), projectDir);
////		alert ("relRoot = " + relRoot.string());
//	} else {
////		alert ("ofIsPathInPath not");
//	}


	commands.emplace_back("# ---- PG VERSION " + getPGVersion());
	commands.emplace_back("Add :openFrameworksProjectGeneratorVersion string " + getPGVersion());

	fileProperties fp;
	addFile("App.xcconfig", "", fp);
	addFile(fs::path{"bin"} / "data", "", fp);


	if (!fs::equivalent(getOFRoot(), fs::path{"../../.."})) {
		string root = getOFRoot().string();
//		alert ("fs not equivalent to ../../.. root = " + root);
		findandreplaceInTexfile(projectDir / (projectName + ".xcodeproj/project.pbxproj"), "../../..", root);
		findandreplaceInTexfile(projectDir / "Project.xcconfig", "../../..", root);
		if( target == "osx" ){
			findandreplaceInTexfile(projectDir / "Makefile", "../../..", root);
			// MARK: not needed because baseProject::save() does the same
//			findandreplaceInTexfile(projectDir / "config.make", "../../..", root);
		}
	} else {
//		alert ("fs equivalent " + relRoot.string());
	}
	return true;
}

void xcodeProject::saveScheme(){
	auto schemeFolder = projectDir / ( projectName + ".xcodeproj" ) / "xcshareddata/xcschemes";
//	alert ("saveScheme " + schemeFolder.string());

	if (fs::exists(schemeFolder)) {
		fs::remove_all(schemeFolder);
	}
	fs::create_directories(schemeFolder);

	if(target=="osx"){
		for (auto & f : { string("Release"), string("Debug") }) {
			auto fileFrom = templatePath / ("emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample " + f + ".xcscheme");
			auto fileTo = schemeFolder / (projectName + " " +f+ ".xcscheme");
			fs::copy(fileFrom, fileTo);
			findandreplaceInTexfile(fileTo, "emptyExample", projectName);
		}

		auto fileTo = projectDir / (projectName + ".xcodeproj/project.xcworkspace");
		auto fileFrom = templatePath / "emptyExample.xcodeproj/project.xcworkspace";
		fs::copy(fileFrom, fileTo);
	}else{

		// MARK:- IOS sector;
		auto fileFrom = templatePath / "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample.xcscheme";
		auto fileTo = schemeFolder / (projectName + ".xcscheme");
		fs::copy(fileFrom, fileTo);
		findandreplaceInTexfile(fileTo, "emptyExample", projectName);
	}
}

void xcodeProject::saveMakefile(){
//	alert ("saveMakefile " , 35);
	for (auto & f : { "Makefile", "config.make" }) {
		fs::path fileFrom = templatePath / f;
		fs::path fileTo = projectDir / f;
		// Always overwrite for now, so we can have the original file from template before substituting anything
		// if (!fs::exists(fileTo))
		{
			fs::copy(fileFrom, fileTo, fs::copy_options::overwrite_existing);
		}
	}
}

bool xcodeProject::loadProjectFile(){ //base
	renameProject();
	// MARK: just to return something.
	return true;
}

void xcodeProject::renameProject(){ //base
	// FIXME: review BUILT_PRODUCTS_DIR
	commands.emplace_back("Set :objects:"+buildConfigurationListUUID+":name " + projectName);

	// Just OSX here, debug app naming.
	if( target == "osx" ){
		// TODO: Hardcode to variable
		// FIXME: Debug needed in name?
		commands.emplace_back("Set :objects:E4B69B5B0A3A1756003C02F2:path " + projectName + "Debug.app");
	}
}

string xcodeProject::getFolderUUID(const fs::path & folder, bool isFolder, fs::path base) {
//	alert ("xcodeProject::getFolderUUID " + folder.string() + " : base=" + base.string());
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
		vector < string > folders = ofSplitString(folder.string(), "/", true);
		string lastFolderUUID = projRootUUID;

		if (folders.size()){
			for (int a=0; a<folders.size(); a++) {
				vector <string> joinFolders;
				joinFolders.assign(folders.begin(), folders.begin() + (a+1));
				string fullPath = ofJoinString(joinFolders, "/");

				// Query if path is already stored. if not execute this following block
				if ( folderUUID.find(fullPath) == folderUUID.end() ) {
					// cout << "creating" << endl;
					string thisUUID = generateUUID(fullPath);
					folderUUID[fullPath] = thisUUID;

					// here we add an UUID for the group (folder) and we initialize an array to receive children (files or folders inside)
					commands.emplace_back("");
					commands.emplace_back("Add :objects:"+thisUUID+":name string " + folders[a]);
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
							commands.emplace_back("Add :objects:"+thisUUID+":path string " + filePath.string());
						} else {
//							cout << ">>>>> filePath empty " << endl;
						}
					} else {
//						cout << "isFolder false" << endl;
					}

					commands.emplace_back("Add :objects:"+thisUUID+":isa string PBXGroup");
					commands.emplace_back("Add :objects:"+thisUUID+":children array");
//					commands.emplace_back("Add :objects:"+thisUUID+":sourceTree string <group>");
					commands.emplace_back("Add :objects:"+thisUUID+":sourceTree string SOURCE_ROOT");

					// And this new object is cointained in parent hierarchy, or even projRootUUID
					commands.emplace_back("Add :objects:"+lastFolderUUID+":children: string " + thisUUID);

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
//	cout << "xcodeProject::addSrc " << srcFile << " : " << folder << endl;
	string ext = ofPathToString(srcFile.extension());

	bool addToResources = false;
	
	fileProperties fp {
		.reference = true,
		.addToBuildPhase = true,
		.codeSignOnCopy = false,
		.copyFilesBuildPhase = false,
		.addToBuildResource = false,
		.addToResources = false,
	};
	
	if( type == DEFAULT ){
//		if ( ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c" ) {
//			addToResources = false;
//		}
		if (ext == ".h" || ext == ".hpp"){
			fp.addToBuildPhase = false;
//			addToResources = false;
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
void xcodeProject::addFramework(const string & name, const fs::path & path, const fs::path & folder){
	// alert( "xcodeProject::addFramework " + name + " : " + path.string() + " : " + folder.string() , 33);
	// name = name of the framework
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);

	addCommand("# ----- addFramework name=" + name + " path=" + ofPathToString(path) + " folder=" + ofPathToString(folder));
	
	bool isSystemFramework = true;
	if (!folder.empty() && !ofIsStringInString(path.string(), "/System/Library/Frameworks")
		&& target != "ios"){
		isSystemFramework = false;
	}
	
	string UUID {
		addFile(path, folder,
			{
				.reference = true,
				.addToBuildPhase = true,
				.codeSignOnCopy = !isSystemFramework,
				.copyFilesBuildPhase = !isSystemFramework,
				.frameworksBuildPhase = (target != "ios" && !folder.empty())
			}
		)
	};

	commands.emplace_back("# ----- FRAMEWORK_SEARCH_PATHS");

	string parent { ofPathToString(path.parent_path()) };

	for (auto & c : buildConfigs) {
		commands.emplace_back("Add :objects:" + c + ":buildSettings:FRAMEWORK_SEARCH_PATHS: string " + parent);
	}
}


void xcodeProject::addXCFramework(const string & name, const fs::path & path, const fs::path & folder) {
	//	alert( "xcodeProject::addFramework " + name + " : " + path.string() + " : " + folder.string() , 33);
	
	// name = name of the framework
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);
	
	addCommand("# ----- addXCFramework name=" + name + " path=" + ofPathToString(path) + " folder=" + ofPathToString(folder));
	
	
	bool isSystemFramework = false;
	if (!folder.empty() && !ofIsStringInString(path.string(), "/System/Library/Frameworks")
		&& target != "ios"){
		isSystemFramework = true;
	}
	
	string UUID {
		addFile(path, folder, {
			.reference = true,
			.addToBuildPhase = true,
			.codeSignOnCopy = !isSystemFramework,
			.copyFilesBuildPhase = !isSystemFramework,
			.frameworksBuildPhase = !folder.empty()
		})
	};

	commands.emplace_back("# ----- XCFRAMEWORK_SEARCH_PATHS");

	string parent { ofPathToString(path.parent_path()) };

	for (auto & c : buildConfigs) {
		commands.emplace_back("Add :objects:" + c + ":buildSettings:FRAMEWORK_SEARCH_PATHS: string " + parent);
	}

//	if (!folder.empty()) {
//		// add it to the linking phases...
//		// PBXFrameworksBuildPhase
//		// https://www.rubydoc.info/gems/xcodeproj/Xcodeproj/Project/Object/PBXFrameworksBuildPhase
//		// The phase responsible on linking with frameworks. Known as ‘Link Binary With Libraries` in the UI.
//
//		// This is what was missing. a reference in root objects to the framework, so we can add the reference to PBXFrameworksBuildPhase
//		auto tempUUID = generateUUID(name + "-InFrameworks");
//		commands.emplace_back("Add :objects:" + tempUUID + ":fileRef string " + UUID);
//		commands.emplace_back("Add :objects:" + tempUUID + ":isa string PBXBuildFile");
//
//		commands.emplace_back("# --- PBXFrameworksBuildPhase");
//		commands.emplace_back("Add :objects:E4B69B590A3A1756003C02F2:files: string " + tempUUID);
//	}

}


void xcodeProject::addDylib(const string & name, const fs::path & path, const fs::path & folder){
//	alert( "xcodeProject::addDylib " + name + " : " + path.string() , 33);

	// name = name of the dylib
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);

	string UUID {
		addFile(path, folder, {
			.reference = true,
			.addToBuildPhase = true,
			.codeSignOnCopy = true,
			.copyFilesBuildPhase = true
		})
	};
}


void xcodeProject::addInclude(string includeName){
	//alert("addInclude " + includeName);
	for (auto & c : buildConfigs) {
		string s = "Add :objects:"+c+":buildSettings:HEADER_SEARCH_PATHS: string " + includeName;
		commands.emplace_back("Add :objects:"+c+":buildSettings:HEADER_SEARCH_PATHS: string " + includeName);
	}
}

void xcodeProject::addLibrary(const LibraryBinary & lib){
//	alert( "xcodeProject::addLibrary " + lib.path , 33);
	//	alert("addLibrary " + lib.path , 35);
	for (auto & c : buildConfigs) {
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_LDFLAGS: string " + lib.path);
	}
}

void xcodeProject::addLDFLAG(string ldflag, LibType libType){
//	alert( "xcodeProject::addLDFLAG " + ldflag , 34);
	for (auto & c : buildConfigs) {
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_LDFLAGS: string " + ldflag);
	}
}

void xcodeProject::addCFLAG(string cflag, LibType libType){
	//alert("xcodeProject::addCFLAG " + cflag);
	//commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_CFLAGS array");
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_CFLAGS: string " + cflag);
	}
}

void xcodeProject::addDefine(string define, LibType libType){
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		commands.emplace_back("Add :objects:"+c+":buildSettings:GCC_PREPROCESSOR_DEFINITIONS: string " + define);
	}
}

// FIXME: libtype is unused here
void xcodeProject::addCPPFLAG(string cppflag, LibType libType){
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_CPLUSPLUSFLAGS: string " + cppflag);
	}
}

void xcodeProject::addAfterRule(string rule){
	// return;
//	cout << ">>>>>> addAfterRule " << rule << endl;
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":buildActionMask string 2147483647");
	// commands.emplace_back("Add :objects:"+afterPhaseUUID+":files array");
	// commands.emplace_back("Add :objects:"+afterPhaseUUID+":inputPaths array");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":isa string PBXShellScriptBuildPhase");
	// commands.emplace_back("Add :objects:"+afterPhaseUUID+":outputPaths array");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":runOnlyForDeploymentPostprocessing string 0");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":shellPath string /bin/sh");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":showEnvVarsInLog string 0");

	// ofStringReplace(rule, "\"", "\\\"");
	// commands.emplace_back("Add :objects:"+afterPhaseUUID+":shellScript string \"" + rule + "\"");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":shellScript string " + rule);


	// adding this phase to build phases array
	// TODO: Check if nit needs another buildConfigurationListUUID for debug.
	commands.emplace_back("Add :objects:"+buildConfigurationListUUID+":buildPhases: string " + afterPhaseUUID);
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
			addDylib(dylibPath.filename().string(), dylibPath, folder);
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
//		alert ("xcodeproj addon.frameworks : " + f);
		ofLogVerbose() << "adding addon frameworks: " << f;

		size_t found=f.find('/');
		if (found==string::npos){
			fs::path folder = fs::path{ "addons" } / addon.name / "frameworks";
//			fs::path folder = addon.filesToFolders[f];

			if (target == "ios"){
				addFramework( f + ".framework", "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/System/Library/Frameworks/" +
					f + ".framework",
					folder);
			} else {
				if (addon.isLocalAddon) {
					folder = addon.addonPath / "frameworks";
				}
				addFramework( f + ".framework",
					"/System/Library/Frameworks/" + f + ".framework",
					folder);
			}
		} else {
			if (ofIsStringInString(f, "/System/Library")){
				vector < string > pathSplit = ofSplitString(f, "/");
				addFramework(pathSplit[pathSplit.size()-1],
							 f,
							 "addons/" + addon.name + "/frameworks");

			} else {
				vector < string > pathSplit = ofSplitString(f, "/");
				addFramework(pathSplit[pathSplit.size()-1], f, addon.filesToFolders[f]);
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
			addXCFramework(f + ".framework",
				"/System/Library/Frameworks/" + f + ".framework",
				folder);
			
		} else {
			if (ofIsStringInString(f, "/System/Library")) {
				vector<string> pathSplit = ofSplitString(f, "/");
				addFramework(pathSplit[pathSplit.size() - 1],
					f,
					"addons/" + addon.name + "/frameworks");

			} else {
				vector<string> pathSplit = ofSplitString(f, "/");
				addFramework(pathSplit[pathSplit.size() - 1], f, addon.filesToFolders[f]);
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
		string command = "/usr/libexec/PlistBuddy " + fileName.string();
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

					} catch (std::exception e) {
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
//	alert("addFile " + ofToString(path), 31);
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
	if (fs::exists( projectDir / path )) {
//		alert("exists !", 35);
		string fileType = "file";
		fileType = extensionToFileType[path.extension()];
		if (fileType == "") {
			if (fs::is_directory(path)) {
				fileType = "folder";
			}
		}

		UUID = generateUUID(path);
		string name = ofPathToString(path.filename());
		
//		debugCommands = true;
		addCommand("");
		addCommand("# -- addFile " + name);
//		commands.emplace_back("Add :objects:"+UUID+":fileEncoding string 4");
		
		//-----------------------------------------------------------------
		// based on the extension make some choices about what to do:
		//-----------------------------------------------------------------

//-----------------------------------------------------------------
// (A) make a FILE REF
//-----------------------------------------------------------------
		// encoding may be messing up for frameworks... so I switched to a pbx file ref without encoding fields
		
//		if (fp.reference) {
//		} else {
//			addCommand("Add :objects:"+UUID+":isa string PBXGroup");
//		}
		addCommand("Add :objects:"+UUID+":fileEncoding string 4");
		addCommand("Add :objects:"+UUID+":isa string PBXFileReference");
//		addCommand("Add :objects:"+UUID+":sourceTree string <group>");
		addCommand("Add :objects:"+UUID+":sourceTree string SOURCE_ROOT");
		addCommand("Add :objects:"+UUID+":lastKnownFileType string " + fileType);
		addCommand("Add :objects:"+UUID+":path string " + ofPathToString(path));
		addCommand("Add :objects:"+UUID+":name string " + name);
		
		string folderUUID = getFolderUUID(folder, false);
		addCommand("# ---- addFileToFolder UUID " + ofPathToString(folder));
		addCommand("Add :objects:" + folderUUID + ":children: string " + UUID);
		addCommand("#");
		
		
		string buildUUID { generateUUID(name + "-build") };
		// If any other option is true, add buildUUID entries.
		if (fp.addToBuildPhase ||
			fp.codeSignOnCopy ||
			fp.copyFilesBuildPhase ||
			fp.addToBuildResource ||
			fp.addToResources ||
			fp.frameworksBuildPhase
			) {
			addCommand("# ---- addToBuildPhase " + buildUUID);
			addCommand("Add :objects:"+buildUUID+":isa string PBXBuildFile");
			addCommand("Add :objects:"+buildUUID+":fileRef string "+UUID);
		}
		
		if (fp.addToBuildPhase) {
			// Not sure if it applies to everything, applies to srcFile.
			addCommand("# ---- addToBuildPhase");
			addCommand("Add :objects:"+buildActionMaskUUID+":files: string " + buildUUID);
		}
		
		if (fp.codeSignOnCopy) {
			addCommand("# ---- codeSignOnCopy " + buildUUID);
			addCommand("Add :objects:"+buildUUID+":settings:ATTRIBUTES array");
			addCommand("Add :objects:"+buildUUID+":settings:ATTRIBUTES: string CodeSignOnCopy");
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
		
		if (fp.addToBuildResource) {
			string mediaAssetsUUID = "9936F60E1BFA4DEE00891288";
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
		
		if (fp.frameworksBuildPhase) {
			addCommand("# ---- frameworksBuildPhase " + buildUUID);
			addCommand("Add :objects:E4B69B590A3A1756003C02F2:files: string " + buildUUID);

		}

		debugCommands = false;

	}
	return UUID;
}
