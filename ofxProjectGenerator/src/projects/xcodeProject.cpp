#include "xcodeProject.h"
#include <iostream>
#include "Utils.h"



/*

xcode project files are plists but we can convert to xml, using plutil:

plutil -convert xml1 -o - myproj.xcodeproj/project.pbxproj

as an XML file, it's very odd and fairly unreadable, which is why this code is pretty gnarly.

some additional things that might be useful to try in the future:

(json parsing)  http://emilloer.com/2011/08/15/dealing-with-project-dot-pbxproj-in-ruby/
(objective c) https://github.com/expanz/xcode-editor
(plist c++) https://github.com/animetrics/PlistCpp

*/

// we are going to use POCO for computing the MD5 Hash of file names and paths, etc:
// to add things to the xcode project file, we need some template XML around
// these are common things we'll want to add

#define STRINGIFY(A)  #A

//-----------------------------------------------------------------
const char PBXGroup[] =
STRINGIFY(

    <key>GROUPUUID</key>
    <dict>
        <key>children</key>
        <array>
        </array>
        <key>isa</key>
        <string>PBXGroup</string>
        <key>name</key>
        <string>GROUPNAME</string>
        <key>sourceTree</key>
        <string>&lt;group&gt;</string>      // <group> or SOURCE_ROOT, etc
    </dict>

);

//-----------------------------------------------------------------
const char PBXFileReference[] =
STRINGIFY(

        <key>FILEUUID</key>
        <dict>
            <key>explicitFileType</key>
            <string>FILETYPE</string>
            <key>fileEncoding</key>
            <string>4</string>
            <key>isa</key>
            <string>PBXFileReference</string>
            <key>name</key>
            <string>FILENAME</string>
            <key>path</key>
            <string>FILEPATH</string>
            <key>sourceTree</key>
            <string>SOURCE_ROOT</string>
        </dict>

);

//-----------------------------------------------------------------
const char PBXFileReferenceWithoutEncoding[] =
STRINGIFY(
          
          <key>FILEUUID</key>
          <dict>
          <key>explicitFileType</key>
          <string>FILETYPE</string>
          <key>isa</key>
          <string>PBXFileReference</string>
          <key>name</key>
          <string>FILENAME</string>
          <key>path</key>
          <string>FILEPATH</string>
          <key>sourceTree</key>
          <string>SOURCE_ROOT</string>
          </dict>
          
          );


//-----------------------------------------------------------------
const char PBXFileReferenceXib[] =
STRINGIFY(

        <key>FILEUUID</key>
        <dict>
            <key>lastKnownFileType</key>
            <string>FILETYPE</string>
            <key>fileEncoding</key>
            <string>4</string>
            <key>isa</key>
            <string>PBXFileReference</string>
            <key>name</key>
            <string>FILENAME</string>
            <key>path</key>
            <string>FILEPATH</string>
            <key>sourceTree</key>
            <string>SOURCE_ROOT</string>
        </dict>

);

//-----------------------------------------------------------------
const char PBXBuildFile[] =
STRINGIFY(

          <key>BUILDUUID</key>
          <dict>
          <key>fileRef</key>
          <string>FILEUUID</string>
          <key>isa</key>
          <string>PBXBuildFile</string>
          </dict>

);

//-----------------------------------------------------------------
const char HeaderSearchPath[] =
STRINGIFY(

    <key>HEADER_SEARCH_PATHS</key>
    <array>
    <string>$(OF_CORE_HEADERS)</string>
    </array>

);


//-----------------------------------------------------------------
const char LDFlags[] =
STRINGIFY(

          <key>OTHER_LDFLAGS</key>
          <array>
          <string>$(OF_CORE_FRAMEWORKS) $(OF_CORE_LIBS)</string>
          </array>

);

//-----------------------------------------------------------------
const char CPPFlags[] =
STRINGIFY(

          <key>OTHER_CPLUSPLUSFLAGS</key>
          <array>
		  <string>"-D__MACOSX_CORE__"</string>
		  <string>"-mtune=native"</string>
		  <string>"-Wreturn-type"</string>
		  <string>"-Werror=return-type"</string>
          </array>

);

//-----------------------------------------------------------------
const char CFlags[] =
STRINGIFY(

          <key>OTHER_CFLAGS</key>
          <array>
          </array>

);

//-----------------------------------------------------------------
const char Defines[] =
STRINGIFY(

          <key>GCC_PREPROCESSOR_DEFINITIONS</key>
          <array></array>

);

const char workspace[] =
STRINGIFY(

          <?xml version="1.0" encoding="UTF-8"?>
          <Workspace version = "1.0">
              <FileRef
                location = "self:PROJECTNAME.xcodeproj">
              </FileRef>
          </Workspace>

);

