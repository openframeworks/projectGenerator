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
//	alert ("createProjectFile " + xcodeProject.string(), 35);

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

	if (fs::exists( projectDir / "App.xcconfig" )) {
		string UUID { generateUUID( string("App.xcconfig") ) };
		commands.emplace_back("# ---- App.xcconfig");
		commands.emplace_back("Add :objects:"+UUID+":fileEncoding string 4");
		commands.emplace_back("Add :objects:"+UUID+":isa string PBXFileReference");
		commands.emplace_back("Add :objects:"+UUID+":lastKnownFileType string text.xcconfig");
		commands.emplace_back("Add :objects:"+UUID+":path string App.xcconfig");
		commands.emplace_back("Add :objects:"+UUID+":sourceTree string <group>");
		commands.emplace_back("Add :objects:" + folderUUID[""] + ":children: string " + UUID);
		commands.emplace_back("#");
	}


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
	string buildUUID { "" };
	string ext = srcFile.extension().string();

	bool addToResources = true;
	bool addToBuild = true;
	bool addToBuildResource = false;
	string fileKind = "file";

	if( type == DEFAULT ){
		if ( ext == ".cpp" || ext == ".cc" || ext ==".cxx" ) {
			fileKind = "sourcecode.cpp.cpp";
			addToResources = false;
		}
		else if ( ext == ".c" ) {
			fileKind = "sourcecode.c.c";
			addToResources = false;
		}
		else if (ext == ".h" || ext == ".hpp") {
			fileKind = "sourcecode.c.h";
			addToBuild = false;
			addToResources = false;
		}
		else if (ext == ".mm" || ext == ".m") {
			addToResources = false;
			fileKind = "sourcecode.cpp.objcpp";
		}
		else if (ext == ".xib") {
			fileKind = "file.xib";
			addToBuild	= false;
			addToBuildResource = true;
			addToResources = true;
		}
		else if (ext == ".metal") {
			fileKind = "file.metal";
			addToBuild    = true;
			addToBuildResource = true;
			addToResources = true;
		}
		else if(ext == ".entitlements"){
			fileKind = "text.plist.entitlements";
			addToBuild    = true;
			addToBuildResource = true;
			addToResources = true;
		}
		else if(ext == ".info"){
			fileKind = "text.plist.xml";
			addToBuild    = true;
			addToBuildResource = true;
			addToResources = true;
		}
		else if( target == "ios" ){
			fileKind = "file";
			addToBuild	= false;
			addToResources = true;
		}
	} else {
		switch(type){
		case CPP:
			fileKind = "sourcecode.cpp.cpp";
			addToResources = false;
			break;
		case C:
			fileKind = "sourcecode.c.c";
			addToResources = false;
			break;
		case HEADER:
			fileKind = "sourcecode.c.h";
			addToBuild = false;
			addToResources = false;
			break;
		case OBJC:
			addToResources = false;
			fileKind = "sourcecode.cpp.objcpp";
			break;
		default:
			ofLogError() << "explicit source type " << type << " not supported yet on osx for " << srcFile;
			break;
		}
	}

	//-----------------------------------------------------------------
	// (A) make a FILE REF
	//-----------------------------------------------------------------

	string UUID { generateUUID(srcFile) };   // replace with theo's smarter system.

	fs::path srcFS { srcFile };
	string name { srcFS.filename().string() };
	string path { srcFS.parent_path().string() };

	commands.emplace_back("# ---- ADDSRC");
	commands.emplace_back("Add :objects:"+UUID+":name string "+name);
	commands.emplace_back("Add :objects:"+UUID+":path string "+srcFile.string());
	commands.emplace_back("Add :objects:"+UUID+":isa string PBXFileReference");
	if(ext == ".xib"){
		commands.emplace_back("Add :objects:"+UUID+":lastKnownFileType string "+fileKind);
	} else {
		commands.emplace_back("Add :objects:"+UUID+":explicitFileType string "+fileKind);
	}
	commands.emplace_back("Add :objects:"+UUID+":sourceTree string SOURCE_ROOT");
	commands.emplace_back("Add :objects:"+UUID+":fileEncoding string 4");

	//-----------------------------------------------------------------
	// (B) BUILD REF
	//-----------------------------------------------------------------
	if (addToBuild || addToBuildResource ){
		buildUUID = generateUUID(srcFile.string() + "-build");
		commands.emplace_back("Add :objects:"+buildUUID+":fileRef string "+UUID);
		commands.emplace_back("Add :objects:"+buildUUID+":isa string PBXBuildFile");

		// FIXME: IOS ONLY check if array insert is working here
		if( addToBuildResource ){
			// TEST 21092022
			string mediaAssetsUUID = "9936F60E1BFA4DEE00891288";
//			commands.emplace_back("Add :objects:"+mediaAssetsUUID+":files: string " + buildUUID);
			commands.emplace_back("# ---- addToBuildResource");
			commands.emplace_back("Add :objects:"+mediaAssetsUUID+":files: string " + UUID);
		}

		if( addToBuild ){
			// this replaces completely the findArrayForUUID
			// I found the root from the array (id present already on original project so no need to query an array by a member. in fact buildPhaseUUID maybe can be removed.
			commands.emplace_back("# ---- addToBuild");
			commands.emplace_back("Add :objects:"+buildActionMaskUUID+":files: string " + buildUUID);
		}
	}

	//-----------------------------------------------------------------
	// (C) resrouces
	//-----------------------------------------------------------------
	// MARK: IOS ONLY HERE // because resourcesUUID = "" in macOs
	if (addToResources == true && resourcesUUID != ""){
		commands.emplace_back("# ---- addToResources");
		string resUUID = generateUUID(srcFile.string() + "-build");
		commands.emplace_back("Add :objects:"+resUUID+":fileRef string "+UUID);
		commands.emplace_back("Add :objects:"+resUUID+":isa string PBXBuildFile");
		// FIXME: test if it is working on iOS
		commands.emplace_back("Add :objects:"+resourcesUUID+": string "+resUUID);
	}


	//-----------------------------------------------------------------
	// (D) folder
	//-----------------------------------------------------------------
	fs::path base;
	fs::path src { srcFile };
	fs::path folderFS { folder };

	if (!fs::exists(folderFS)) {
		// cout << "folder doesn't exist " << folderFS << endl;
		fs::path parent = src.parent_path();
		auto nit = folderFS.end();

		base = parent;
		fs::path folderFS2 = folderFS;

		while(base.filename() == folderFS2.filename() && base.filename() != "" && folderFS2.filename() != "") {
			base = base.parent_path();
			folderFS2 = folderFS2.parent_path();
		}

//		cout << "srcFile " << srcFile << endl;
//		cout << "base " << base << endl;
//		cout << "folderFS2 " << folderFS2 << endl;
//		cout << "------ e" << endl;
	}

	// string xcodeProject::getFolderUUID(string folder, bool isFolder, string base) {
