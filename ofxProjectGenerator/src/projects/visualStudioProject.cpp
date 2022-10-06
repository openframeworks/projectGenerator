


#include "visualStudioProject.h"
#include "Utils.h"

std::string visualStudioProject::LOG_NAME = "visualStudioProjectFile";


bool visualStudioProject::createProjectFile(){

    std::string project = ofFilePath::join(projectDir,projectName + ".vcxproj");
    std::string user = ofFilePath::join(projectDir,projectName + ".vcxproj.user");
    std::string solution = ofFilePath::join(projectDir,projectName + ".sln");
    std::string filters = ofFilePath::join(projectDir, projectName + ".vcxproj.filters");

    ofFile::copyFromTo(ofFilePath::join(templatePath,"emptyExample.vcxproj"),project,false, true);
    ofFile::copyFromTo(ofFilePath::join(templatePath,"emptyExample.vcxproj.user"),user, false, true);
    ofFile::copyFromTo(ofFilePath::join(templatePath,"emptyExample.sln"),solution, false, true);
	ofFile::copyFromTo(ofFilePath::join(templatePath,"emptyExample.vcxproj.filters"),filters, false, true);
	ofFile::copyFromTo(ofFilePath::join(templatePath,"icon.rc"), projectDir + "icon.rc", false, true);

	ofFile filterFile(filters);
	std::string temp = filterFile.readToBuffer();
	pugi::xml_parse_result result = filterXmlDoc.load_string(temp.c_str());
	if (result.status==pugi::status_ok) ofLogVerbose() << "loaded filter ";
	else ofLogVerbose() << "problem loading filter ";

    findandreplaceInTexfile(solution,"emptyExample",projectName);
    findandreplaceInTexfile(user,"emptyExample",projectName);
    findandreplaceInTexfile(project,"emptyExample",projectName);

    std::string relRoot = getOFRelPath(ofFilePath::removeTrailingSlash(projectDir));
    if (relRoot != "../../../"){

	std::string relRootWindows = relRoot;
        // let's make it windows friendly:
        for(int i = 0; i < relRootWindows.length(); i++) {
            if( relRootWindows[i] == '/' )
                relRootWindows[i] = '\\';
        }

        // sln has windows paths:
        findandreplaceInTexfile(solution, "..\\..\\..\\", relRootWindows);

        // vcx has unixy paths:
        //..\..\..\libs
        findandreplaceInTexfile(project, "..\\..\\..\\", relRoot);
    }

    return true;
}


bool visualStudioProject::loadProjectFile(){

    ofFile project(projectDir + projectName + ".vcxproj");
	if(!project.exists()){
		ofLogError(LOG_NAME) << "error loading " << project.path() << " doesn't exist";
		return false;
	}
	pugi::xml_parse_result result = doc.load(project);
    bLoaded = result.status==pugi::status_ok;
    return bLoaded;
}


bool visualStudioProject::saveProjectFile(){

    std::string filters = projectDir + projectName + ".vcxproj.filters";
    filterXmlDoc.save_file(filters.c_str());


    return doc.save_file((projectDir + projectName + ".vcxproj").c_str());
}


void visualStudioProject::appendFilter(std::string folderName){


    fixSlashOrder(folderName);

	 std::string uuid = generateUUID(folderName);

	 std::string tag = "//ItemGroup[Filter]/Filter[@Include=\"" + folderName + "\"]";
	 pugi::xpath_node_set set = filterXmlDoc.select_nodes(tag.c_str());
	 if (set.size() > 0){

		//pugi::xml_node node = set[0].node();
	 } else {


		 pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[Filter]/Filter").node().parent();
		 pugi::xml_node nodeAdded = node.append_child("Filter");
		 nodeAdded.append_attribute("Include").set_value(folderName.c_str());
		 pugi::xml_node nodeAdded2 = nodeAdded.append_child("UniqueIdentifier");

		 uuid.insert(8,"-");
		 uuid.insert(8+4+1,"-");
		 uuid.insert(8+4+4+2,"-");
		 uuid.insert(8+4+4+4+3,"-");

		 //d8376475-7454-4a24-b08a-aac121d3ad6f

		 std::string uuidAltered = "{" + uuid + "}";
		 nodeAdded2.append_child(pugi::node_pcdata).set_value(uuidAltered.c_str());
	 }
}

