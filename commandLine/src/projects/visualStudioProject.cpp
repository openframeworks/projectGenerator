#include "visualStudioProject.h"
#include "Utils.h"
#include "ofUtils.h"
#include <set>

string visualStudioProject::LOG_NAME = "visualStudioProjectFile";

bool visualStudioProject::createProjectFile(){
//	alert("visualStudioProject::createProjectFile");

	ensureDllDirectoriesExist();
	solution = projectDir / (projectName + ".sln");
	if (backupProjectFiles) {
		createBackup(solution, projectDir);
		createBackup({ projectDir / "addons.make" }, projectDir);
		createBackup({ projectDir / "config.make" }, projectDir);
		createBackup({ projectDir / "Makefile" }, projectDir);
		createBackup({ projectDir / (projectName + ".vcxproj") }, projectDir);
	}
	
	// FIXME: this will insert an empty pair
	std::pair <string, string> replacementsForward, replacementsBack;
	if (!fs::equivalent(getOFRoot(), fs::path{ "../../.." })) {
		fs::path root { getOFRoot() };
		string relRootWindows { convertStringToWindowsSeparator(root.string()) + "\\" };
		replacementsForward = { "../../../", relRootWindows };
		replacementsBack = { "..\\..\\..\\", relRootWindows };
	} else {
//		cout << "equivalent to default ../../.." << endl;
	}
	\
	// solution
	copyTemplateFiles.push_back({
		templatePath / "emptyExample.sln",
		projectDir / (projectName + ".sln"),
		{
			{ "emptyExample", projectName },
			replacementsForward,
			replacementsBack
		}
	});
	
	// project
	copyTemplateFiles.push_back({
		templatePath / "emptyExample.vcxproj",
		projectDir / (projectName + ".vcxproj"),
		{
			{ "emptyExample", projectName },
			replacementsForward,
			replacementsBack
		}

	});
	
	// user
	copyTemplateFiles.push_back({
		templatePath / "emptyExample.vcxproj.user",
		projectDir / (projectName + ".vcxproj.user"),
		{
			{ "emptyExample", projectName },
			replacementsForward,
			replacementsBack
		}
	});

	// filters
	copyTemplateFiles.push_back({
		templatePath / "emptyExample.vcxproj.filters",
		projectDir / (projectName + ".vcxproj.filters"),
		{
			{ "emptyExample", projectName },
			replacementsForward,
			replacementsBack
		}
	});

	// icon
	copyTemplateFiles.push_back({
		templatePath / "icon.rc",
		projectDir / "icon.rc",
		{
			{ "emptyExample", projectName },
			replacementsForward,
			replacementsBack
		}
	});

	for (auto & c : copyTemplateFiles) {
			try {
				c.run();
			} catch (const std::exception& e) {
				std::cerr << "Error running copy template files: " << e.what() << std::endl;
				return false;
			}
		}


	 fs::path filters { projectDir / (projectName + ".vcxproj.filters") };

	pugi::xml_parse_result result = filterXmlDoc.load_file(filters.c_str());
	if (result.status==pugi::status_ok) {
		ofLogVerbose() << "loaded filter ";
	} else {
		ofLogVerbose() << "problem loading filter ";
	}
	return true;
}


bool visualStudioProject::loadProjectFile(){
	fs::path projectPath { projectDir / (projectName + ".vcxproj") };
	if (!fs::exists(projectPath)) {
		ofLogError(LOG_NAME) << "error loading " << projectPath << " doesn't exist";
		return false;
	}
	pugi::xml_parse_result result { doc.load_file(projectPath.c_str()) };
	bLoaded = result.status == pugi::status_ok;
//	alert ("visualStudioProject::loadProjectFile() " + projectPath.string() + " : " + ofToString(bLoaded));
	return bLoaded;
}