//	cout << ">>> getFolderUUID  " << folder << endl;
//	cout << ">>> base  " << base << endl;
	string folderUUID = getFolderUUID(folder, true, base);
	commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);
}

void xcodeProject::addFramework(const string & name, const fs::path & path, const fs::path & folder){
//	alert( "xcodeProject::addFramework " + name + " : " + path.string() + " : " + folder.string() , 33);

//	cout << "xcodeProject::addFramework " << name << " : " << path << " : " << folder << endl;
	// name = name of the framework
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);

	//-----------------------------------------------------------------
	// based on the extension make some choices about what to do:
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	// (A) make a FILE REF
	//-----------------------------------------------------------------

	string UUID = generateUUID( name );

	// encoding may be messing up for frameworks... so I switched to a pbx file ref without encoding fields
	//commands.emplace_back("Add :objects:"+UUID+":fileEncoding string 4");

	commands.emplace_back("# ----- addFramework name="+name+" path="+path.string()+" folder=" +folder.string());
	commands.emplace_back("Add :objects:"+UUID+":name string "+name);
	commands.emplace_back("Add :objects:"+UUID+":path string "+path.string());
	commands.emplace_back("Add :objects:"+UUID+":isa string PBXFileReference");
	commands.emplace_back("Add :objects:"+UUID+":lastKnownFileType string wrapper.framework");
//	commands.emplace_back("Add :objects:"+UUID+":sourceTree string <group>");
	commands.emplace_back("Add :objects:"+UUID+":sourceTree string SOURCE_ROOT");


	commands.emplace_back("# ----- addFramework - add to build phase");
	string buildUUID = generateUUID(name + "-build");
	commands.emplace_back("Add :objects:"+buildUUID+":isa string PBXBuildFile");
	commands.emplace_back("Add :objects:"+buildUUID+":fileRef string "+UUID);

	// new - code sign frameworks on copy
	commands.emplace_back("# ----- addFramework - sign on copy");

	commands.emplace_back("Add :objects:"+buildUUID+":settings:ATTRIBUTES array");
	commands.emplace_back("Add :objects:"+buildUUID+":settings:ATTRIBUTES: string CodeSignOnCopy");

	//	this now adds the recently created object UUID to its parent folder
	string folderUUID = getFolderUUID(folder, false);
	commands.emplace_back("# ----- addFramework - add to parent folder : " + folder.string());
	commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);


	//commands.emplace_back("Add :objects:"+frameworksUUID+":children array");
	//commands.emplace_back("Add :objects:"+frameworksUUID+":children: string " + buildUUID);

	// we add the second to a final build phase for copying the framework into app.   we need to make sure we *don't* do this for system frameworks

	string buildUUID2;

	// maybe check if path exists in path
	if (!folder.empty() && !ofIsStringInString(path.string(), "/System/Library/Frameworks")
		&& target != "ios"){

		buildUUID2 = generateUUID(name + "-build2");
		commands.emplace_back("Add :objects:"+buildUUID2+":fileRef string "+UUID);
		commands.emplace_back("Add :objects:"+buildUUID2+":isa string PBXBuildFile");

		// new - code sign frameworks on copy
		commands.emplace_back("Add :objects:"+buildUUID2+":settings:ATTRIBUTES array");
		commands.emplace_back("Add :objects:"+buildUUID2+":settings:ATTRIBUTES: string CodeSignOnCopy");

		// UUID hardcoded para PBXCopyFilesBuildPhase
		// FIXME: hardcoded - this is the same for the next fixme. so maybe a clearer ident can make things better here.
		commands.emplace_back("Add :objects:E4C2427710CC5ABF004149E2:files: string " + buildUUID2);
	}

	commands.emplace_back("# ----- FRAMEWORK_SEARCH_PATHS");

	fs::path parentFolder { path.parent_path() };