const char afterRule[] =
STRINGIFY(
        <key>928F60851B6710B200E2D791</key>
        <dict>
            <key>buildActionMask</key>
            <string>2147483647</string>
            <key>files</key>
            <array />
            <key>inputPaths</key>
            <array />
            <key>isa</key>
            <string>PBXShellScriptBuildPhase</string>
            <key>outputPaths</key>
            <array />
            <key>runOnlyForDeploymentPostprocessing</key>
            <string>0</string>
            <key>shellPath</key>
            <string>/bin/sh</string>
            <key>shellScript</key>
            <string>SHELL_SCRIPT</string>
        </dict>
);

xcodeProject::xcodeProject(std::string target)
:baseProject(target){
    if( target == "osx" ){
        srcUUID         = "E4B69E1C0A3A1BDC003C02F2";
        addonUUID       = "BB4B014C10F69532006C3DED";
        localAddonUUID  = "6948EE371B920CB800B5AC1A";
        buildPhaseUUID  = "E4B69E200A3A1BDC003C02F2";   
        resourcesUUID   = "";
        frameworksUUID  = "E7E077E715D3B6510020DFD4";   //PBXFrameworksBuildPhase
        afterPhaseUUID  = "928F60851B6710B200E2D791";
        buildPhasesUUID  = "E4C2427710CC5ABF004149E2";
        frameworksBuildPhaseUUID = "E4328149138ABC9F0047C5CB";
        
    }else{
        srcUUID         = "E4D8936A11527B74007E1F53";
        addonUUID       = "BB16F26B0F2B646B00518274";
        localAddonUUID  = "6948EE371B920CB800B5AC1A";
        buildPhaseUUID  = "E4D8936E11527B74007E1F53";
        resourcesUUID   = "BB24DD8F10DA77E000E9C588";
        buildPhaseResourcesUUID = "BB24DDCA10DA781C00E9C588";
        frameworksUUID  = "1DF5F4E00D08C38300B7A737";   //PBXFrameworksBuildPhase  // todo: check this?
        afterPhaseUUID  = "928F60851B6710B200E2D791";
        buildPhasesUUID = "9255DD331112741900D6945E";   //
    }
};


void xcodeProject::saveScheme(){

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
	}else{
		std::string schemeTo = projectDir  + projectName + ".xcodeproj" + "/xcshareddata/xcschemes/" + projectName + ".xcscheme";
		ofFile::copyFromTo(ofFilePath::join(templatePath, "emptyExample.xcodeproj/xcshareddata/xcschemes/emptyExample.xcscheme"), schemeTo);
	    findandreplaceInTexfile(schemeTo, "emptyExample", projectName);
	}
	
	//TODO: do we still need this?
    //string xcsettings = projectDir  + projectName + ".xcodeproj" + "/xcshareddata/WorkspaceSettings.xcsettings";
    //ofFile::copyFromTo(templatePath + "emptyExample.xcodeproj/xcshareddata/WorkspaceSettings.xcsettings", xcsettings);

}


void xcodeProject::saveWorkspaceXML(){

    std::string workspaceFolder = projectDir + projectName + ".xcodeproj" + "/project.xcworkspace/";
    std::string xcodeProjectWorkspace = workspaceFolder + "contents.xcworkspacedata";    

    
    if (ofFile::doesFileExist(xcodeProjectWorkspace)){
        ofFile::removeFile(xcodeProjectWorkspace);
    }
    
    if (ofDirectory::doesDirectoryExist(workspaceFolder)){
        ofDirectory::removeDirectory(workspaceFolder, true);
    }
    
	ofDirectory::createDirectory(workspaceFolder, false, true);
    ofFile::copyFromTo(templatePath + "/emptyExample.xcodeproj/project.xcworkspace/contents.xcworkspacedata", xcodeProjectWorkspace);
    findandreplaceInTexfile(xcodeProjectWorkspace, "PROJECTNAME", projectName);

}

void xcodeProject::saveMakefile(){
    std::string makefile = ofFilePath::join(projectDir,"Makefile");
    if(!ofFile(makefile).exists()){
        ofFile::copyFromTo(ofFilePath::join(templatePath, "Makefile"), makefile, true, true);
    }

    std::string configmake = ofFilePath::join(projectDir,"config.make");
    if(!ofFile(configmake).exists()){
        ofFile::copyFromTo(ofFilePath::join(templatePath, "config.make"), configmake, true, true);
    }
}


