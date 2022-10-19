#include "xcodeProject.h"
#include "Utils.h"
#include <iostream>
#include "json.hpp"

using std::vector;
using std::string;
using std::cout;
using std::endl;
using nlohmann::json;
using nlohmann::json_pointer;

xcodeProject::xcodeProject(string target)
:baseProject(target){
	// FIXME: remove unused variables
	if( target == "osx" ){
		folderUUID = {
			{ "src", 			"E4B69E1C0A3A1BDC003C02F2" },
			{ "addons", 		"BB4B014C10F69532006C3DED" },
			{ "localAddons",	"6948EE371B920CB800B5AC1A" },
			{ "", 				"E4B69B4A0A3A1720003C02F2" }
		};

		buildConfigurationListUUID = "E4B69B5A0A3A1756003C02F2";
		buildActionMaskUUID = "E4B69B580A3A1756003C02F2";

		projRootUUID    = "E4B69B4A0A3A1720003C02F2";
		resourcesUUID   = "";
		frameworksUUID  = "E7E077E715D3B6510020DFD4";   //PBXFrameworksBuildPhase
		afterPhaseUUID  = "928F60851B6710B200E2D791";
		buildPhasesUUID  = "E4C2427710CC5ABF004149E2";
		
	} else {
		folderUUID = {
			{ "src", 			"E4D8936A11527B74007E1F53" },
			{ "addons", 		"BB16F26B0F2B646B00518274" },
			{ "localAddons", 	"6948EE371B920CB800B5AC1A" },
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
	string xcodeProject = ofFilePath::join(projectDir , projectName + ".xcodeproj");
	
	if (ofDirectory::doesDirectoryExist(xcodeProject)){
		ofDirectory::removeDirectory(xcodeProject, true);
	}
   
	ofDirectory xcodeDir(xcodeProject);
	xcodeDir.create(true);
	xcodeDir.close();
	
	ofFile::copyFromTo(ofFilePath::join(templatePath,"emptyExample.xcodeproj/project.pbxproj"),
					   ofFilePath::join(xcodeProject, "project.pbxproj"), true, true);

	findandreplaceInTexfile(ofFilePath::join(xcodeProject, "project.pbxproj"), "emptyExample", projectName);

	ofFile::copyFromTo(ofFilePath::join(templatePath,"Project.xcconfig"),projectDir, true, true);
	
	ofDirectory binDirectory(ofFilePath::join(projectDir, "bin"));
	if (!binDirectory.exists()){
		ofDirectory dataDirectory(ofFilePath::join(projectDir, "bin/data"));
		dataDirectory.create(true);
		dataDirectory.close();
	}
	if(binDirectory.exists()){
		ofDirectory dataDirectory(ofFilePath::join(binDirectory.path(), "data"));
		if (!dataDirectory.exists()){
			dataDirectory.create(false);
		}
		
		// originally only on IOS
		//this is needed for 0.9.3 / 0.9.4 projects which have iOS media assets in bin/data/
		ofDirectory srcDataDir(ofFilePath::join(templatePath, "bin/data"));
		if( srcDataDir.exists() ){
			baseProject::recursiveCopyContents(srcDataDir, dataDirectory);
		}
		dataDirectory.close();
		srcDataDir.close();
	}
	binDirectory.close();
	
	if( target == "osx" ){
		ofFile::copyFromTo(ofFilePath::join(templatePath,"openFrameworks-Info.plist"),projectDir, true, true);
		ofFile::copyFromTo(ofFilePath::join(templatePath,"of.entitlements"),projectDir, true, true);
	}else{
		ofFile::copyFromTo(ofFilePath::join(templatePath,"ofxiOS-Info.plist"),projectDir, true, true);
		ofFile::copyFromTo(ofFilePath::join(templatePath,"ofxiOS_Prefix.pch"),projectDir, true, true);

		ofDirectory mediaAssetsTemplateDirectory(ofFilePath::join(templatePath, "mediaAssets"));
		ofDirectory mediaAssetsProjectDirectory(ofFilePath::join(projectDir, "mediaAssets"));
		if (!mediaAssetsProjectDirectory.exists()){
			mediaAssetsTemplateDirectory.copyTo(mediaAssetsProjectDirectory.getAbsolutePath(), false, false);
		}
		mediaAssetsTemplateDirectory.close();
		mediaAssetsProjectDirectory.close();
	}

	/// SAVESCHEME HERE
	saveScheme();

	if(target=="osx"){
		saveMakefile();
	}

	// make everything relative the right way.
	string relRoot = getOFRelPath(ofFilePath::removeTrailingSlash(projectDir));
	if (relRoot != "../../../"){
		string relPath2 = relRoot;
		
		relPath2.erase(relPath2.end()-1);
		findandreplaceInTexfile(projectDir + projectName + ".xcodeproj/project.pbxproj", "../../..", relPath2);
		//findandreplaceInTexfile(projectDir + "Project.xcconfig", "../../../", relRoot);
		findandreplaceInTexfile(projectDir + "Project.xcconfig", "../../..", relPath2);
		if( target == "osx" ){
			findandreplaceInTexfile(projectDir + "Makefile", "../../..", relPath2);
			findandreplaceInTexfile(projectDir + "config.make", "../../..", relPath2);
		}
	}
	return true;
}

void xcodeProject::saveScheme(){
	string schemeFolder = projectDir + projectName + ".xcodeproj" + "/xcshareddata/xcschemes/";
	if (ofDirectory::doesDirectoryExist(schemeFolder)){
		ofDirectory::removeDirectory(schemeFolder, true);
	}
	ofDirectory::createDirectory(schemeFolder, false, true);
	
	if(target=="osx"){
		for (auto & f : { string("Release"), string("Debug") }) {
			string schemeTo = schemeFolder + projectName + " " +f+ ".xcscheme";
			ofFile::copyFromTo(ofFilePath::join(templatePath, "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample "+f+".xcscheme"), schemeTo);
			findandreplaceInTexfile(schemeTo, "emptyExample", projectName);
		}
	 
		string workspaceTo = projectDir  + projectName + ".xcodeproj/project.xcworkspace";
		ofFile::copyFromTo(ofFilePath::join(templatePath, "emptyExample.xcodeproj/project.xcworkspace"), workspaceTo);
	}else{

		// MARK:- IOS sector;
		string schemeTo = schemeFolder + projectName + ".xcscheme";
		ofFile::copyFromTo(ofFilePath::join(templatePath, "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample.xcscheme"), schemeTo);
		findandreplaceInTexfile(schemeTo, "emptyExample", projectName);
	}
}

void xcodeProject::saveMakefile(){
	for (auto & f : {"Makefile", "config.make" }) {
		string fileName = ofFilePath::join(projectDir, f);
		if(!ofFile(fileName).exists()){
			ofFile::copyFromTo(ofFilePath::join(templatePath, f), fileName, true, true);
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
		commands.emplace_back("Set :objects:E4B69B5B0A3A1756003C02F2:path " + projectName + "Debug.app");
	}
}

string xcodeProject::getFolderUUID(string folder) {
	string UUID = "";
	if ( folderUUID.find(folder) == folderUUID.end() ) { // NOT FOUND
		vector < string > folders = ofSplitString(folder, "/", true);
		string lastFolderUUID = projRootUUID;
		
		if (folders.size()){
			for (int a=0; a<folders.size(); a++) {
				 vector <string> joinFolders;
				 joinFolders.assign(folders.begin(), folders.begin() + (a+1));
				 string fullPath = ofJoinString(joinFolders, "/");
				
				// folder is still not found here:
				if ( folderUUID.find(fullPath) == folderUUID.end() ) {

					string thisUUID = generateUUID(fullPath);
					folderUUID[fullPath] = thisUUID;
					
					// here we add an UUID for the group (folder) and we initialize an array to receive children (files or folders inside)
					commands.emplace_back("Add :objects:"+thisUUID+":isa string PBXGroup");
					commands.emplace_back("Add :objects:"+thisUUID+":name string "+folders[a]);
					commands.emplace_back("Add :objects:"+thisUUID+":children array");
					commands.emplace_back("Add :objects:"+thisUUID+":sourceTree string <group>");

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
		UUID = folderUUID[folder];
	}
	return UUID;
}


void xcodeProject::addSrc(string srcFile, string folder, SrcType type){

	string buildUUID;

	//-----------------------------------------------------------------
	// find the extension for the file that's passed in.
	//-----------------------------------------------------------------

	size_t found = srcFile.find_last_of(".");
	string ext = srcFile.substr(found+1);

	//-----------------------------------------------------------------
	// based on the extension make some choices about what to do:
	//-----------------------------------------------------------------

	bool addToResources = true;
	bool addToBuild = true;
	bool addToBuildResource = false;
	string fileKind = "file";

	if(type==DEFAULT){
		if( ext == "cpp" || ext == "cc" || ext =="cxx" ){
			fileKind = "sourcecode.cpp.cpp";
			addToResources = false;
		}
		else if( ext == "c" ){
			fileKind = "sourcecode.c.c";
			addToResources = false;
		}
		else if(ext == "h" || ext == "hpp"){
			fileKind = "sourcecode.c.h";
			addToBuild = false;
			addToResources = false;
		}
		else if(ext == "mm" || ext == "m"){
			addToResources = false;
			fileKind = "sourcecode.cpp.objcpp";
		}
		else if(ext == "xib"){
			fileKind = "file.xib";
			addToBuild	= false;
			addToBuildResource = true;
			addToResources = true;
		}
		else if(ext == ".metal"){
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
	}else{
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

	string UUID = generateUUID(srcFile);   // replace with theo's smarter system.
	string name, path;
	splitFromLast(srcFile, "/", path, name);

	commands.emplace_back("Add :objects:"+UUID+":path string "+srcFile);
	commands.emplace_back("Add :objects:"+UUID+":isa string PBXFileReference");
	commands.emplace_back("Add :objects:"+UUID+":name string "+name);
	if(ext == "xib"){
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
		
		buildUUID = generateUUID(srcFile + "-build");
		
		commands.emplace_back("Add :objects:"+buildUUID+":fileRef string "+UUID);

		/*
		if (addToBuild) {
			cout << "ADD TO BUILD" << endl;
			cout << srcFile << endl;
			cout << commands.back() << endl;
			cout << "-----" << endl;
		}
		if (addToBuildResource) {
			cout << "ADD TO BUILD RESOURCE" << endl;
			cout << srcFile << endl;
			cout << commands.back() << endl;
			cout << "-----" << endl;
		}
		*/
		commands.emplace_back("Add :objects:"+buildUUID+":isa string PBXBuildFile");
		
		// FIXME: IOS ONLY check if array insert is working here
		if( addToBuildResource ){

			// TEST 21092022
			string mediaAssetsUUID = "9936F60E1BFA4DEE00891288";
//			commands.emplace_back("Add :objects:"+mediaAssetsUUID+":files: string " + buildUUID);
			commands.emplace_back("Add :objects:"+mediaAssetsUUID+":files: string " + UUID);
		}

		if( addToBuild ){
			// this replaces completely the findArrayForUUID
			// I found the root from the array (id present already on original project so no need to query an array by a member. in fact buildPhaseUUID maybe can be removed.
			commands.emplace_back("Add :objects:"+buildActionMaskUUID+":files: string " + buildUUID);
		}
	}

	//-----------------------------------------------------------------
	// (C) resrouces
	//-----------------------------------------------------------------

	// MARK: IOS ONLY HERE // because resourcesUUID = "" in macOs
	if (addToResources == true && resourcesUUID != ""){
		
		string resUUID = generateUUID(srcFile + "-build");
		
		commands.emplace_back("Add :objects:"+resUUID+":fileRef string "+UUID);
		commands.emplace_back("Add :objects:"+resUUID+":isa string PBXBuildFile");
		
		// FIXME: test if it is working on iOS
		commands.emplace_back("Add :objects:"+resourcesUUID+": string "+resUUID);
	}


	//-----------------------------------------------------------------
	// (D) folder
	//-----------------------------------------------------------------

	string folderUUID = getFolderUUID(folder);
	commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);
}

void xcodeProject::addFramework(string name, string path, string folder){
	// name = name of the framework
	// path = the full path (w name) of this framework
	// folder = the path in the addon (in case we want to add this to the file browser -- we don't do that for system libs);
	
	//-----------------------------------------------------------------
	// based on the extension make some choices about what to do:
	//-----------------------------------------------------------------
	
	//-----------------------------------------------------------------
	// (A) make a FILE REF
	//-----------------------------------------------------------------
	
	// encoding may be messing up for frameworks... so I switched to a pbx file ref without encoding fields
	string UUID = generateUUID( name );

	//commands.emplace_back("Add :objects:"+UUID+":fileEncoding string 4");
	commands.emplace_back("Add :objects:"+UUID+":path string "+path);
	commands.emplace_back("Add :objects:"+UUID+":isa string PBXFileReference");
	commands.emplace_back("Add :objects:"+UUID+":name string "+name);
	commands.emplace_back("Add :objects:"+UUID+":lastKnownFileType string wrapper.framework");
	commands.emplace_back("Add :objects:"+UUID+":sourceTree string <group>");
	
	string buildUUID = generateUUID(name + "-build");
	commands.emplace_back("Add :objects:"+buildUUID+":isa string PBXBuildFile");
	commands.emplace_back("Add :objects:"+buildUUID+":fileRef string "+UUID);
	
	// new - code sign frameworks on copy
	commands.emplace_back("Add :objects:"+buildUUID+":settings:ATTRIBUTES array");
	commands.emplace_back("Add :objects:"+buildUUID+":settings:ATTRIBUTES: string CodeSignOnCopy");

	// we add one of the build refs to the list of frameworks
	// TENTATIVA desesperada aqui...
	string folderUUID = getFolderUUID(folder);
	commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);

	//commands.emplace_back("Add :objects:"+frameworksUUID+":children array");
	//commands.emplace_back("Add :objects:"+frameworksUUID+":children: string " + buildUUID);

	// we add the second to a final build phase for copying the framework into app.   we need to make sure we *don't* do this for system frameworks
	
	if (folder.size() != 0 && !ofIsStringInString(path, "/System/Library/Frameworks")
		&& target != "ios"){
		
		string buildUUID2 = generateUUID(name + "-build2");
		commands.emplace_back("Add :objects:"+buildUUID2+":fileRef string "+UUID);
		commands.emplace_back("Add :objects:"+buildUUID2+":isa string PBXBuildFile");
		
		// new - code sign frameworks on copy
		commands.emplace_back("Add :objects:"+buildUUID2+":settings:ATTRIBUTES array");
		commands.emplace_back("Add :objects:"+buildUUID2+":settings:ATTRIBUTES: string CodeSignOnCopy");

		
		// UUID hardcoded para PBXCopyFilesBuildPhase
		// FIXME: hardcoded - this is the same for the next fixme. so maybe a clearer ident can make things better here.
		commands.emplace_back("Add :objects:E4C2427710CC5ABF004149E2:files: string " + buildUUID2);
	}
	
	// now, we get the path for this framework without the name

	string pathWithoutName;
	vector < string > pathSplit = ofSplitString(path, "/");
	for (int i = 0; i < pathSplit.size()-1; i++){
		if (i != 0) pathWithoutName += "/";
		pathWithoutName += pathSplit[i];
	}
	
	// then, we are going to add this to "FRAMEWORK_SEARCH_PATHS" -- we do this twice, once for debug once for release.
	
	for (auto & c : buildConfigs) {
		commands.emplace_back
		("Add :objects:"+c+":buildSettings:FRAMEWORK_SEARCH_PATHS: string " + pathWithoutName);
	}

	// finally, this is for making folders based on the frameworks position in the addon. so it can appear in the sidebar / file explorer
	
	if (folder.size() > 0 && !ofIsStringInString(folder, "/System/Library/Frameworks")){
		string folderUUID = getFolderUUID(folder);
	} else { 
		//FIXME: else what?
	}
	
	if (target != "ios" && folder.size() != 0){
		// add it to the linking phases...
		// FIXME: hardcoded UUID
		commands.emplace_back("Add :objects:E4B69B590A3A1756003C02F2:files: string "+buildUUID);
	}
}

void xcodeProject::addInclude(string includeName){
	// Adding source to all build configurations, debug and release
	for (auto & c : buildConfigurations) {
		commands.emplace_back("Add :objects:"+c+":buildSettings:HEADER_SEARCH_PATHS: string " + includeName);
	}
}

void xcodeProject::addLibrary(const LibraryBinary & lib){
	// TODO: Test this
	for (auto & c : buildConfigs) {
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_LDFLAGS: string " + lib.path);
	}
}

// FIXME: libtype is unused here and in the next configurations
void xcodeProject::addLDFLAG(string ldflag, LibType libType){
	for (auto & c : buildConfigs) {
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_LDFLAGS: string " + ldflag);
	}
}

void xcodeProject::addCFLAG(string cflag, LibType libType){
	for (auto & c : buildConfigurations) {
		// FIXME: add array here if it doesnt exist
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_CFLAGS array");
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
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":buildActionMask string 2147483647");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":files array");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":inputPaths array");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":isa string PBXShellScriptBuildPhase");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":outputPaths array");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":runOnlyForDeploymentPostprocessing string 0");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":shellPath string /bin/sh");
	commands.emplace_back("Add :objects:"+afterPhaseUUID+":shellScript string " + rule);
	
	// adding this phase to build phases array
	// TODO: Check if nit needs another buildConfigurationListUUID for debug.
	commands.emplace_back("Add :objects:"+buildConfigurationListUUID+":buildPhases: string " + afterPhaseUUID);

}