bool visualStudioProject::saveProjectFile(){

	/*
	 PSEUDOCODE HERE
	 open sln project
	 find position of first "Global" word, put cursor behind
	 add one entry for each additional, fixing slashes, generating new uuid.
	 Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "openframeworksLib", "..\..\..\libs\openFrameworksCompiled\project\vs\openframeworksLib.vcxproj", "{5837595D-ACA9-485C-8E76-729040CE4B0B}"
	 EndProject
	 */
	if (!additionalvcxproj.empty()) {
		string additionalProjects;
		//		string divider = "\r\n";
		string divider = "\n";
		for (auto & a : additionalvcxproj) {
			string name = a.filename().stem().string();
			string aString = a.string();
//			fixSlashOrder(aString);
			string uuid = generateUUID(name);
			additionalProjects +=
			"Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \""+name+"\", \""+aString+"\", \"{"+uuid+"}\"" +
			divider + "EndProject" + divider;
		}
		//		string findString = "Global" + divider;
		string findString = "Global";

		additionalProjects += findString ;

		solution = solution.lexically_normal();
		//		findandreplaceInTexfile(solution, findString, additionalProjects);

		std::ifstream file(solution);
		std::stringstream buffer;
		buffer << file.rdbuf();
		string str = buffer.str();
		file.close();

		std::size_t pos = str.find(findString);
		if (pos != std::string::npos) {
			str.replace(pos, findString.length(), additionalProjects);
		}

		std::ofstream myfile(solution);
		myfile << str;
		myfile.close();


		//		cout << fs::current_path() << endl;
		//		cout << fs::absolute(solution) << endl;
		//		cout << solution.lexically_normal() << endl;
	}



	auto filters { projectDir / (projectName + ".vcxproj.filters") };
	//	alert ("saving filters file : " + filters.string(), 35);
	bool ok1 = filterXmlDoc.save_file(filters.c_str());

	auto vcxFile { projectDir / (projectName + ".vcxproj") };
//	alert ("saving vcxFile file : " + vcxFile.string(), 35);
	bool ok2 = doc.save_file(vcxFile.c_str());

	return ok1 && ok2;
}


void visualStudioProject::appendFilter(string folderName){
//	fixSlashOrder(folderName);
	string uuid { generateUUID(folderName) };
	string tag { "//ItemGroup[Filter]/Filter[@Include=\"" + folderName + "\"]" };
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

		string uuidAltered = "{" + uuid + "}";
		nodeAdded2.append_child(pugi::node_pcdata).set_value(uuidAltered.c_str());
	 }
}