bool xcodeProject::createProjectFile(){
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

    if( target == "osx" ){
        ofFile::copyFromTo(ofFilePath::join(templatePath,"openFrameworks-Info.plist"),projectDir, true, true);
		
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
		}

    }else{
        ofFile::copyFromTo(ofFilePath::join(templatePath,"ofxiOS-Info.plist"),projectDir, true, true);
        ofFile::copyFromTo(ofFilePath::join(templatePath,"ofxiOS_Prefix.pch"),projectDir, true, true);

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
            
            //this is needed for 0.9.3 / 0.9.4 projects which have iOS media assets in bin/data/
            ofDirectory srcDataDir(ofFilePath::join(templatePath, "bin/data"));
            if( srcDataDir.exists() ){
                baseProject::recursiveCopyContents(srcDataDir, dataDirectory);
            }
        }
        ofDirectory mediaAssetsTemplateDirectory(ofFilePath::join(templatePath, "mediaAssets"));
        ofDirectory mediaAssetsProjectDirectory(ofFilePath::join(projectDir, "mediaAssets"));
        if (!mediaAssetsProjectDirectory.exists()){
            mediaAssetsTemplateDirectory.copyTo(mediaAssetsProjectDirectory.getAbsolutePath(), false, false);
        }
    }

    // this is for xcode 4 scheme issues. but I'm not sure this is right.

    //saveWorkspaceXML();
    saveScheme();
    if(target=="osx"){
    	saveMakefile();
    }

    // make everything relative the right way.
    std::string relRoot = getOFRelPath(ofFilePath::removeTrailingSlash(projectDir));
    if (relRoot != "../../../"){
        std::string relPath2 = relRoot;
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



void xcodeProject::renameProject(){

    pugi::xpath_node_set uuidSet = doc.select_nodes("//string[contains(.,'emptyExample')]");
    for (pugi::xpath_node_set::const_iterator it = uuidSet.begin(); it != uuidSet.end(); ++it){
        pugi::xpath_node node = *it;
        std::string val = it->node().first_child().value();
        findandreplace(val, "emptyExample",  projectName);
        it->node().first_child().set_value(val.c_str());
    }
}


bool xcodeProject::loadProjectFile(){
    std::string fileName = projectDir + projectName + ".xcodeproj/project.pbxproj";
    renameProject();
    pugi::xml_parse_result result = doc.load_file(ofToDataPath(fileName).c_str());

    return result.status==pugi::status_ok;

}



bool xcodeProject::saveProjectFile(){



    // does this belong here?

    renameProject();

    // save the project out:

    std::string fileName = projectDir + projectName + ".xcodeproj/project.pbxproj";
    bool bOk =  doc.save_file(ofToDataPath(fileName).c_str());

    return bOk;

}



bool xcodeProject::findArrayForUUID(std::string UUID, pugi::xml_node & nodeMe){
    char query[255];
    sprintf(query, "//string[contains(.,'%s')]", UUID.c_str());
    pugi::xpath_node_set uuidSet = doc.select_nodes(query);
    for (pugi::xpath_node_set::const_iterator it = uuidSet.begin(); it != uuidSet.end(); ++it){
        pugi::xpath_node node = *it;
        if (strcmp(node.node().parent().name(), "array") == 0){
            nodeMe = node.node().parent();
            return true;
        } else {
        }
    }
    return false;
}




pugi::xml_node xcodeProject::findOrMakeFolderSet(pugi::xml_node nodeToAddTo, std::vector < std::string > & folders, std::string pathForHash){




    char query[255];
    sprintf(query, "//key[contains(.,'%s')]/following-sibling::node()[1]//array/string", nodeToAddTo.previous_sibling().first_child().value());
    pugi::xpath_node_set array = doc.select_nodes(query);

    bool bAnyNodeWithThisName = false;
    pugi::xml_node nodeWithThisName;
    std::string name = folders[0];


    for (pugi::xpath_node_set::const_iterator it = array.begin(); it != array.end(); ++it){

        pugi::xpath_node node = *it;
        //node.node().first_child().print(std::cout);

        // this long thing checks, is this a pbxgroup, and if so, what's it's name.
        // do it once for path and once for name, since ROOT level pbxgroups have a path name. ugh.

        char querypbx[255];
        sprintf(querypbx, "//key[contains(.,'%s')]/following-sibling::node()[1]//string[contains(.,'PBXGroup')]/parent::node()[1]//key[contains(.,'path')]/following-sibling::node()[1]", node.node().first_child().value());
        if (doc.select_single_node(querypbx).node() != NULL){

            if (strcmp(doc.select_single_node(querypbx).node().first_child().value(), folders[0].c_str()) == 0){
                printf("found matching node \n");
                bAnyNodeWithThisName = true;
                nodeWithThisName = doc.select_single_node(querypbx).node().parent();
            }
        }

        sprintf(querypbx, "//key[contains(.,'%s')]/following-sibling::node()[1]//string[contains(.,'PBXGroup')]/parent::node()[1]//key[contains(.,'name')]/following-sibling::node()[1]", node.node().first_child().value());

        if (doc.select_single_node(querypbx).node() != NULL){
            if (strcmp(doc.select_single_node(querypbx).node().first_child().value(), folders[0].c_str()) == 0){
                bAnyNodeWithThisName = true;
                nodeWithThisName = doc.select_single_node(querypbx).node().parent();
            }
        }

    }



    // now, if we have a pbxgroup with the right name, pop this name off the folder set, and keep going.
    // else, let's add a folder set, boom.

    if (bAnyNodeWithThisName == false){

        // make a new UUID
        // todo get the full path here somehow...

        pathForHash += "/" + folders[0];

        std::string UUID = generateUUID(pathForHash);

        // add a new node
        std::string PBXGroupStr = std::string(PBXGroup);
        findandreplace( PBXGroupStr, "GROUPUUID", UUID);
        findandreplace( PBXGroupStr, "GROUPNAME", folders[0]);

        pugi::xml_document pbxDoc;
        pugi::xml_parse_result result = pbxDoc.load_buffer(PBXGroupStr.c_str(), strlen(PBXGroupStr.c_str()));


        nodeWithThisName = doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(pbxDoc.first_child().next_sibling());
        doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(pbxDoc.first_child());

        

        // add to array
        char queryArray[255];
        sprintf(queryArray, "//key[contains(.,'%s')]/following-sibling::node()[1]//array", nodeToAddTo.previous_sibling().first_child().value());
        doc.select_single_node(queryArray).node().append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());
        //array.begin()->node().parent().append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());


    } else {

        pathForHash += "/" + folders[0];
    }


    folders.erase(folders.begin());

    if (folders.size() > 0){
        return findOrMakeFolderSet(nodeWithThisName, folders, pathForHash);
    } else {
        return nodeWithThisName;
    }

}