void visualStudioProject::addSrc(std::string srcFile, std::string folder, SrcType type){

    fixSlashOrder(folder);
    fixSlashOrder(srcFile);

	std::vector < std::string > folderSubNames = ofSplitString(folder, "\\");
	std::string folderName = "";
	for (int i = 0; i < folderSubNames.size(); i++){
		if (i != 0) folderName += "\\";
		folderName += folderSubNames[i];
		appendFilter(folderName);
	}

	if(type==DEFAULT){
		if (ofIsStringInString(srcFile, ".h") || ofIsStringInString(srcFile, ".hpp") || ofIsStringInString(srcFile, ".inl")){
			appendValue(doc, "ClInclude", "Include", srcFile);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[ClInclude]").node();
			pugi::xml_node nodeAdded = node.append_child("ClInclude");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());

		} else if (ofIsStringInString(srcFile, ".glsl") || ofIsStringInString(srcFile, ".vert") || ofIsStringInString(srcFile, ".frag")) {
			// TODO: add to None but there's no None in the original template so this fails
			/*appendValue(doc, "None", "Include", srcFile);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[None]").node();
			pugi::xml_node nodeAdded = node.append_child("None");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());*/

		} else{
			appendValue(doc, "ClCompile", "Include", srcFile);

			pugi::xml_node nodeFilters = filterXmlDoc.select_node("//ItemGroup[ClCompile]").node();
			pugi::xml_node nodeAdded = nodeFilters.append_child("ClCompile");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());
		}
	}else{
    	switch(type){
    	case CPP:{
			appendValue(doc, "ClCompile", "Include", srcFile);

			pugi::xml_node nodeFilters = filterXmlDoc.select_node("//ItemGroup[ClCompile]").node();
			pugi::xml_node nodeAdded = nodeFilters.append_child("ClCompile");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());
			break;
    	}
    	case C:{
			pugi::xml_node node = appendValue(doc, "ClCompile", "Include", srcFile);

			if(!node.child("CompileAs")){
				pugi::xml_node compileAs = node.append_child("CompileAs");
				compileAs.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Debug|x64'");
				compileAs.set_value("Default");

				compileAs = node.append_child("CompileAs");
				compileAs.append_attribute("Condition").set_value("'$(Configuration)|$(Platform)'=='Release|x64'");
				compileAs.set_value("Default");
			}

			pugi::xml_node nodeFilters = filterXmlDoc.select_node("//ItemGroup[ClCompile]").node();
			pugi::xml_node nodeAdded = nodeFilters.append_child("ClCompile");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());
			break;
    	}
    	case HEADER:{
			appendValue(doc, "ClInclude", "Include", srcFile);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[ClInclude]").node();
			pugi::xml_node nodeAdded = node.append_child("ClInclude");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());
			break;
    	}
    	case OBJC:{
    		ofLogError() << "objective c type not supported on vs for " << srcFile;
			break;
    	}
    	default:{
    		ofLogError() << "explicit source type " << type << " not supported yet on osx for " << srcFile;
    		break;
    	}
    	}
	}



}

void visualStudioProject::addInclude(std::string includeName){


    fixSlashOrder(includeName);

    pugi::xpath_node_set source = doc.select_nodes("//ClCompile/AdditionalIncludeDirectories");
    for (pugi::xpath_node_set::const_iterator it = source.begin(); it != source.end(); ++it){
        pugi::xpath_node node = *it;
        std::string includes = node.node().first_child().value();
        std::vector < std::string > strings = ofSplitString(includes, ";");
        bool bAdd = true;
        for (int i = 0; i < (int)strings.size(); i++){
            if (strings[i].compare(includeName) == 0){
                bAdd = false;
            }
        }
        if (bAdd == true){
            strings.push_back(includeName);
            std::string includesNew = unsplitString(strings, ";");
            node.node().first_child().set_value(includesNew.c_str());
        }

    }
    //appendValue(doc, "Add", "directory", includeName);
}

void addLibraryPath(const pugi::xpath_node_set & nodes, std::string libFolder) {
	for (auto & node : nodes) {
		std::string includes = node.node().first_child().value();
		std::vector < std::string > strings = ofSplitString(includes, ";");
		bool bAdd = true;
		for (int i = 0; i < (int)strings.size(); i++) {
			if (strings[i].compare(libFolder) == 0) {
				bAdd = false;
			}
		}
		if (bAdd == true) {
			strings.push_back(libFolder);
			std::string libPathsNew = unsplitString(strings, ";");
			node.node().first_child().set_value(libPathsNew.c_str());
		}
	}
}