void visualStudioProject::addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type){
//	alert("addSrc file: " + srcFile.string(), 35);
//	alert("addSrc folder: " + folder.string(), 35);

	// I had an empty ClCompile field causing errors
	if (srcFile.empty()) {
		cout << "Empty " << srcFile << " : " << folder << endl;
		return;
	}

    string srcFileString = ofPathToString(srcFile);//.string();
//	fixSlashOrder(srcFileString);
    string folderString = ofPathToString(folder);//.string();
//	fixSlashOrder(folderString);

	// Made to address ofxGstreamer - adds some core files
	if (folderString == "") {
		folderString = "other";
	}

// FIXME: Convert to FS::path
	std::vector < string > folderSubNames = ofSplitString(folderString, "\\");
	string folderName = "";
	for (std::size_t i = 0; i < folderSubNames.size(); i++){
		if (i != 0) folderName += "\\";
		folderName += folderSubNames[i];
		appendFilter(folderName);
	}

	string ext = srcFile.extension().string();
	if(type==DEFAULT){
		if ( ext == ".h" || ext == ".hpp" || ext == ".inl" ) {
			appendValue(doc, "ClInclude", "Include", srcFileString);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[ClInclude]").node();
			pugi::xml_node nodeAdded = node.append_child("ClInclude");
			nodeAdded.append_attribute("Include").set_value(srcFileString.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folderString.c_str());
		} else if (ext == ".vert" || ext == ".frag") {
			// TODO: add to None but there's no None in the original template so this fails
			/*appendValue(doc, "None", "Include", srcFile);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[None]").node();
			pugi::xml_node nodeAdded = node.append_child("None");
			nodeAdded.append_attribute("Include").set_value(srcFile.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folder.c_str());*/

		} else if (ext == ".storyboard" || ext == ".mm" || ext == ".m" || ext == ".swift" || ext == ".java" || ext == ".kotlin") {
			// Do not add files for other platforms
		} else{
			appendValue(doc, "ClCompile", "Include", srcFileString);

			pugi::xml_node nodeFilters = filterXmlDoc.select_node("//ItemGroup[ClCompile]").node();
			pugi::xml_node nodeAdded = nodeFilters.append_child("ClCompile");
			nodeAdded.append_attribute("Include").set_value(srcFileString.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folderString.c_str());
		}
	}else{
		switch(type){
		case CPP:{
			appendValue(doc, "ClCompile", "Include", srcFileString);

			pugi::xml_node nodeFilters = filterXmlDoc.select_node("//ItemGroup[ClCompile]").node();
			pugi::xml_node nodeAdded = nodeFilters.append_child("ClCompile");
			nodeAdded.append_attribute("Include").set_value(srcFileString.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folderString.c_str());
			break;
		}
		case C:{
			pugi::xml_node node = appendValue(doc, "ClCompile", "Include", srcFileString);

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
			nodeAdded.append_attribute("Include").set_value(srcFileString.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folderString.c_str());
			break;
		}
		case HEADER:{
			appendValue(doc, "ClInclude", "Include", srcFileString);

			pugi::xml_node node = filterXmlDoc.select_node("//ItemGroup[ClInclude]").node();
			pugi::xml_node nodeAdded = node.append_child("ClInclude");
			nodeAdded.append_attribute("Include").set_value(srcFileString.c_str());
			nodeAdded.append_child("Filter").append_child(pugi::node_pcdata).set_value(folderString.c_str());
			break;
		}
		case OBJC:{
			ofLogError() << "objective c type not supported on vs for " << srcFileString;
			break;
		}
		default:{
			ofLogError() << "explicit source type " << type << " not supported yet on VSProject for " << srcFileString;
			break;
		}
		}
	}
}

bool exclusiveAppend( string& values, string item, string delimiter = ";") {
	auto strings = ofSplitString(values, delimiter);
	bool bAdd = true;
	for (size_t i = 0; i < strings.size(); i++) {
		if (strings[i].compare(item) == 0) {
			bAdd = false;
			break;
		}
	}
	if (bAdd == true) {
		// strings.emplace_back(item);
		values += delimiter+item;
		//unsplitString(strings, delimiter);
		return true;
	}
	return false;
}

void addToAllNodes(const pugi::xpath_node_set & nodes, string item, string delimiter = ";") {
	for (auto & node : nodes) {
		std::string values = node.node().first_child().value();
		if(exclusiveAppend(values, item, delimiter)){

			node.node().first_child().set_value(values.c_str());
			// if(bPrint) {
			// 	string msg = "Adding To Node: " + string(node.node().first_child().value());
			// 	alert(msg, 35);
			// }
		}
		// std::vector < std::string > strings = ofSplitString(includes, delimiter);
		// bool bAdd = true;
		// for (size_t i = 0; i < strings.size(); i++) {
		// 	if (strings[i].compare(item) == 0) {
		// 		bAdd = false;
		// 		break;
		// 	}
		// }
		// if (bAdd == true) {
		// 	strings.emplace_back(item);
		// 	node.node().first_child().set_value(unsplitString(strings, delimiter).c_str());
		// }
	}
}