// todo: frameworks
//
void xcodeProject::addFramework(std::string name, std::string path, std::string folder){
    
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
    
    std::string pbxfileref = std::string(PBXFileReferenceWithoutEncoding);
    
    // make a uuid for the framework file.
    
    std::string UUID = generateUUID( name );

    findandreplace( pbxfileref, "FILEUUID", UUID);
    findandreplace( pbxfileref, "FILENAME", name);
    findandreplace( pbxfileref, "FILEPATH", path);
    findandreplace( pbxfileref, "SOURCE_ROOT", "&lt;group&gt;");
    findandreplace( pbxfileref, "explicitFileType", "lastKnownFileType");
    findandreplace( pbxfileref, "FILETYPE", "wrapper.framework");
    
    pugi::xml_document fileRefDoc;
    pugi::xml_parse_result result = fileRefDoc.load_buffer(pbxfileref.c_str(), strlen(pbxfileref.c_str()));
    
    // insert near the top of the file <plist><dict><dict>
    doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child().next_sibling());   // UUID FIRST
    doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child());                  // DICT SECOND
    
    // files need build refs, here we make 2....

    std::string buildUUID = generateUUID(name + "-build");
    std::string pbxbuildfile = std::string(PBXBuildFile);
    findandreplace( pbxbuildfile, "FILEUUID", UUID);
    findandreplace( pbxbuildfile, "BUILDUUID", buildUUID);
    fileRefDoc.load_buffer(pbxbuildfile.c_str(), strlen(pbxbuildfile.c_str()));
    doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child().next_sibling());   // UUID FIRST
    doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child());                  // DICT SECOND
    
    
    
    // we add one of the build refs to the list of frameworks
    
    pugi::xml_node array;
    
    
  
    
    findArrayForUUID(frameworksUUID, array);    // this is the build array (all build refs get added here)
    array.append_child("string").append_child(pugi::node_pcdata).set_value(buildUUID.c_str());

    // we add the second to a final build phase for copying the framework into app.   we need to make sure we *don't* do this for system frameworks
    
    

    if (folder.size() != 0 && !ofIsStringInString(path, "/System/Library/Frameworks")
        && target != "ios"){
        
        std::string buildUUID2 = generateUUID(name + "-build2");
        pbxbuildfile = std::string(PBXBuildFile);
        findandreplace( pbxbuildfile, "FILEUUID", UUID);
        findandreplace( pbxbuildfile, "BUILDUUID", buildUUID2);
        fileRefDoc.load_buffer(pbxbuildfile.c_str(), strlen(pbxbuildfile.c_str()));
        doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child().next_sibling());   // UUID FIRST
        doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child());                  // DICT SECOND
        
		pugi::xpath_node xpathResult = doc.select_single_node("//string[contains(.,'PBXCopyFilesBuildPhase')]/../array");
        pugi::xml_node node = xpathResult.node();
        node.append_child("string").append_child(pugi::node_pcdata).set_value(buildUUID2.c_str());
    }
    
    // now, we get the path for this framework without the name

    std::string pathWithoutName;
    std::vector < std::string > pathSplit = ofSplitString(path, "/");
    for (int i = 0; i < pathSplit.size()-1; i++){
        if (i != 0) pathWithoutName += "/";
        pathWithoutName += pathSplit[i];
    }
    
    // then, we are going to add this to "FRAMEWORK_SEARCH_PATHS" -- we do this twice, once for debug once for release.
	
    pugi::xpath_node_set frameworkSearchPaths = doc.select_nodes("//key[contains(.,'FRAMEWORK_SEARCH_PATHS')]/following-sibling::node()[1]");
    
    if (frameworkSearchPaths.size() > 0){
        for (pugi::xpath_node_set::const_iterator it = frameworkSearchPaths.begin(); it != frameworkSearchPaths.end(); ++it){
            pugi::xpath_node xpathNode = *it;
            pugi::xml_node  xmlNode = xpathNode.node();
            xmlNode.append_child("string").append_child(pugi::node_pcdata).set_value(pathWithoutName.c_str());
        }
    }
    
    // finally, this is for making folders based on the frameworks position in the addon. so it can appear in the sidebar / file explorer
    
    if (folder.size() > 0 && !ofIsStringInString(folder, "/System/Library/Frameworks")){
        
        std::vector < std::string > folders = ofSplitString(folder, "/", true);
        
        if (folders.size() > 1){
            if (folders[0] == "src"){
                std::string xmlStr = "//key[contains(.,'"+srcUUID+"')]/following-sibling::node()[1]";
                
                folders.erase(folders.begin());
                pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();
                pugi::xml_node nodeToAddTo = findOrMakeFolderSet( node, folders, "src");
                nodeToAddTo.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());
                
            } else if (folders[0] == "addons"){
                std::string xmlStr = "//key[contains(.,'"+addonUUID+"')]/following-sibling::node()[1]";
                
                folders.erase(folders.begin());
                pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();
                pugi::xml_node nodeToAddTo = findOrMakeFolderSet( node, folders, "addons");
                
                nodeToAddTo.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());
                
            } else {
                std::string xmlStr = "//key[contains(.,'"+srcUUID+"')]/following-sibling::node()[1]";
                
                pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();
                
                // I'm not sure the best way to proceed;
                // we should maybe find the rootest level and add it there.
                // TODO: fix this.
            }
        };
        
    } else {
        
        
        pugi::xml_node array;
        std::string xmlStr = "//key[contains(.,'"+srcUUID+"')]/following-sibling::node()[1]";
        pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();
        node.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());
        //nodeToAddTo.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());
        
    }

    
    if (target != "ios" && folder.size() != 0){
        // add it to the linking phases...
        pugi::xml_node arrayBuild;
        findArrayForUUID(frameworksBuildPhaseUUID, arrayBuild);    // this is the build array (all build refs get added here)
        arrayBuild.append_child("string").append_child(pugi::node_pcdata).set_value(buildUUID.c_str());
    }
}




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

    std::string pbxfileref = std::string(PBXFileReference);
    if(ext == "xib"){
        pbxfileref = std::string(PBXFileReferenceXib);
    }
    
    
    std::string UUID = generateUUID(srcFile);   // replace with theo's smarter system.

    std::string name, path;
    splitFromLast(srcFile, "/", path, name);

    findandreplace( pbxfileref, "FILENAME", name);
    findandreplace( pbxfileref, "FILEPATH", srcFile);
    findandreplace( pbxfileref, "FILETYPE", fileKind);
    findandreplace( pbxfileref, "FILEUUID", UUID);

    pugi::xml_document fileRefDoc;
    pugi::xml_parse_result result = fileRefDoc.load_buffer(pbxfileref.c_str(), strlen(pbxfileref.c_str()));

    // insert it at <plist><dict><dict>
    doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child().next_sibling());   // UUID FIRST
    doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child());                  // DICT SECOND

    //-----------------------------------------------------------------
    // (B) BUILD REF
    //-----------------------------------------------------------------

    if (addToBuild || addToBuildResource ){

        buildUUID = generateUUID(srcFile + "-build");
	std::string pbxbuildfile = std::string(PBXBuildFile);
        findandreplace( pbxbuildfile, "FILEUUID", UUID);
        findandreplace( pbxbuildfile, "BUILDUUID", buildUUID);
        
        fileRefDoc.load_buffer(pbxbuildfile.c_str(), strlen(pbxbuildfile.c_str()));
        doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child().next_sibling());   // UUID FIRST
        doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child());                  // DICT SECOND

        // add it to the build array.
        if( addToBuildResource ){
            pugi::xml_node array;
            findArrayForUUID(buildPhaseResourcesUUID, array);    // this is the build array (all build refs get added here)
            array.append_child("string").append_child(pugi::node_pcdata).set_value(buildUUID.c_str());
        }
        if( addToBuild ){
            pugi::xml_node array;
            findArrayForUUID(buildPhaseUUID, array);    // this is the build array (all build refs get added here)
            array.append_child("string").append_child(pugi::node_pcdata).set_value(buildUUID.c_str());

        }
    }

    //-----------------------------------------------------------------
    // (C) resrouces
    //-----------------------------------------------------------------


    if (addToResources == true && resourcesUUID != ""){
		
	std::string resUUID = generateUUID(srcFile + "-build");
	std::string pbxbuildfile = std::string(PBXBuildFile);
        findandreplace( pbxbuildfile, "FILEUUID", UUID);
        findandreplace( pbxbuildfile, "BUILDUUID", resUUID);
        fileRefDoc.load_buffer(pbxbuildfile.c_str(), strlen(pbxbuildfile.c_str()));
        doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child().next_sibling());   // UUID FIRST
        doc.select_single_node("/plist[1]/dict[1]/dict[2]").node().prepend_copy(fileRefDoc.first_child());                  // DICT SECOND

        // add it to the build array.
        pugi::xml_node array;
        findArrayForUUID(resourcesUUID, array);    // this is the build array (all build refs get added here)
        array.append_child("string").append_child(pugi::node_pcdata).set_value(resUUID.c_str());
		
    }


    //-----------------------------------------------------------------
    // (D) folder
    //-----------------------------------------------------------------


    if (bAddFolder == true){

        std::vector < std::string > folders = ofSplitString(folder, "/", true);

        if (folders.size() > 1){
            if (folders[0] == "src"){
                std::string xmlStr = "//key[contains(.,'"+srcUUID+"')]/following-sibling::node()[1]";

                folders.erase(folders.begin());
                pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();
                pugi::xml_node nodeToAddTo = findOrMakeFolderSet( node, folders, "src");
                nodeToAddTo.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());

            } else if (folders[0] == "addons"){
                std::string xmlStr = "//key[contains(.,'"+addonUUID+"')]/following-sibling::node()[1]";

                folders.erase(folders.begin());
                pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();
                pugi::xml_node nodeToAddTo = findOrMakeFolderSet( node, folders, "addons");

                nodeToAddTo.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());

            } else if (folders[0] == "local_addons"){
                std::string xmlStr = "//key[contains(.,'"+localAddonUUID+"')]/following-sibling::node()[1]";

                folders.erase(folders.begin());
                pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();
                pugi::xml_node nodeToAddTo = findOrMakeFolderSet( node, folders, "localAddons");

                nodeToAddTo.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());

            } else {
                std::string xmlStr = "//key[contains(.,'"+srcUUID+"')]/following-sibling::node()[1]";

                pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();

                // I'm not sure the best way to proceed;
                // we should maybe find the rootest level and add it there.
                // TODO: fix this.
            }
        };

    } else {


        pugi::xml_node array;
        std::string xmlStr = "//key[contains(.,'"+srcUUID+"')]/following-sibling::node()[1]";
        pugi::xml_node node = doc.select_single_node(xmlStr.c_str()).node();
        node.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());
        //nodeToAddTo.child("array").append_child("string").append_child(pugi::node_pcdata).set_value(UUID.c_str());

    }

    //saveFile(projectDir + "/" + projectName + ".xcodeproj" + "/project.pbxproj");
}


