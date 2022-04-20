#include "xcodeProject.h"
#include "Utils.h"
#include "ofXml.h"
#include <iostream>

xcodeProject::xcodeProject(std::string target)
:baseProject(target){
	alert("xcodeProject");	
	
	// FIXME: remove unused variables
	if( target == "osx" ){
		folderUUID = {
			{ "src", "E4B69E1C0A3A1BDC003C02F2" },
			{ "addons", "BB4B014C10F69532006C3DED" },
			{ "localAddons", "6948EE371B920CB800B5AC1A" },
			{ "", "E4B69B4A0A3A1720003C02F2" }
		};

		// FIXME: get this UUIDs for IOS too
		buildConfigurationListUUID = "E4B69B5A0A3A1756003C02F2";
		buildActionMaskUUID = "E4B69B580A3A1756003C02F2";

		projRootUUID    = "E4B69B4A0A3A1720003C02F2";
//        srcUUID         = "E4B69E1C0A3A1BDC003C02F2"; // covered in map
//        addonUUID       = "BB4B014C10F69532006C3DED"; // covered in map
//        localAddonUUID  = "6948EE371B920CB800B5AC1A"; // covered in map
//        buildPhaseUUID  = "E4B69E200A3A1BDC003C02F2";   // not needed, we use buildactionmask UUID
		resourcesUUID   = "";
		frameworksUUID  = "E7E077E715D3B6510020DFD4";   //PBXFrameworksBuildPhase
		afterPhaseUUID  = "928F60851B6710B200E2D791";
		buildPhasesUUID  = "E4C2427710CC5ABF004149E2";
//		frameworksBuildPhaseUUID = "E4328149138ABC9F0047C5CB";
		
	}else{
		folderUUID = {
			{ "src", 			"E4D8936A11527B74007E1F53" },
			{ "addons", 		"BB16F26B0F2B646B00518274" },
			{ "localAddons", 	"6948EE371B920CB800B5AC1A" },
			{ "", 				"29B97314FDCFA39411CA2CEA" }
		};

		projRootUUID    = "29B97314FDCFA39411CA2CEA";
//        srcUUID         = "E4D8936A11527B74007E1F53";
//        addonUUID       = "BB16F26B0F2B646B00518274";
//        localAddonUUID  = "6948EE371B920CB800B5AC1A";
//        buildPhaseUUID  = "E4D8936E11527B74007E1F53";
		resourcesUUID   = "BB24DD8F10DA77E000E9C588";
		buildPhaseResourcesUUID = "BB24DDCA10DA781C00E9C588";
		frameworksUUID  = "1DF5F4E00D08C38300B7A737";   //PBXFrameworksBuildPhase  // todo: check this?
		afterPhaseUUID  = "928F60851B6710B200E2D791";
		buildPhasesUUID = "9255DD331112741900D6945E";
	}
};