//	alert ("parentFolder " + parentFolder.string() );

	for (auto & c : buildConfigs) {
		commands.emplace_back
		("Add :objects:"+c+":buildSettings:FRAMEWORK_SEARCH_PATHS: string " + parentFolder.string());
	}

	if (target != "ios" && !folder.empty()){
		// add it to the linking phases...
		// PBXFrameworksBuildPhase
		// https://www.rubydoc.info/gems/xcodeproj/Xcodeproj/Project/Object/PBXFrameworksBuildPhase
		// The phase responsible on linking with frameworks. Known as ‘Link Binary With Libraries` in the UI.

		// This is what was missing. a reference in root objects to the framework, so we can add the reference to PBXFrameworksBuildPhase
		auto tempUUID = generateUUID(name + "-InFrameworks");
		commands.emplace_back("Add :objects:"+tempUUID+":fileRef string "+UUID);
		commands.emplace_back("Add :objects:"+tempUUID+":isa string PBXBuildFile");

		commands.emplace_back("# --- PBXFrameworksBuildPhase");
		commands.emplace_back("Add :objects:E4B69B590A3A1756003C02F2:files: string " + tempUUID);
	}
		// return;

}


//void xcodeProject::addDylib(string name, string path){
void xcodeProject::addDylib(const string & name, const fs::path & path, const fs::path & folder){
//	alert( "xcodeProject::addDylib " + name + " : " + path.string() , 33);

	// name = name of the dylib
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);

	//-----------------------------------------------------------------
	// based on the extension make some choices about what to do:
	//-----------------------------------------------------------------

	//-----------------------------------------------------------------
	// (A) make a FILE REF
	//-----------------------------------------------------------------

	string UUID = generateUUID( name );

	// encoding may be messing up for frameworks... so I switched to a pbx file ref without encoding fields
	//commands.emplace_back("Add :objects:"+UUID+":fileEncoding string 4");
	commands.emplace_back("");
	commands.emplace_back("# ---- xcodeProject::addDylib " + name + " : " + path.string());
	commands.emplace_back("Add :objects:"+UUID+":name string "+name);
	commands.emplace_back("Add :objects:"+UUID+":path string "+path.string());
	commands.emplace_back("Add :objects:"+UUID+":isa string PBXFileReference");
	commands.emplace_back("Add :objects:"+UUID+":lastKnownFileType string compiled.mach-o.dylib");
	commands.emplace_back("Add :objects:"+UUID+":sourceTree string SOURCE_ROOT");


	// add to folder in xcode?
	string folderUUID = getFolderUUID(folder, false);
	commands.emplace_back("");
	commands.emplace_back("# --- folderUUID " + folderUUID);
	commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);


	string buildUUID = generateUUID(name + "-build");
	commands.emplace_back("");
	commands.emplace_back("# --- buildUUID " + buildUUID);
	commands.emplace_back("Add :objects:"+buildUUID+":isa string PBXBuildFile");
	commands.emplace_back("Add :objects:"+buildUUID+":fileRef string "+UUID);
	// new - code sign dylibs on copy
	commands.emplace_back("Add :objects:"+buildUUID+":settings:ATTRIBUTES array");
	commands.emplace_back("Add :objects:"+buildUUID+":settings:ATTRIBUTES: string CodeSignOnCopy");

//	// we add one of the build refs to the list of frameworks
//	// TENTATIVA desesperada aqui...
//	string folderUUID = getFolderUUID(folder);
//	commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);

//	string folderUUID = getFolderUUID(folder, false);
//	commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);


	string buildUUID2 = generateUUID(name + "-build2");
	commands.emplace_back("");
	commands.emplace_back("# --- buildUUID2 " + buildUUID2);
	commands.emplace_back("Add :objects:"+buildUUID2+":fileRef string "+UUID);
	commands.emplace_back("Add :objects:"+buildUUID2+":isa string PBXBuildFile");

	// new - code sign frameworks on copy
	commands.emplace_back("Add :objects:"+buildUUID2+":settings:ATTRIBUTES array");
	commands.emplace_back("Add :objects:"+buildUUID2+":settings:ATTRIBUTES: string CodeSignOnCopy");

	// UUID hardcoded para PBXCopyFilesBuildPhase
	// FIXME: hardcoded - this is the same for the next fixme. so maybe a clearer ident can make things better here.

	commands.emplace_back("Add :objects:E4A5B60F29BAAAE400C2D356:files: string " + buildUUID2);
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