// todo: these three have very duplicate code... please fix up a bit.

void xcodeProject::addInclude(std::string includeName){




    char query[255];
    sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'HEADER_SEARCH_PATHS')]/following-sibling::node()[1]");
    pugi::xpath_node_set headerArray = doc.select_nodes(query);

    if (headerArray.size() > 0){

        for (pugi::xpath_node_set::const_iterator it = headerArray.begin(); it != headerArray.end(); ++it){
            pugi::xpath_node node = *it;
            //node.node().print(std::cout);
            node.node().append_child("string").append_child(pugi::node_pcdata).set_value(includeName.c_str());
        }

    } else {

        //printf("we don't have HEADER_SEARCH_PATHS, so we're adding them... and calling this function again \n");
        sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'buildSettings')]/following-sibling::node()[1]");
        pugi::xpath_node_set dictArray = doc.select_nodes(query);


        for (pugi::xpath_node_set::const_iterator it = dictArray.begin(); it != dictArray.end(); ++it){
            pugi::xpath_node node = *it;

            //node.node().print(std::cout);
            std::string headerXML = std::string(HeaderSearchPath);
            pugi::xml_document headerDoc;
            pugi::xml_parse_result result = headerDoc.load_buffer(headerXML.c_str(), strlen(headerXML.c_str()));

            // insert it at <plist><dict><dict>
            node.node().prepend_copy(headerDoc.first_child().next_sibling());   // KEY FIRST
            node.node().prepend_copy(headerDoc.first_child());                  // ARRAY SECOND

        }

        // now that we have it, try again...
        addInclude(includeName);
    }

    //saveFile(projectDir + "/" + projectName + ".xcodeproj" + "/project.pbxproj");

}