bool xcodeProject::createProjectFile(){
	alert("createProjectFile");
	// todo: some error checking.

	std::string xcodeProject = ofFilePath::join(projectDir , projectName + ".xcodeproj");
	
	if (ofDirectory::doesDirectoryExist(xcodeProject)){
		ofDirectory::removeDirectory(xcodeProject, true);
	}
   
	ofDirectory xcodeDir(xcodeProject);
	xcodeDir.create(true);
	xcodeDir.close();
	
	ofFile::copyFromTo(ofFilePath::join(templatePath,"emptyExample.xcodeproj/project.pbxproj"),
					   ofFilePath::join(xcodeProject, "project.pbxproj"), true, true);

	ofFile::copyFromTo(ofFilePath::join(templatePath,"Project.xcconfig"),projectDir, true, true);

	
	ofDirectory binDirectory(ofFilePath::join(projectDir, "bin"));
	if (!binDirectory.exists()){
		ofDirectory dataDirectory(ofFilePath::join(projectDir, "bin/data"));
		dataDirectory.create(true);
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
	}
	
	
	if( target == "osx" ){
		if (!ofFile::doesFileExist(ofFilePath::join(projectDir, "openFrameworks-Info.plist"))) {
			ofFile::copyFromTo(ofFilePath::join(templatePath,"openFrameworks-Info.plist"),projectDir, true, true);
		} else {
			std::cout << "openFrameworks-Info.plist already exists" << std::endl;
		}
	}else{
		ofFile::copyFromTo(ofFilePath::join(templatePath,"ofxiOS-Info.plist"),projectDir, true, true);
		ofFile::copyFromTo(ofFilePath::join(templatePath,"ofxiOS_Prefix.pch"),projectDir, true, true);

		ofDirectory mediaAssetsTemplateDirectory(ofFilePath::join(templatePath, "mediaAssets"));
		ofDirectory mediaAssetsProjectDirectory(ofFilePath::join(projectDir, "mediaAssets"));
		if (!mediaAssetsProjectDirectory.exists()){
			mediaAssetsTemplateDirectory.copyTo(mediaAssetsProjectDirectory.getAbsolutePath(), false, false);
		}
	}

	saveScheme();
	if(target=="osx"){
		saveMakefile();
	}

	// make everything relative the right way.
	std::string relRoot = getOFRelPath(ofFilePath::removeTrailingSlash(projectDir));
//	std::cout << "XXXXX " << relRoot << std::endl;
	if (relRoot != "../../../"){
		std::string relPath2 = relRoot;
//		std::cout << "XXXXX " << relPath2 << std::endl;
		
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
	alert("saveScheme");
	std::string schemeFolder = projectDir + projectName + ".xcodeproj" + "/xcshareddata/xcschemes/";
	if (ofDirectory::doesDirectoryExist(schemeFolder)){
		ofDirectory::removeDirectory(schemeFolder, true);
	}
	ofDirectory::createDirectory(schemeFolder, false, true);
	
	if(target=="osx"){
		std::string schemeToD = projectDir  + projectName + ".xcodeproj" + "/xcshareddata/xcschemes/" + projectName + " Debug.xcscheme";
		ofFile::copyFromTo(ofFilePath::join(templatePath, "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample Debug.xcscheme"), schemeToD);
	
		std::string schemeToR = projectDir  + projectName + ".xcodeproj" + "/xcshareddata/xcschemes/" + projectName + " Release.xcscheme";
		ofFile::copyFromTo(ofFilePath::join(templatePath, "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample Release.xcscheme"), schemeToR);
		findandreplaceInTexfile(schemeToD, "emptyExample", projectName);
		findandreplaceInTexfile(schemeToR, "emptyExample", projectName);
	 
		std::string workspaceTo = projectDir  + projectName + ".xcodeproj/project.xcworkspace";
		ofFile::copyFromTo(ofFilePath::join(templatePath, "emptyExample.xcodeproj/project.xcworkspace"), workspaceTo);
	}else{
		std::string schemeTo = projectDir  + projectName + ".xcodeproj" + "/xcshareddata/xcschemes/" + projectName + ".xcscheme";
		ofFile::copyFromTo(ofFilePath::join(templatePath, "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample.xcscheme"), schemeTo);
		findandreplaceInTexfile(schemeTo, "emptyExample", projectName);
	}
}

void xcodeProject::saveMakefile(){
	alert("saveMakefile");
	for (auto & f : {"Makefile", "config.make" }) {
		std::string fileName = ofFilePath::join(projectDir, f);
		if(!ofFile(fileName).exists()){
			ofFile::copyFromTo(ofFilePath::join(templatePath, f), fileName, true, true);
		}
	}
	
//	std::string makefile = ofFilePath::join(projectDir,"Makefile");
//	if(!ofFile(makefile).exists()){
//		ofFile::copyFromTo(ofFilePath::join(templatePath, "Makefile"), makefile, true, true);
//	}
//
//	std::string configmake = ofFilePath::join(projectDir,"config.make");
//	if(!ofFile(configmake).exists()){
//		ofFile::copyFromTo(ofFilePath::join(templatePath, "config.make"), configmake, true, true);
//	}
}

bool xcodeProject::loadProjectFile(){
	alert("loadProjectFile");
	renameProject();
	// MARK: just to return something
	return true;
}

void xcodeProject::renameProject(){
	alert("renameProject");
	
	commands.emplace_back("Delete :objects:"+buildConfigurationListUUID+":name  ");
	commands.emplace_back("Add :objects:"+buildConfigurationListUUID+":name string " + projectName);
	// FIXME: review BUILT_PRODUCTS_DIR
	// TODO: Hardcode to variable
	commands.emplace_back("Delete :objects:E4B69B5B0A3A1756003C02F2:path  ");
	commands.emplace_back("Add :objects:E4B69B5B0A3A1756003C02F2:path string " + projectName + "Debug.app");

}

std::string xcodeProject::getFolderUUID(std::string folder) {
	std::string UUID = "";
	if ( folderUUID.find(folder) == folderUUID.end() ) { // NOT FOUND
		std::vector < std::string > folders = ofSplitString(folder, "/", true);
		std::string lastFolderUUID = projRootUUID;
		
		if (folders.size()){
			for (int a=0; a<folders.size(); a++) {
				 std::vector <std::string> joinFolders;
				 joinFolders.assign(folders.begin(), folders.begin() + (a+1));
				 std::string fullPath = ofJoinString(joinFolders, "/");
				
				// folder is still not found here:
				if ( folderUUID.find(fullPath) == folderUUID.end() ) {

					std::string thisUUID = generateUUID(fullPath);
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


// MARK: -
void xcodeProject::addSrc(std::string srcFile, std::string folder, SrcType type){
	std::string buildUUID;

	//-----------------------------------------------------------------
	// find the extension for the file that's passed in.
	//-----------------------------------------------------------------

	size_t found = srcFile.find_last_of(".");
	std::string ext = srcFile.substr(found+1);

	//-----------------------------------------------------------------
	// based on the extension make some choices about what to do:
	//-----------------------------------------------------------------

	bool addToResources = true;
	bool addToBuild = true;
	bool addToBuildResource = false;
	std::string fileKind = "file";
	bool bAddFolder = true;

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
		}else if(ext == ".metal"){
			fileKind = "file.metal";
			addToBuild    = true;
			addToBuildResource = true;
			addToResources = true;
		}else if( target == "ios" ){
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
	
	if (folder == "src"){
		bAddFolder = false;
	}

	//-----------------------------------------------------------------
	// (A) make a FILE REF
	//-----------------------------------------------------------------

	std::string UUID = generateUUID(srcFile);   // replace with theo's smarter system.
	std::string name, path;
	splitFromLast(srcFile, "/", path, name);

	commands.emplace_back("Add :objects:"+UUID+":path string "+srcFile);
	commands.emplace_back("Add :objects:"+UUID+":isa string PBXFileReference");
	commands.emplace_back("Add :objects:"+UUID+":name string "+name);
	if(ext == "xib"){
		commands.emplace_back("Add :objects:"+UUID+":lastKnownFileType string "+fileKind);
	} else {
		commands.emplace_back("Add :objects:"+UUID+":explicitFileType string "+fileKind);
	}
	//	commands.emplace_back("Add :objects:"+UUID+":sourceTree string SOURCE_ROOT");
	commands.emplace_back("Add :objects:"+UUID+":sourceTree string <group>");
	commands.emplace_back("Add :objects:"+UUID+":fileEncoding string 4");

	//-----------------------------------------------------------------
	// (B) BUILD REF
	//-----------------------------------------------------------------

	if (addToBuild || addToBuildResource ){
		buildUUID = generateUUID(srcFile + "-build");
//		std::cout << "buildUUID = " << buildUUID << std::endl;
		
		commands.emplace_back("Add :objects:"+buildUUID+":fileRef string "+UUID);
		commands.emplace_back("Add :objects:"+buildUUID+":isa string PBXBuildFile");

		// FIXME: IOS ONLY checar se o insert no array ta funcionando aqui.
		if( addToBuildResource ){

			// FIXME: achar como fazer isto aqui, semelhante ao da proxima secao
//			commands.emplace_back("Add :objects:"+buildActionMaskUUID+":files: string " + buildUUID);
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
		
		std::string resUUID = generateUUID(srcFile + "-build");
		
		commands.emplace_back("Add :objects:"+resUUID+":fileRef string "+UUID);
		commands.emplace_back("Add :objects:"+resUUID+":isa string PBXBuildFile");
		
		// FIXME: testar se no IOS ta indo tudo bem. este aqui deve ser equivalente ao proximo q esta comentado.
		
		// FIXME: XAXA
		commands.emplace_back("Add :objects:"+resourcesUUID+": string "+resUUID);
	}


	//-----------------------------------------------------------------
	// (D) folder
	//-----------------------------------------------------------------

	if (bAddFolder == true){
		std::string folderUUID = getFolderUUID(folder);
		commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);

	} else {

	}
}

void xcodeProject::addFramework(std::string name, std::string path, std::string folder){
	alert ("addFramework name:" + name + " -- path:" + path + " -- folder:" + folder);
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
	std::string UUID = generateUUID( name );

//	commands.emplace_back("Add :objects:"+UUID+":fileEncoding string 4");
	commands.emplace_back("Add :objects:"+UUID+":path string "+path);
	commands.emplace_back("Add :objects:"+UUID+":isa string PBXFileReference");
	commands.emplace_back("Add :objects:"+UUID+":name string "+name);
	commands.emplace_back("Add :objects:"+UUID+":lastKnownFileType string wrapper.framework");
	commands.emplace_back("Add :objects:"+UUID+":sourceTree string <group>");
	
	std::string buildUUID = generateUUID(name + "-build");
	commands.emplace_back("Add :objects:"+buildUUID+":isa string PBXBuildFile");
	commands.emplace_back("Add :objects:"+buildUUID+":fileRef string "+UUID);
	
	// new - code sign frameworks on copy
	commands.emplace_back("Add :objects:"+buildUUID+":settings:ATTRIBUTES array");
	commands.emplace_back("Add :objects:"+buildUUID+":settings:ATTRIBUTES: string CodeSignOnCopy");

	// we add one of the build refs to the list of frameworks
	// FIXME: removi aqui pra testar
	// E7E077E715D3B6510020DFD4
	
	// TENTATIVA desesperada aqui...
	std::string folderUUID = getFolderUUID(folder);
	commands.emplace_back("Add :objects:"+folderUUID+":children: string " + UUID);

	
//	commands.emplace_back("Add :objects:"+frameworksUUID+":children array");
//	commands.emplace_back("Add :objects:"+frameworksUUID+":children: string " + buildUUID);

	// we add the second to a final build phase for copying the framework into app.   we need to make sure we *don't* do this for system frameworks
	
	if (folder.size() != 0 && !ofIsStringInString(path, "/System/Library/Frameworks")
		&& target != "ios"){
		
		std::string buildUUID2 = generateUUID(name + "-build2");
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

	std::string pathWithoutName;
	std::vector < std::string > pathSplit = ofSplitString(path, "/");
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
//		std::cout << "this " <<  folder << std::endl;
		std::string folderUUID = getFolderUUID(folder);
	} else { //FIXME: else what?
		
	}
	
	if (target != "ios" && folder.size() != 0){
		// add it to the linking phases...
		// FIXME: hardcoded UUID
		commands.emplace_back("Add :objects:E4B69B590A3A1756003C02F2:files: string "+buildUUID);
	}
}

void xcodeProject::addInclude(std::string includeName){
	alert("addInclude " + includeName);
	// Adding source to all build configurations, debug release appstore
	for (auto & c : buildConfigurations) {
		commands.emplace_back("Add :objects:"+c+":buildSettings:HEADER_SEARCH_PATHS: string " + includeName);
	}
}

void xcodeProject::addLibrary(const LibraryBinary & lib){
	alert("addInclude " + lib.path);
	// TODO: Test this
	for (auto & c : buildConfigs) {
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_LDFLAGS: string " + lib.path);
	}
}

// FIXME: libtype is unused here and in the next configurations
void xcodeProject::addLDFLAG(std::string ldflag, LibType libType){
	for (auto & c : buildConfigs) {
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_LDFLAGS: string " + ldflag);
	}
}

void xcodeProject::addCFLAG(std::string cflag, LibType libType){
	alert("addCFLAG " + cflag);

	for (auto & c : buildConfigurations) {
		// FIXME: add array here if it doesnt exist
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_CFLAGS array");
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_CFLAGS: string " + cflag);
	}
}

void xcodeProject::addDefine(std::string define, LibType libType){
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		commands.emplace_back("Add :objects:"+c+":buildSettings:GCC_PREPROCESSOR_DEFINITIONS: string " + define);
	}
}

// FIXME: libtype is unused here
void xcodeProject::addCPPFLAG(std::string cppflag, LibType libType){
	for (auto & c : buildConfigs) {
		// FIXME: add array here if it doesnt exist
		commands.emplace_back("Add :objects:"+c+":buildSettings:OTHER_CPLUSPLUSFLAGS: string " + cppflag);
	}
}

void xcodeProject::addAfterRule(std::string rule){
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
	alert("addAddon : " + addon.name);

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
	std::sort(addon.srcFiles.begin(), addon.srcFiles.end(), std::less<std::string>());
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
		if (found==std::string::npos){
			if (target == "ios"){
				addFramework( addon.frameworks[i] + ".framework", "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk/System/Library/Frameworks/" + addon.frameworks[i] + ".framework", "addons/" + addon.name + "/frameworks");
			} else {
			 addFramework( addon.frameworks[i] + ".framework", "/System/Library/Frameworks/" + addon.frameworks[i] + ".framework", "addons/" + addon.name + "/frameworks");
			}
		} else {
			
			if (ofIsStringInString(addon.frameworks[i], "/System/Library")){
				
				std::vector < std::string > pathSplit = ofSplitString(addon.frameworks[i], "/");
				
				addFramework(pathSplit[pathSplit.size()-1],
							 addon.frameworks[i],
							 "addons/" + addon.name + "/frameworks");
				
			} else {
			
				std::vector < std::string > pathSplit = ofSplitString(addon.frameworks[i], "/");
				
				addFramework(pathSplit[pathSplit.size()-1],
							 addon.frameworks[i],
							 addon.filesToFolders[addon.frameworks[i]]);
			}
		}
	}
}

bool xcodeProject::saveProjectFile(){
	alert("saveProjectFile");
	
	static std::string fileName = projectDir + projectName + ".xcodeproj/project.pbxproj";
	

//	if (2==3)
	{
		std::string command = "/usr/libexec/PlistBuddy " + fileName;
		std::string allCommands = "";
		for (auto & c : commands) {
			command += " -c \"" + c + "\"";
			allCommands += c + "\n";
		}
		std::cout << ofSystem(command) << std::endl;
		std::cout << allCommands << std::endl;
	}
	

	/*
	http://xpather.com
	/plist/dict/key
	 https://www.w3.org/2005/xpath-functions/

	 /plist/dict/key/
	 /plist/dict/key[text()='objects']/following-sibling::dict/
	 
	 /plist/dict/dict/key[text()='6948EE371B920CB800B5AC1A']
	 
	 //key[text()='6948EE371B920CB800B5AC1A']
	 //key[text()='6948EE371B920CB800B5AC1A']/following-sibling::node()[2]
	 //key[text()='6948EE371B920CB800B5AC1A']/following-sibling::dict[1]
	 
	 */
	
	
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(fileName.c_str());

//	doc.load(fileName);
//	ofXml xml;
//	xml.load(fileName);
//	std::cout << xml << std::endl;
	//	auto node = xml.getChild(path);
	//	if (nodes) {
	//		std::cout << "YESS " << std::endl;
	//	} else {
	//		std::cout << "node not found : " << path << " : " << nodes << std::endl;
	//	}


//	const std::string path = "//objects/key[text()='6948EE371B920CB800B5AC1A']/following-sibling::dict[1]"; ///following-sibling::dict[1]
//	auto nodes = doc.select_nodes(path.c_str());
//	std::cout << "------" << std::endl;
//	for(auto & node: nodes)
//	{
//		std::cout << node.node().name() << std::endl;
//	}
//	std::cout << "------" << std::endl;
		
//	if (2==3)
	for (auto & c : commands) {
		
		std::cout << c << std::endl;
		
		std::vector<std::string> cols = ofSplitString(c, " ");
		std::string thispath = cols[1];
		
		std::vector<std::string> parts = ofSplitString(thispath, ":");
		std::string xpath = "/";
		
		//	:objects:E4B69B610A3A1757003C02F2:buildSettings:HEADER_SEARCH_PATHS:

			//key[text()='objects']/following-sibling::dict[1]/key[text()='E4B69B610A3A1757003C02F2']/following-sibling::dict[1]/key[text()='buildSettings']
		bool first = true;
		for (auto & p : parts) {
			if (!first) {
				xpath += "/key[text()='"+p+"']/following-sibling::dict[1]";
			} else {
				first = false;
			}
		}
		
		std::cout << xpath << std::endl;
		//    xpath_node(const xml_attribute& attribute, const xml_node& parent);
		if (cols[2] == "string") {
			
//			00503                 xml_node insert_child_after(const char_t* name, const xml_node& node);
//			auto node = doc.select_node(xpath.c_str());
//			auto inserted = node.node().insert_child_after("key", node);
//			inserted.append_attribute("name");

//			pugi::xml_attribute = pugi::xml_attribute::as_string attr(cols[3]);
//			pugi::xpath_node(attr, );
		}
		auto nodes = doc.select_nodes(xpath.c_str());
		std::cout << "------" << std::endl;
		for(auto & node: nodes)
		{
			std::cout << node.node().name() << std::endl;
		}
//		std::cout << "------" << std::endl;
		
//		path = path.substr(1);
//		ofStringReplace(path, ":", "/following-sibling::*/");
//		path = "/dict/"+path;
		// if the last character in path is : or / then array value is appended.
		
//		auto node = xml.getChild(path);
//		if (nodes)
		{
			if (cols[0] == "Delete") {
//				xml.removeChild(node);
			}
		
			if (cols[0] == "Add") {
				if (cols[2] == "string") {
//					node.appendChild("string").set(cols[3]);
				} else {
					std::cout << c << std::endl;
				}
			} else {
				std::cout << c << std::endl;
			}
		}
	}
	// FIXME: temporary
	return true;
}

/*
 
 Delete :objects:E4B69B5A0A3A1756003C02F2:name
 Delete :objects:E4B69B5B0A3A1756003C02F2:path
 Add :objects:8EFFDE8A20D0EBA60F39CD03:children array
 Add :objects:99FA3DBC1C7456C400CFA0EE:buildSettings:OTHER_CFLAGS array
 Add :objects:99FA3DBB1C7456C400CFA0EE:buildSettings:OTHER_CFLAGS array
 Add :objects:7F3C04577424427D348B1425:children array
 Add :objects:2317994671CD76BBA16927B8:children array
 Add :objects:65F3085962B3A8825746C3C2:children array
 Add :objects:50EAFB5FF7C5722E7B4895C7:children array
 Add :objects:49A9E39C52C2DF4DDA1E185F:children array
 Add :objects:398D2AD7D162CE13BF40CD4F:children array
 Add :objects:821159A073ABA066739FD401:children array

 */