void xcodeProject::addAddon(ofAddon & addon){
	for(int i=0;i<(int)addons.size();i++){
		if(addons[i].name==addon.name){
			return;
		}
	}

	for(int i=0;i<addon.dependencies.size();i++){
		baseProject::addAddon(addon.dependencies[i]);
	}

	for(int i=0;i<addon.dependencies.size();i++){
		for(int j=0;j<(int)addons.size();j++){
			if(addon.dependencies[i] != addons[j].name){ //make sure dependencies of addons arent already added to prj
				baseProject::addAddon(addon.dependencies[i]);
			}else{
				//trying to add duplicated addon dependency... skipping!
			}
		}
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
	std::sort(addon.srcFiles.begin(), addon.srcFiles.end(), std::less<string>());
	for(int i=0;i<(int)addon.srcFiles.size(); i++){
		ofLogVerbose() << "adding addon srcFiles: " << addon.srcFiles[i];
		addSrc(addon.srcFiles[i],addon.filesToFolders[addon.srcFiles[i]]);
	}
	for(int i=0;i<(int)addon.defines.size(); i++){
		ofLogVerbose() << "adding addon defines: " << addon.defines[i];
		addDefine(addon.defines[i]);
	}

	for(int i=0;i<(int)addon.frameworks.size(); i++){
		ofLogVerbose() << "adding addon frameworks: " << addon.frameworks[i];
		
		size_t found=addon.frameworks[i].find('/');
		if (found==string::npos){
			if (target == "ios"){
				addFramework( addon.frameworks[i] + ".framework", "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/System/Library/Frameworks/" +
					addon.frameworks[i] + ".framework", "addons/" + addon.name + "/frameworks");
			} else {
				addFramework( addon.frameworks[i] + ".framework",
					"/System/Library/Frameworks/" +
					addon.frameworks[i] + ".framework", "addons/" + addon.name + "/frameworks");
			}
		} else {
			if (ofIsStringInString(addon.frameworks[i], "/System/Library")){
				vector < string > pathSplit = ofSplitString(addon.frameworks[i], "/");
				addFramework(pathSplit[pathSplit.size()-1],
							 addon.frameworks[i],
							 "addons/" + addon.name + "/frameworks");
				
			} else {
				vector < string > pathSplit = ofSplitString(addon.frameworks[i], "/");
				addFramework(pathSplit[pathSplit.size()-1],
							 addon.frameworks[i],
							 addon.filesToFolders[addon.frameworks[i]]);
			}
		}
	}
}

bool xcodeProject::saveProjectFile(){
	std::string fileName = projectDir + projectName + ".xcodeproj/project.pbxproj";
	
	// JSON Block - Multiplatform
	string contents = ofBufferFromFile(fileName).getText();
	json j = json::parse(contents);

	for (auto & c : commands) {
		//cout << c << endl;
		vector<string> cols = ofSplitString(c, " ");
		string thispath = cols[1];
		ofStringReplace(thispath, ":", "/");
		
		if (thispath.substr(thispath.length() -1) != "/") {
			//if (cols[0] == "Set") {
			json::json_pointer p = json::json_pointer(thispath);
			if (cols[2] == "string") {
				j[p] = cols[3];
			}
			else if (cols[2] == "array") {
				j[p] = {};
			}
		}
		else {
			thispath = thispath.substr(0, thispath.length() -1);
			json::json_pointer p = json::json_pointer(thispath);
			try {
				// Fixing XCode one item array issue
				if (!j[p].is_array()) {
					auto v = j[p];
					j[p] = json::array();
					if (!v.is_null()) {
						j[p].push_back(v);
					}
				}
				j[p].push_back(cols[3]);

			} catch (std::exception e) {
				cout << "json error " << endl;
				cout << e.what() << endl;
			}
		}
	}
	
	ofFile jsonFile(fileName, ofFile::WriteOnly);
	try{
		jsonFile << j.dump(1, '	');
	}catch(std::exception & e){
		ofLogError("ofSaveJson") << "Error saving json to " << fileName << ": " << e.what();
		return false;
	}catch(...){
		ofLogError("ofSaveJson") << "Error saving json to " << fileName;
		return false;
	}
	
//	PLISTBUDDY - Mac only
//	{
//		string command = "/usr/libexec/PlistBuddy " + fileName;
//		string allCommands = "";
//		for (auto & c : commands) {
//			command += " -c \"" + c + "\"";
//			allCommands += c + "\n";
//		}
//		cout << ofSystem(command) << endl;
//		cout << allCommands << endl;
//	}
	return true;
}