void xcodeProject::addLibrary(const LibraryBinary & lib){

    char query[255];
    sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'OTHER_LDFLAGS')]/following-sibling::node()[1]");
    pugi::xpath_node_set headerArray = doc.select_nodes(query);


    if (headerArray.size() > 0){
        for (pugi::xpath_node_set::const_iterator it = headerArray.begin(); it != headerArray.end(); ++it){
            pugi::xpath_node node = *it;
            node.node().append_child("string").append_child(pugi::node_pcdata).set_value(lib.path.c_str());
        }

    } else {

        //printf("we don't have OTHER_LDFLAGS, so we're adding them... and calling this function again \n");
        sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'buildSettings')]/following-sibling::node()[1]");

        pugi::xpath_node_set dictArray = doc.select_nodes(query);

        for (pugi::xpath_node_set::const_iterator it = dictArray.begin(); it != dictArray.end(); ++it){
            pugi::xpath_node node = *it;

            //node.node().print(std::cout);
            std::string ldXML = std::string(LDFlags);
            pugi::xml_document ldDoc;
            pugi::xml_parse_result result = ldDoc.load_buffer(ldXML.c_str(), strlen(ldXML.c_str()));

            // insert it at <plist><dict><dict>
            node.node().prepend_copy(ldDoc.first_child().next_sibling());   // KEY FIRST
            node.node().prepend_copy(ldDoc.first_child());                  // ARRAY SECOND

            //node.node().print(std::cout);
        }

        // now that we have it, try again...
        addLibrary(lib);
    }

    //saveFile(projectDir + "/" + projectName + ".xcodeproj" + "/project.pbxproj");
}