void addLibraryName(const pugi::xpath_node_set & nodes, std::string libName) {

	for (auto & node : nodes) {

		std::string includes = node.node().first_child().value();
		std::vector < std::string > strings = ofSplitString(includes, ";");
		bool bAdd = true;
		for (int i = 0; i < (int)strings.size(); i++) {
			if (strings[i].compare(libName) == 0) {
				bAdd = false;
			}
		}

		if (bAdd == true) {
			strings.push_back(libName);
			std::string libsNew = unsplitString(strings, ";");
			node.node().first_child().set_value(libsNew.c_str());
		}
	}
}

void visualStudioProject::addProps(std::string propsFile){
	pugi::xpath_node_set items = doc.select_nodes("//ImportGroup");
	for (int i = 0; i < items.size(); i++) {
		pugi::xml_node additionalOptions;
		items[i].node().append_child("Import").append_attribute("Project").set_value(propsFile.c_str());
	}
}

void visualStudioProject::addLibrary(const LibraryBinary & lib) {
	auto libraryName = lib.path;
	fixSlashOrder(libraryName);

	// ok first, split path and library name.
	size_t found = libraryName.find_last_of("\\");
	std::string libFolder = libraryName.substr(0, found);
	std::string libName = libraryName.substr(found + 1);

	std::string libBaseName;
	std::string libExtension;

	splitFromLast(libName, ".", libBaseName, libExtension);

	// ---------| invariant: libExtension is `lib`

	// paths for libraries
	std::string linkPath;
	if (!lib.target.empty() && !lib.arch.empty()) {
		linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.target + "') and contains(@Condition,'" + lib.arch + "')]/Link/";
	}
	else if (!lib.target.empty()) {
		linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.target + "')]/Link/";
	}
	else if (!lib.arch.empty()) {
		linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.arch + "')]/Link/";
	}
	else {
		linkPath = "//ItemDefinitionGroup/Link/";
	}

	if (!libFolder.empty()) {
		pugi::xpath_node_set addlLibsDir = doc.select_nodes((linkPath + "AdditionalLibraryDirectories").c_str());
		addLibraryPath(addlLibsDir, libFolder);
	}

	pugi::xpath_node_set addlDeps = doc.select_nodes((linkPath + "AdditionalDependencies").c_str());
	addLibraryName(addlDeps, libName);

	ofLogVerbose() << "adding lib path " << libFolder;
	ofLogVerbose() << "adding lib " << libName;

}

void visualStudioProject::addCFLAG(std::string cflag, LibType libType){
	pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	for(int i=0;i<items.size();i++){
		pugi::xml_node additionalOptions;
		bool found=false;
		std::string condition(items[i].node().attribute("Condition").value());
		if (libType == RELEASE_LIB && condition.find("Release") != std::string::npos) {
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}else if(libType==DEBUG_LIB && condition.find("Debug") != std::string::npos){
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}
		if(!found) continue;
		if(!additionalOptions){
			items[i].node().child("ClCompile").append_child("AdditionalOptions").append_child(pugi::node_pcdata).set_value(cflag.c_str());
		}else{
			additionalOptions.set_value((std::string(additionalOptions.value()) + " " + cflag).c_str());
		}
	}

}

void visualStudioProject::addCPPFLAG(std::string cppflag, LibType libType){
	pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	for(int i=0;i<items.size();i++){
		pugi::xml_node additionalOptions;
		bool found=false;
		std::string condition(items[i].node().attribute("Condition").value());
		if(libType==RELEASE_LIB && condition.find("Debug") != std::string::npos){
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}else if(libType==DEBUG_LIB && condition.find("Release") != std::string::npos){
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}
		if(!found) continue;
		if(!additionalOptions){
			items[i].node().child("ClCompile").append_child("AdditionalOptions").append_child(pugi::node_pcdata).set_value(cppflag.c_str());
		}else{
			additionalOptions.set_value((std::string(additionalOptions.value()) + " " + cppflag).c_str());
		}
	}

}

void visualStudioProject::addDefine(std::string define, LibType libType)
{
	pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	for (int i = 0; i<items.size(); i++) {
		pugi::xml_node additionalOptions;
		bool found = false;
		std::string condition(items[i].node().attribute("Condition").value());
		if (libType == RELEASE_LIB && condition.find("Debug") != std::string::npos) {
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}
		else if (libType == DEBUG_LIB && condition.find("Release") != std::string::npos) {
			additionalOptions = items[i].node().child("ClCompile").child("AdditionalOptions");
			found = true;
		}
		if (!found) continue;
		if (!additionalOptions) {
			items[i].node().child("ClCompile").append_child("PreprocessorDefinitions").append_child(pugi::node_pcdata).set_value(define.c_str());
		}
		else {
			additionalOptions.set_value((std::string(additionalOptions.value()) + " " + define).c_str());
		}
	}
}