void visualStudioProject::addInclude(const fs::path & includeName){
	// alert ("visualStudioProject::addInclude " + includeName.string(), 35);
	// string inc = includeName.string();
	// fixSlashOrder(inc);
	//alert ("visualStudioProject::addInclude " + inc, 35);

	pugi::xpath_node_set source = doc.select_nodes("//ClCompile/AdditionalIncludeDirectories");
	addToAllNodes(source, includeName.string());

// 	for (pugi::xpath_node_set::const_iterator it = source.begin(); it != source.end(); ++it){
// 		pugi::xpath_node node = *it;
// 		std::string includes = node.node().first_child().value();
// 		std::vector < std::string > strings = ofSplitString(includes, ";");
// 		bool bAdd = true;
// 		for (size_t i = 0; i < strings.size(); i++) {
// 			if (strings[i].compare(includeName.string()) == 0){
// 				bAdd = false;
// 				break;
// 			}
// 		}
// 		if (bAdd == true){
// 			strings.emplace_back(includeName.string());
// 			std::string includesNew = unsplitString(strings, ";");
// //			alert ("includesNew " + includesNew);
// 			node.node().first_child().set_value(includesNew.c_str());
// 		}

// 	}
	//appendValue(doc, "Add", "directory", includeName);
}


// void addLibraryName(const pugi::xpath_node_set & nodes, string lib) {
// 	for (auto & node : nodes) {
// 		string includes = node.node().first_child().value();
// 		std::vector < string > strings = ofSplitString(includes, ";");
// 		bool bAdd = true;
// 		for (size_t i = 0; i < strings.size(); i++) {
// 			if (strings[i].compare(lib) == 0) {
// 				bAdd = false;
// 			}
// 		}
// 		if (bAdd == true) {
// 			strings.emplace_back(lib);
// 			string libNew = unsplitString(strings, ";");
// 			node.node().first_child().set_value(libNew.c_str());
// 		}
// 	}
// }

void visualStudioProject::addProps(fs::path propsFile){
//	alert ("visualStudioProject::addProps " + propsFile.string());
    string path = ofPathToString(propsFile);//.string();
//	fixSlashOrder(path);
	pugi::xpath_node_set items = doc.select_nodes("//ImportGroup");
	for (auto & item : items) {
		// FIXME: needed?
		pugi::xml_node additionalOptions;
		item.node().append_child("Import").append_attribute("Project").set_value(path.c_str());
	}
//	auto check = doc.select_nodes("//ImportGroup/Import/Project");
}

void visualStudioProject::addLibrary(const LibraryBinary & lib) {

	auto libraryName = fs::path { lib.path };
	auto libFolder = libraryName.parent_path();
	string libFolderString = libFolder.string();
	// fixSlashOrder(libFolderString);
	auto libName = libraryName.filename();

	// Determine the correct link path based on the target and architecture
	string linkPath;
	if (!lib.target.empty() && !lib.arch.empty()) {
		if (lib.arch == "ARM64") {
			// For ARM64, ensure it does not match ARM64EC
			linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.target + "') and contains(@Condition,'ARM64') and not(contains(@Condition,'ARM64EC'))]/Link/";
		} else {
			// For other architectures
			linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.target + "') and contains(@Condition,'" + lib.arch + "')]/Link/";
		}
	} else if (!lib.target.empty()) {
		linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.target + "')]/Link/";
	} else if (!lib.arch.empty()) {
		if (lib.arch == "ARM64") {
			linkPath = "//ItemDefinitionGroup[contains(@Condition,'ARM64') and not(contains(@Condition,'ARM64EC'))]/Link/";
		} else {
			linkPath = "//ItemDefinitionGroup[contains(@Condition,'" + lib.arch + "')]/Link/";
		}
	} else {
		linkPath = "//ItemDefinitionGroup/Link/";
	}

	// Add library paths and names to the correct ItemDefinitionGroup based on the link path
	if (!libFolderString.empty()) {
		pugi::xpath_node_set addlLibsDir = doc.select_nodes((linkPath + "AdditionalLibraryDirectories").c_str());
		ofLogVerbose() << "adding " << lib.arch << " lib path " << linkPath;
		addToAllNodes(addlLibsDir, libFolderString);
	}

	pugi::xpath_node_set addlDeps = doc.select_nodes((linkPath + "AdditionalDependencies").c_str());
	addToAllNodes(addlDeps, libName.string());

	ofLogVerbose("visualStudioProject::addLibrary") << "adding lib path " << libFolder;
	ofLogVerbose("visualStudioProject::addLibrary") << "adding lib " << libName;
}