void xcodeProject::addLDFLAG(std::string ldflag, LibType libType){

    char query[255];
    sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'OTHER_LDFLAGS')]/following-sibling::node()[1]");
    pugi::xpath_node_set headerArray = doc.select_nodes(query);


    if (headerArray.size() > 0){
        for (pugi::xpath_node_set::const_iterator it = headerArray.begin(); it != headerArray.end(); ++it){
            pugi::xpath_node node = *it;
            node.node().append_child("string").append_child(pugi::node_pcdata).set_value(ldflag.c_str());
        }

    } else {

        //printf("we don't have OTHER_LDFLAGS, so we're adding them... and calling this function again \n");
        sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'buildSettings')]/following-sibling::node()[1]");

        pugi::xpath_node_set dictArray = doc.select_nodes(query);

        for (pugi::xpath_node_set::const_iterator it = dictArray.begin(); it != dictArray.end(); ++it){
            pugi::xpath_node node = *it;

            //node.node().print(std::cout);
            std::string ldXML = std::string(LDFlags);
            pugi::xml_document ldDoc;
            pugi::xml_parse_result result = ldDoc.load_buffer(ldXML.c_str(), strlen(ldXML.c_str()));

            // insert it at <plist><dict><dict>
            node.node().prepend_copy(ldDoc.first_child().next_sibling());   // KEY FIRST
            node.node().prepend_copy(ldDoc.first_child());                  // ARRAY SECOND

            //node.node().print(std::cout);
        }

        // now that we have it, try again...
        addLDFLAG(ldflag);
    }

    //saveFile(projectDir + "/" + projectName + ".xcodeproj" + "/project.pbxproj");
}

void xcodeProject::addCFLAG(std::string cflag, LibType libType){

    char query[255];
    sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'OTHER_CFLAGS')]/following-sibling::node()[1]");
    pugi::xpath_node_set headerArray = doc.select_nodes(query);


    if (headerArray.size() > 0){
        for (pugi::xpath_node_set::const_iterator it = headerArray.begin(); it != headerArray.end(); ++it){
            pugi::xpath_node node = *it;
            node.node().append_child("string").append_child(pugi::node_pcdata).set_value(cflag.c_str());
        }

    } else {

        //printf("we don't have OTHER_LDFLAGS, so we're adding them... and calling this function again \n");
        sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'buildSettings')]/following-sibling::node()[1]");

        pugi::xpath_node_set dictArray = doc.select_nodes(query);

        for (pugi::xpath_node_set::const_iterator it = dictArray.begin(); it != dictArray.end(); ++it){
            pugi::xpath_node node = *it;

            //node.node().print(std::cout);
            std::string ldXML = std::string(CFlags);
            pugi::xml_document ldDoc;
            pugi::xml_parse_result result = ldDoc.load_buffer(ldXML.c_str(), strlen(ldXML.c_str()));

            // insert it at <plist><dict><dict>
            node.node().prepend_copy(ldDoc.first_child().next_sibling());   // KEY FIRST
            node.node().prepend_copy(ldDoc.first_child());                  // ARRAY SECOND

            //node.node().print(std::cout);
        }

        // now that we have it, try again...
        addCFLAG(cflag);
    }

    //saveFile(projectDir + "/" + projectName + ".xcodeproj" + "/project.pbxproj");
}