void visualStudioProject::addAddon(ofAddon & addon){
    for(int i=0;i<(int)addons.size();i++){
		if(addons[i].name==addon.name) return;
	}

    for(int i=0;i<addon.dependencies.size();i++){
        baseProject::addAddon(addon.dependencies[i]);
    }

	ofLogNotice() << "adding addon: " << addon.name;
	addons.push_back(addon);

    for(int i=0;i<(int)addon.includePaths.size();i++){
        ofLogVerbose() << "adding addon include path: " << addon.includePaths[i];
        addInclude(addon.includePaths[i]);
    }

	for (auto & props : addon.propsFiles) {
		ofLogVerbose() << "adding addon props: " << props;
		addProps(props);
	}

    for(auto & lib: addon.libs){
		ofLogVerbose() << "adding addon libs: " << lib.path;
		addLibrary(lib);
    }

    for(int i=0;i<(int)addon.srcFiles.size(); i++){
        ofLogVerbose() << "adding addon srcFiles: " << addon.srcFiles[i];
		if(addon.filesToFolders[addon.srcFiles[i]]=="") addon.filesToFolders[addon.srcFiles[i]]="other";
        addSrc(addon.srcFiles[i],addon.filesToFolders[addon.srcFiles[i]]);
    }

    for(int i=0;i<(int)addon.csrcFiles.size(); i++){
        ofLogVerbose() << "adding addon c srcFiles: " << addon.csrcFiles[i];
		if(addon.filesToFolders[addon.csrcFiles[i]]=="") addon.filesToFolders[addon.csrcFiles[i]]="other";
        addSrc(addon.csrcFiles[i],addon.filesToFolders[addon.csrcFiles[i]],C);
    }

    for(int i=0;i<(int)addon.cppsrcFiles.size(); i++){
        ofLogVerbose() << "adding addon cpp srcFiles: " << addon.cppsrcFiles[i];
		if(addon.filesToFolders[addon.cppsrcFiles[i]]=="") addon.filesToFolders[addon.cppsrcFiles[i]]="other";
        addSrc(addon.cppsrcFiles[i],addon.filesToFolders[addon.cppsrcFiles[i]],C);
    }

    for(int i=0;i<(int)addon.headersrcFiles.size(); i++){
        ofLogVerbose() << "adding addon header srcFiles: " << addon.headersrcFiles[i];
		if(addon.filesToFolders[addon.headersrcFiles[i]]=="") addon.filesToFolders[addon.headersrcFiles[i]]="other";
        addSrc(addon.headersrcFiles[i],addon.filesToFolders[addon.headersrcFiles[i]],C);
    }

    for(int i=0;i<(int)addon.objcsrcFiles.size(); i++){
        ofLogVerbose() << "adding addon objc srcFiles: " << addon.objcsrcFiles[i];
		if(addon.filesToFolders[addon.objcsrcFiles[i]]=="") addon.filesToFolders[addon.objcsrcFiles[i]]="other";
        addSrc(addon.objcsrcFiles[i],addon.filesToFolders[addon.objcsrcFiles[i]],C);
    }

	for(int i=0;i<(int)addon.dllsToCopy.size();i++){
		ofLogVerbose() << "adding addon dlls to bin: " << addon.dllsToCopy[i];
        auto sep = std::filesystem::path("/").make_preferred();
        std::string dll = std::filesystem::absolute(addon.addonPath + sep.string() + addon.dllsToCopy[i]).string();
		ofFile(dll).copyTo(ofFilePath::join(projectDir,"bin/"),false,true);
	}

	for(int i=0;i<(int)addon.cflags.size();i++){
		ofLogVerbose() << "adding addon cflags: " << addon.cflags[i];
		addCFLAG(addon.cflags[i],RELEASE_LIB);
		addCFLAG(addon.cflags[i],DEBUG_LIB);
	}
	
	for(int i=0;i<(int)addon.cppflags.size();i++){
		ofLogVerbose() << "adding addon cppflags: " << addon.cppflags[i];
		addCPPFLAG(addon.cppflags[i],RELEASE_LIB);
		addCPPFLAG(addon.cppflags[i],DEBUG_LIB);
	}

	for (int i = 0; i<(int)addon.defines.size(); i++) {
		ofLogVerbose() << "adding addon define: " << addon.defines[i];
		addDefine(addon.defines[i], RELEASE_LIB);
		addDefine(addon.defines[i], DEBUG_LIB);
	}
}