void visualStudioProject::addCompileOption(const string& nodeName, const string& value, const string& delimiter, LibType libType){

	string configuration = ((libType == DEBUG_LIB)?"Debug":"Release");
	string nodePath = "//ItemDefinitionGroup[contains(@Condition,'" + configuration + "')]/ClCompile/"+nodeName;
	
	pugi::xpath_node_set source = doc.select_nodes(nodePath.c_str());
	// if(bPrint){
	// 	alert("visualStudioProject::addCompileOption " + nodeName + " val: " + value + " del: " + delimiter, 33 );  
	// 	alert("     nodePath: " + nodePath, 33);
	// }

	addToAllNodes(source, value, delimiter);


// 	pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
// 	for (auto & item : items) {
// 		pugi::xml_node options;
// 		bool found=false;
// 		string condition(item.node().attribute("Condition").value());
// 		ofLogNotice() << "Condition: " << condition ;
// 		if (libType == RELEASE_LIB && condition.find("Release") != string::npos) {
// 			options = item.node().child("ClCompile").child(nodeName.c_str());
// 			found = true;
// 		}else if(libType==DEBUG_LIB && condition.find("Debug") != string::npos){
// 			options = item.node().child("ClCompile").child(nodeName.c_str());
// 			found = true;
// 		}
// 		if(!found) continue;
// 		if(!options){
// 			item.node().child("ClCompile").append_child(nodeName.c_str()).append_child(pugi::node_pcdata).set_value(value.c_str());
// 		}else{
// 			string option_values = options.value();
// 			if(exclusiveAppend(option_values, value, delimiter)){
// 				alert("adding option: " + option_values, 34);
// 				options.set_value(option_values.c_str());
// 			}
// //			options.set_value((string(options.value()) + delimiter + value).c_str());
// 		}
// 		// addToNode(options, value, delimiter);
// 		doc.print(std::cout);
// 	}
}

void visualStudioProject::addCFLAG(const string& cflag, LibType libType){
	addCompileOption("AdditionalOptions", cflag, " ", libType);
	// pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	// // FIXME: iterator
	// for (auto & item : items) {
	// 	pugi::xml_node additionalOptions;
	// 	bool found=false;
	// 	string condition(item.node().attribute("Condition").value());
	// 	if (libType == RELEASE_LIB && condition.find("Release") != string::npos) {
	// 		additionalOptions = item.node().child("ClCompile").child("AdditionalOptions");
	// 		found = true;
	// 	}else if(libType==DEBUG_LIB && condition.find("Debug") != string::npos){
	// 		additionalOptions = item.node().child("ClCompile").child("AdditionalOptions");
	// 		found = true;
	// 	}
	// 	if(!found) continue;
	// 	if(!additionalOptions){
	// 		item.node().child("ClCompile").append_child("AdditionalOptions").append_child(pugi::node_pcdata).set_value(cflag.c_str());
	// 	}else{
	// 		additionalOptions.set_value((string(additionalOptions.value()) + " " + cflag).c_str());
	// 	}
	// }
}


void visualStudioProject::addCPPFLAG(const string& cppflag, LibType libType){
		addCompileOption("AdditionalOptions", cppflag, " ", libType);
	// pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	// for (auto & item : items) {
	// 	pugi::xml_node additionalOptions;
	// 	bool found=false;
	// 	string condition(item.node().attribute("Condition").value());
	// 	if(libType==RELEASE_LIB && condition.find("Release") != string::npos){
	// 		additionalOptions = item.node().child("ClCompile").child("AdditionalOptions");
	// 		found = true;
	// 	}
	// 	else if(libType==DEBUG_LIB && condition.find("Debug") != string::npos) {
	// 		additionalOptions = item.node().child("ClCompile").child("AdditionalOptions");
	// 		found = true;
	// 	}
	// 	if(!found) continue;
	// 	if(!additionalOptions){
	// 		item.node().child("ClCompile").append_child("AdditionalOptions").append_child(pugi::node_pcdata).set_value(cppflag.c_str());
	// 	}else{
	// 		additionalOptions.set_value((string(additionalOptions.value()) + " " + cppflag).c_str());
	// 	}
	// }
}