void xcodeProject::addDefine(std::string define, LibType libType){

	char query[255];
	sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[text()='GCC_PREPROCESSOR_DEFINITIONS']/following-sibling::node()[1]");
	pugi::xpath_node_set headerArray = doc.select_nodes(query);


	if (headerArray.size() > 0){
		for (pugi::xpath_node_set::const_iterator it = headerArray.begin(); it != headerArray.end(); ++it){
			pugi::xpath_node node = *it;
			node.node().append_child("string").append_child(pugi::node_pcdata).set_value(define.c_str());
			//node.node().print(std::cout);
		}

	} else {

		//printf("we don't have GCC_PREPROCESSOR_DEFINITIONS, so we're adding them... and calling this function again \n");
		sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'buildSettings')]/following-sibling::node()[1]");

		pugi::xpath_node_set dictArray = doc.select_nodes(query);

		for (pugi::xpath_node_set::const_iterator it = dictArray.begin(); it != dictArray.end(); ++it){
			pugi::xpath_node node = *it;

			//node.node().print(std::cout);
			std::string definesXML = std::string(Defines);
			pugi::xml_document definesDoc;
			pugi::xml_parse_result result = definesDoc.load_buffer(definesXML.c_str(), strlen(definesXML.c_str()));

			// insert it at <plist><dict><dict>
			node.node().prepend_copy(definesDoc.first_child().next_sibling());   // KEY FIRST
			node.node().prepend_copy(definesDoc.first_child());                  // ARRAY SECOND

			//node.node().print(std::cout);
			//ofLogNotice() << "_____________________________________________________________________________________________________________";
		}

		// now that we have it, try again...
		addDefine(define);
	}

	//saveFile(projectDir + "/" + projectName + ".xcodeproj" + "/project.pbxproj");
}

void xcodeProject::addAfterRule(std::string rule){

	char query[255];
    sprintf(query, "//key[contains(.,'objects')]/following-sibling::node()[1]");
    pugi::xpath_node_set objects = doc.select_nodes(query);


    if (objects.size() > 0){
        for (auto node: objects){
            //node.node().print(std::cout);
            std::string ldXML = std::string(afterRule);
            ofStringReplace(ldXML,"SHELL_SCRIPT",rule);
            pugi::xml_document ldDoc;
            pugi::xml_parse_result result = ldDoc.load_buffer(ldXML.c_str(), strlen(ldXML.c_str()));

            // insert it at <plist><dict><dict>
            node.node().prepend_copy(ldDoc.first_child().next_sibling());   // KEY FIRST
            node.node().prepend_copy(ldDoc.first_child());                  // ARRAY SECOND
            

            pugi::xml_node array;
            findArrayForUUID(buildPhasesUUID, array);    // this is the build array (all build refs get added here)
            array.append_child("string").append_child(pugi::node_pcdata).set_value(afterPhaseUUID.c_str());
            
        }

    }
}

void xcodeProject::addCPPFLAG(std::string cppflag, LibType libType){

    char query[255];
    sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'OTHER_CPLUSPLUSFLAGS')]/following-sibling::node()[1]");
    pugi::xpath_node_set headerArray = doc.select_nodes(query);


    if (headerArray.size() > 0){
        for (pugi::xpath_node_set::const_iterator it = headerArray.begin(); it != headerArray.end(); ++it){
            pugi::xpath_node node = *it;
            node.node().append_child("string").append_child(pugi::node_pcdata).set_value(cppflag.c_str());
        }

    } else {

        //printf("we don't have OTHER_CPLUSPLUSFLAGS, so we're adding them... and calling this function again \n");
        sprintf(query, "//key[contains(.,'baseConfigurationReference')]/parent::node()//key[contains(.,'buildSettings')]/following-sibling::node()[1]");

        pugi::xpath_node_set dictArray = doc.select_nodes(query);

        for (pugi::xpath_node_set::const_iterator it = dictArray.begin(); it != dictArray.end(); ++it){
            pugi::xpath_node node = *it;

            //node.node().print(std::cout);
            std::string ldXML = std::string(CPPFlags);
            pugi::xml_document ldDoc;
            pugi::xml_parse_result result = ldDoc.load_buffer(ldXML.c_str(), strlen(ldXML.c_str()));

            // insert it at <plist><dict><dict>
            node.node().prepend_copy(ldDoc.first_child().next_sibling());   // KEY FIRST
            node.node().prepend_copy(ldDoc.first_child());                  // ARRAY SECOND

            //node.node().print(std::cout);
        }

        // now that we have it, try again...
        addCPPFLAG(cppflag);
    }

    //saveFile(projectDir + "/" + projectName + ".xcodeproj" + "/project.pbxproj");
}

void xcodeProject::addAddon(ofAddon & addon){
	ofLogNotice() << "adding addon " << addon.name;
    for(int i=0;i<(int)addons.size();i++){
		if(addons[i].name==addon.name) return;
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
    for(int i=0;i<(int)addon.srcFiles.size(); i++){
        ofLogVerbose() << "adding addon srcFiles: " << addon.srcFiles[i];
        addSrc(addon.srcFiles[i],addon.filesToFolders[addon.srcFiles[i]]);
    }
	for(int i=0;i<(int)addon.defines.size(); i++){
		ofLogVerbose() << "adding addon defines: " << addon.srcFiles[i];
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
                                                                                            
                                                                                            //
            
    }
    
}