void visualStudioProject::addDefine(const string& define, LibType libType) {
	addCompileOption("PreprocessorDefinitions", define, ";", libType);
	// pugi::xpath_node_set items = doc.select_nodes("//ItemDefinitionGroup");
	// for (auto & item : items) {
	// 	pugi::xml_node preprocessorDefinitions;
	// 	bool found = false;
	// 	string condition(item.node().attribute("Condition").value());
	// 	if (libType == RELEASE_LIB && condition.find("Release") != string::npos) {
	// 		preprocessorDefinitions = item.node().child("ClCompile").child("PreprocessorDefinitions");
	// 		found = true;
	// 	}
	// 	else if (libType == DEBUG_LIB && condition.find("Debug") != string::npos) {
	// 		preprocessorDefinitions = item.node().child("ClCompile").child("PreprocessorDefinitions");
	// 		found = true;
	// 	}
	// 	if (!found) continue;
	// 	if (!preprocessorDefinitions) {
	// 		item.node().child("ClCompile").append_child("AdditionalOptions").append_child(pugi::node_pcdata).set_value(define.c_str());
	// 	}
	// 	else {
	// 		additionalOptions.set_value((string(additionalOptions.value()) + " " + define).c_str());
	// 	}
	// }
}

void visualStudioProject::ensureDllDirectoriesExist() {
	std::vector<fs::path> dirs { { "dll/x64" }, { "dll/ARM64" }, { "dll/ARM64EC" } };
	for (const auto & dir : dirs) {
		fs::path dirPath = { projectDir / dir };
		dirPath = normalizePath(dirPath);
		if (!fs::exists(dirPath)) {
			ofLogVerbose() << "adding dll folder: [" << dirPath.string() << "]";
			try {
			   fs::create_directories(dirPath);
			} catch (const std::exception& e) {
			   std::cerr << "Error creating directories: " << e.what() << std::endl;
			}
		}
	}
}



// void visualStudioProject::addAddon(ofAddon &addon) {

void visualStudioProject::addAddonBegin(const ofAddon& addon){
	// Log the addition of the addon
	ofLogVerbose("visualStudioProject::") << "Adding addon: [" << addon.name << "]";
	// Handle additional vcxproj files in the addon
	fs::path additionalFolder = addon.addonPath / (addon.name + "Lib");
	if (fs::exists(additionalFolder)) {
		for (const auto &entry : fs::directory_iterator(additionalFolder)) {
			auto f = entry.path();
			if (f.extension() == ".vcxproj") {
				additionalvcxproj.emplace_back(f);
			}
		}
	}
}
    

void visualStudioProject::addAddonIncludePaths(const ofAddon& addon) {

	std::set<fs::path> uniqueIncludeDirs;
	for (const auto &dir : addon.includePaths) {
		fs::path normalizedDir = normalizePath(dir);
		std::string dirStr = normalizedDir.string();
		// this dont work
		if (dirStr.find("lib\\vs") == std::string::npos &&
			dirStr.find("\\license") == std::string::npos &&
			dirStr.find("lib\\\\vs") == std::string::npos &&
				dirStr.find("lib\\AndroidJNI") == std::string::npos &&
				dirStr.find("\\bin\\") == std::string::npos) {
			uniqueIncludeDirs.insert(normalizedDir);
		} else {
			ofLogVerbose() << "include dir - not adding vs: [" << dir.string() << "]";
		}
	}

	for (const auto &dir : uniqueIncludeDirs) {
		if( (dir.string().size() && dir.string()[0] == '$')){
			addInclude(dir.string());
		} else{
			fs::path normalizedDir = normalizePath(dir);
			if (containsSourceFiles(normalizedDir)) {
				normalizedDir = makeRelative(projectDir, dir);
				ofLogVerbose() << "[vsproject]-uniqueIncludeDirs] contains src - Adding dir:: [" << normalizedDir.string() << "]";
				addInclude(normalizedDir);
			} else {
				ofLogVerbose() << "[vsproject]-uniqueIncludeDirs] no src - not adding [" << normalizedDir.string() << "]";
			}
		}
	}
}
// void visualStudioProject::addAddonLibs(const ofAddon& addon) {
// 	// Add libraries from the addon
// 	for (auto &lib : addon.libs) {
// 		fs::path normalizedDir = makeRelative(projectDir, lib.path);
// 		lib.path = normalizedDir;
// 		ofLogVerbose() << "Adding addon library: [" << lib.path.string() << "]";
// 		addLibrary(lib);
// 	}
// }
void visualStudioProject::addAddonCflags(const ofAddon& addon) {
	for (auto &a : addon.cflags) {
		ofLogVerbose() << "Adding addon CFLAG: [" << a << "]";
		addCFLAG(a, RELEASE_LIB);
		addCFLAG(a, DEBUG_LIB);
	}
}
void visualStudioProject::addAddonCppflags(const ofAddon& addon) {
	for (auto &a : addon.cppflags) {
		ofLogVerbose() << "Adding addon CPPFLAG: [" << a << "]";
		addCPPFLAG(a, RELEASE_LIB);
		addCPPFLAG(a, DEBUG_LIB);
	}
}
// void visualStudioProject::addAddonLdflags(const ofAddon& addon) {

// }

void visualStudioProject::addSrcFiles(ofAddon& addon, const vector<fs::path> &filepaths, SrcType type, bool bFindInFilesToFolder){
	for (auto &s : filepaths) {
		
		if (bFindInFilesToFolder && (addon.filesToFolders.find(s) == addon.filesToFolders.end())) {
			addon.filesToFolders[s] = fs::path{""};
		}
		ofLogVerbose("visualStudioProject::addSrcFiles") << "Adding addon " << toString(type) << " source file: [" << s.string() << "] folder:[" << addon.filesToFolders[s].string() << "]";
		addSrc(s, addon.filesToFolders[s]);
	}
}

// void visualStudioProject::addAddonSrcFiles(ofAddon& addon) {
// 	addSrcFiles(addon, addon.srcFiles, DEFAULT);
// }
// void visualStudioProject::addAddonCsrcFiles(const ofAddon& addon) {
// 	addSrcFiles(addon, addon.csrcFiles, C);
// }

// void visualStudioProject::addAddonCppsrcFiles(const ofAddon& addon) {
// 	addSrcFiles(addon, addon.cppsrcFiles, CPP);
// }
// void visualStudioProject::addAddonObjcsrcFiles(const ofAddon& addon) {
// 	addSrcFiles(addon, addon.objcsrcFiles, OBJC);
// }
// void visualStudioProject::addAddonHeadersrcFiles(const ofAddon& addon) {
// 	addSrcFiles(addon, addon.headersrcFiles, HEADER);
// }

void visualStudioProject::addAddonDefines(const ofAddon& addon){
	for (auto &a : addon.defines) {
		ofLogVerbose() << "Adding addon define: [" << a << "]";
		addDefine(a, RELEASE_LIB);
		addDefine(a, DEBUG_LIB);
	}
}


void visualStudioProject::addAddonProps(const ofAddon& addon){
	// Add props files from the addon
	for (auto &props : addon.propsFiles) {
		fs::path normalizedDir = makeRelative(projectDir, props);
		ofLogVerbose() << "Adding addon props: [" << normalizedDir.string() << "] folder:[" << addon.filesToFolders.at(props).string() << "]";
		addProps(normalizedDir);
	}
}

