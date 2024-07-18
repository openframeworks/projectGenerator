/*
 * ofAddon.cpp
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#include "ofUtils.h"
#include "ofAddon.h"
#include "Utils.h"
#include <list>
#include <regex>

vector<string> splitStringOnceByLeft(const string &source, const string &delimiter) {
	size_t pos = source.find(delimiter);
	vector<string> res;
	if(pos == string::npos) {
		res.emplace_back(source);
		return res;
	}

	res.emplace_back(source.substr(0, pos));
	res.emplace_back(source.substr(pos + delimiter.length()));
	return res;
}

ofAddon::ofAddon(){
	isLocalAddon = false;
	pathToProject = ".";
	pathToOF = "../../../";
}

bool ofAddon::checkCorrectPlatform(const string & state) {
	if (state == "meta" || state == "common") {
		return true;
	}
	if (std::find(parseStates.begin(), parseStates.end(), state) != parseStates.end()) {
		if (platform == state) {
			return true;
		}
	}
	return false;
//	return std::find(parseStates.begin(),
//					 parseStates.end(),
//					 state) != parseStates.end();
}


bool ofAddon::checkCorrectVariable(const string & variable, const string & state){
	if (state == "meta") {
		return std::find(AddonMetaVariables.begin(),
						 AddonMetaVariables.end(),
						 variable) != AddonMetaVariables.end();
	}
	else if (state == "osx") {
		return std::find(AddonProjectVariables.begin(),
						 AddonProjectVariables.end(),
						 variable) != AddonProjectVariables.end();
	} else {
		return checkCorrectPlatform(state);
	}
}

void ofAddon::addReplaceString(string & variable, string value, bool addToVariable){
	if(addToVariable) variable += value;
	else variable = value;
}

void ofAddon::addReplaceStringVector(std::vector<string> & variable, string value, string prefix, bool addToVariable){
	// alert("addReplaceStringVector val=" + value + " : prefix=" + prefix, (value == prefix) ? 33 : 32);

  if (value == prefix) return;

	vector<string> values;
	if(value.find("\"")!=string::npos){
		values = ofSplitString(value,"\"",true,true);
	}else{
		values = ofSplitString(value," ",true,true);
	}

	if(!addToVariable) variable.clear();
	//value : -F$(OF_ROOT)/addons/ofxSyphon/libs/Syphon/lib/osx/

	//	std::regex findVar("(?<=\\$\().+(?=\\))");
	//	std::regex findVar("\\$\\(.+\\)");
	//	(\$\()(.+)(\)) // now three capture groups here. we use only the second
	std::regex findVar("(\\$\\()(.+)(\\))");
	for(int i=0;i<(int)values.size();i++){
		if(values[i]!=""){
			std::smatch varMatch;
			if(std::regex_search(values[i], varMatch, findVar)) {
				if (varMatch.size() > 2) {
					string varName = varMatch[2].str();
					string varValue;
					if(varName == "OF_ROOT"){
						varValue = ofPathToString(pathToOF);
					}else if(ofGetEnv(varName.c_str()).empty()){
						varValue = ofGetEnv(varName.c_str());
					}
					ofStringReplace(values[i],"$("+varName+")",varValue);
					ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << values[i] << std::endl;
				}
			}

			if(prefix=="" || values[i].find(ofPathToString(pathToOF))==0 || fs::path{values[i]}.is_absolute()) {
				variable.emplace_back(values[i]);
			} else {
				fs::path p = fs::path{ prefix } / values[i];
				variable.emplace_back(ofPathToString(p));
			}
		}
	}
}

void ofAddon::addReplaceStringVector(std::vector<fs::path> & variable, string value, string prefix, bool addToVariable){
	// alert("addReplaceStringVector val=" + value + " : prefix=" + prefix, (value == prefix) ? 33 : 32);

  if (value == prefix) return;

	vector<string> values;
	if(value.find("\"")!=string::npos){
		values = ofSplitString(value,"\"",true,true);
	}else{
		values = ofSplitString(value," ",true,true);
	}

	if(!addToVariable) variable.clear();
	//value : -F$(OF_ROOT)/addons/ofxSyphon/libs/Syphon/lib/osx/

	//	std::regex findVar("(?<=\\$\().+(?=\\))");
	//	std::regex findVar("\\$\\(.+\\)");
	//	(\$\()(.+)(\)) // now three capture groups here. we use only the second
	std::regex findVar("(\\$\\()(.+)(\\))");
	for(int i=0;i<(int)values.size();i++){
		if(values[i]!=""){
			std::smatch varMatch;
			if(std::regex_search(values[i], varMatch, findVar)) {
				if (varMatch.size() > 2) {
					string varName = varMatch[2].str();
					string varValue;
					if(varName == "OF_ROOT"){
						varValue = ofPathToString(pathToOF);
					}else if(ofGetEnv(varName.c_str()).empty()){
						varValue = ofGetEnv(varName.c_str());
					}
					ofStringReplace(values[i],"$("+varName+")",varValue);
					ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << values[i] << std::endl;
				}
			}

			if(prefix=="" || values[i].find(ofPathToString(pathToOF))==0 || fs::path{values[i]}.is_absolute()) {
				variable.emplace_back(fs::path{values[i]});
			} else {
				fs::path p = fs::path{ prefix } / values[i];
				variable.emplace_back(p);
			}
		}
	}
}

void ofAddon::addReplaceStringVector(vector<LibraryBinary> & variable, string value, string prefix, bool addToVariable) {
	vector<string> values;
	if (value.find("\"") != string::npos) {
		values = ofSplitString(value, "\"", true, true);
	}
	else {
		values = ofSplitString(value, " ", true, true);
	}

	if (!addToVariable) variable.clear();
	std::regex findVar("(\\$\\()(.+)(\\))");

	for (auto & v : values) {
		if (v != "") {
			std::smatch varMatch;
			if(std::regex_search(v, varMatch, findVar)) {
				if (varMatch.size() > 2) {
					string varName = varMatch[2].str();
					string varValue;
					if(varName == "OF_ROOT"){
						varValue = ofPathToString(pathToOF);
					}else if(ofGetEnv(varName.c_str()).empty()){
						varValue = ofGetEnv(varName.c_str());
					}
					ofStringReplace(v,"$("+varName+")",varValue);
					ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << v << std::endl;
				}
			}


			if (prefix == "" || v.find(ofPathToString(pathToOF)) == 0 || fs::path{v}.is_absolute()) {
				variable.push_back( { v, "", "" } );
			} else {
				fs::path p = fs::path { prefix } / v;
				variable.push_back( { ofPathToString(p), "", "" } );
			}
		}
	}
}

void ofAddon::parseVariableValue(const string & variable, const string & value, bool addToValue, const string & line, int lineNum){
	

	if (variable == "ADDON_NAME"){
		if (value != name){
			ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
						<< "line " << lineNum << ": " << line << "\n\t\t"
						<< "addon name in filesystem " << name << " doesn't match with addon_config.mk " << value;
		}
		return;
	}

	fs::path addonRelPath = isLocalAddon ? addonPath : (pathToOF / "addons" / name);

	if (variable == "ADDON_ADDITIONAL_LIBS") {
		additionalLibsFolder.emplace_back(value);
		return;
	}
	
	else if (variable == "ADDON_DESCRIPTION") {
		addReplaceString(description, value, addToValue);
		return;
	}

	else if(variable == "ADDON_AUTHOR"){
		addReplaceString(author,value,addToValue);
		return;
	}

	else if(variable == "ADDON_TAGS"){
		addReplaceStringVector(tags,value,"",addToValue);
		return;
	}

	else if(variable == "ADDON_URL"){
		addReplaceString(url,value,addToValue);
		return;
	}

	else if(variable == "ADDON_DEPENDENCIES"){
		addReplaceStringVector(dependencies,value,"",addToValue);
	}

	else if(variable == "ADDON_INCLUDES"){
//		if (!addToValue) {
//			alert ("CLEAR " + variable, 36);
//			alert ("value " + value, 36);
//		}
//		cout << includePaths.size() << endl;
		addReplaceStringVector(includePaths, value, addonRelPath, addToValue);
//		cout << includePaths.size() << endl;
//		cout << "----" << endl;
	}

	else if(variable == ADDON_CFLAGS){
		addReplaceStringVector(cflags,value,"",addToValue);
	}

	else if(variable == ADDON_CPPFLAGS){
		addReplaceStringVector(cppflags,value,"",addToValue);
	}

	else if(variable == ADDON_LDFLAGS){
		addReplaceStringVector(ldflags,value,"",addToValue);
	}

	else if(variable == ADDON_LIBS){
		addReplaceStringVector(libs, value, addonRelPath, addToValue);
	}

	else if(variable == ADDON_DLLS_TO_COPY){
		addReplaceStringVector(dllsToCopy,value,"",addToValue);
	}

	else if(variable == ADDON_PKG_CONFIG_LIBRARIES){
		addReplaceStringVector(pkgConfigLibs,value,"",addToValue);
	}

	else if(variable == ADDON_FRAMEWORKS){
		addReplaceStringVector(frameworks,value,"",addToValue);
	}

	else if (variable == ADDON_XCFRAMEWORKS) {
		addReplaceStringVector(xcframeworks, value, "", addToValue);
	}

	else if(variable == ADDON_SOURCES){
		addReplaceStringVector(srcFiles, value, addonRelPath ,addToValue);
	}

	else if(variable == ADDON_C_SOURCES){
		addReplaceStringVector(csrcFiles, value, addonRelPath, addToValue);
	}

	else if(variable == ADDON_CPP_SOURCES){
		addReplaceStringVector(cppsrcFiles, value, addonRelPath ,addToValue);
	}

	else if(variable == ADDON_HEADER_SOURCES){
		addReplaceStringVector(headersrcFiles, value, addonRelPath, addToValue);
	}

	else if(variable == ADDON_OBJC_SOURCES){
		addReplaceStringVector(objcsrcFiles, value, addonRelPath, addToValue);
	}

	else if(variable == ADDON_DATA){
		addReplaceStringVector(data,value,"",addToValue);
	}

	else if(variable == ADDON_LIBS_EXCLUDE){
		addReplaceStringVector(excludeLibs,value,"",addToValue);
	}
    
    else if(variable == ADDON_LIBS_DIR){
        addReplaceStringVector(libsPaths,value,"",addToValue);
    }

	else if(variable == ADDON_SOURCES_EXCLUDE){
		addReplaceStringVector(excludeSources,value,"",addToValue);
	}

	else if(variable == ADDON_INCLUDES_EXCLUDE){
		addReplaceStringVector(excludeIncludes,value,"",addToValue);
	}

	else if (variable == ADDON_FRAMEWORKS_EXCLUDE) {
		addReplaceStringVector(excludeFrameworks, value, "", addToValue);
	}

	else if (variable == ADDON_DEFINES) {
		addReplaceStringVector(defines, value, "", addToValue);
	}
}

void ofAddon::exclude(vector<string> & variables, vector<string> exclusions){
	for(auto & exclusion: exclusions){
		ofStringReplace(exclusion,"\\","/");
		ofStringReplace(exclusion,".","\\.");
		ofStringReplace(exclusion,"%",".*");
		exclusion =".*"+ exclusion;
//		alert ("ofAddon::exclude " +exclusion, 31);

		std::regex findVar(exclusion);
		std::smatch varMatch;
//		alert ("vars size " + ofToString(variables.size()));
//		for (auto & v : variables) {
//			alert ("\t" + v, 32);
//		}
		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const string & variable){
			auto forwardSlashedVariable = variable;
			ofStringReplace(forwardSlashedVariable, "\\", "/");
//			bool exclude = std::regex_search(forwardSlashedVariable, varMatch, findVar);
//			if (exclude) {
//				alert ("variable removed : " + variable, 31);
//			}
			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
		}), variables.end());
		
//		alert ("vars size " + ofToString(variables.size()));
//		for (auto & v : variables) {
//			alert ("\t" + v, 32);
//		}
	}
}

void ofAddon::exclude(vector<fs::path> & variables, vector<string> exclusions){
	for(auto & exclusion: exclusions){
		ofStringReplace(exclusion,"\\","/");
		ofStringReplace(exclusion,".","\\.");
		ofStringReplace(exclusion,"%",".*");
		exclusion =".*"+ exclusion;
//		alert ("ofAddon::exclude " +exclusion, 31);

		std::regex findVar(exclusion);
		std::smatch varMatch;
//		alert ("vars size " + ofToString(variables.size()));
//		for (auto & v : variables) {
//			alert ("\t" + v, 32);
//		}
		
		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const string & variable){
			auto forwardSlashedVariable = variable;
			ofStringReplace(forwardSlashedVariable, "\\", "/");
//			bool exclude = std::regex_search(forwardSlashedVariable, varMatch, findVar);
//			if (exclude) {
//				alert ("variable removed : " + variable, 31);
//			}
			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
		}), variables.end());
		
//		alert ("vars size " + ofToString(variables.size()));
//		for (auto & v : variables) {
//			alert ("\t" + v, 32);
//		}
	}
}

void ofAddon::exclude(vector<LibraryBinary> & variables, vector<string> exclusions) {
	for(auto & exclusion: exclusions){
		ofStringReplace(exclusion,"\\","/");
		ofStringReplace(exclusion,".","\\.");
		ofStringReplace(exclusion,"%",".*");
		exclusion =".*"+ exclusion;

		std::regex findVar(exclusion);
		std::smatch varMatch;
		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const LibraryBinary & variable){
			auto forwardSlashedVariable = variable.path.string();
			ofStringReplace(forwardSlashedVariable, "\\", "/");
			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
		}), variables.end());
	}
}


void ofAddon::preParseConfig(){
	//	alert ("ofAddon::parseConfig " + addonPath.string(), 33);
	fs::path fileName = isLocalAddon ?
		(pathToProject / addonPath / "addon_config.mk") :
		(addonPath / "addon_config.mk")
	;

	if (!fs::exists(fileName)) {
//		ofLogError() << "ofAddon::parseConfig() " << fileName << " not found " << ofPathToString(fileName);
		return;
	}
	
	for (auto & line : fileToStrings(fileName)) {
		line = ofTrim(line);
		
		if (line[0]=='#' || line == "") {
			continue;
		} // discard comments


		// found section?
		if (line.back() == ':'){
			ofStringReplace(line, ":", "");
			currentParseState = line;
			
			if (std::find(parseStates.begin(), parseStates.end(), currentParseState) == parseStates.end()) {
				ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
//								<< "line " << lineNum << ": " << originalLine << "\n\t\t"
								<< "sectionName " << currentParseState << " not recognized";
			}
			continue;
		}
		
		// found Variable
		if (line.find("=") != string::npos){
//			bool addToValue = false;
			string variable, value;
			vector<string> varValue;
			if (line.find("+=") != string::npos) {
//				addToValue = true;
				varValue = ofSplitString(line, "+=");
			} else {
				varValue = ofSplitString(line, "=");
			}
			variable = ofTrim(varValue[0]);
			value = ofTrim(varValue[1]);

			// FIXME: This seems to be meaningless
			if(!checkCorrectPlatform(currentParseState)){
				continue;
			}

			if(!checkCorrectVariable(variable, currentParseState)){
				ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
//								<< "line " << lineNum << ": " << originalLine << "\n\t\t"
								<< "variable " << variable << " not recognized for section " << currentParseState;
				continue;
			}
			
			if (variable == "ADDON_ADDITIONAL_LIBS") {
				additionalLibsFolder.emplace_back(value);
//				return;
			}
//			parseVariableValue(variable, value, addToValue, originalLine, lineNum);
		}
	}
}

void ofAddon::parseConfig(){
//	alert ("ofAddon::parseConfig " + addonPath.string(), 33);
	fs::path fileName = isLocalAddon ?
		(pathToProject / addonPath / "addon_config.mk") :
		(addonPath / "addon_config.mk")
	;

	if (!fs::exists(fileName)) {
//		ofLogError() << "ofAddon::parseConfig() " << fileName << " not found " << ofPathToString(fileName);
		return;
	}

	int lineNum = 0;

	for (auto & originalLine : fileToStrings(fileName)) {
		lineNum++;
		string line = originalLine;
		ofStringReplace(line,"\r","");
		ofStringReplace(line,"\n","");
		line = ofTrim(line);

		// discard comments
		if(line[0]=='#' || line == ""){
			continue;
		}

		// found section?
		if(line[line.size()-1]==':'){
			ofStringReplace(line,":","");
			// FIXME: Remove
			currentParseState = line;
			
			if (std::find(parseStates.begin(), parseStates.end(), currentParseState) == parseStates.end()) {
				ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
								<< "line " << lineNum << ": " << originalLine << "\n\t\t"
								<< "sectionName " << currentParseState << " not recognized";
			}
			continue;
		}

		// found Variable
		if(line.find("=")!=string::npos){
			bool addToValue = false;
			string variable, value;
			vector<string> varValue;
			if(line.find("+=")!=string::npos){
				addToValue = true;
				varValue = splitStringOnceByLeft(line,"+=");
			}else{
				addToValue = false;
				varValue = splitStringOnceByLeft(line,"=");
			}
			variable = ofTrim(varValue[0]);
			value = ofTrim(varValue[1]);

			// FIXME: This seems to be meaningless
			if(!checkCorrectPlatform(currentParseState)){
				continue;
			}

			if(!checkCorrectVariable(variable, currentParseState)){
				ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
								<< "line " << lineNum << ": " << originalLine << "\n\t\t"
								<< "variable " << variable << " not recognized for section " << currentParseState;
				continue;
			}
			
			parseVariableValue(variable, value, addToValue, originalLine, lineNum);
		}
	}
}

void ofAddon::parseLibsPath(const fs::path & libsPath, const fs::path & parentFolder) {
//	alert ("parseLibsPath " + libsPath.string(), 35);
	if (!fs::exists(libsPath)) {
//		alert("file not found " + libsPath.string(), 35);
		return;
	}

	getLibsRecursively(libsPath, libFiles, libs, platform);
	if (platform == "osx"  ||
        platform == "ios"  ||
        platform == "tvos" ||
        platform == "macos"){
            getFrameworksRecursively(libsPath, frameworks, platform);
            getXCFrameworksRecursively(libsPath, xcframeworks, platform);
	}
	
	if (platform == "vs" || platform == "msys2"
		   || platform == "vscode"
		   || platform == "linux"
		   || platform == "linux64"
		   || platform == "linuxarmv6l"
		   || platform == "linuxarmv7l"
		   || platform == "linuxaarch64"
	   ) {
            getDllsRecursively(libsPath, dllsToCopy, platform);
	}


	// TODO: this is not needed even if it is local addon but project is outside OF root path
	// Absolute paths will be used in this case too.
	// Maybe it is the same situation for all others fixPath occurences?
	if (!isLocalAddon) {
		for (auto & l : libs) {
//			alert("fixpath before " + l.path);
			l.path = ofPathToString(fixPath(l.path));
//			alert("fixpath after  " + l.path);
		}
	}

	for (auto & s : libFiles) {
		fs::path folder;
		if (isLocalAddon) {
			folder = fs::path { "local_addons" } / fs::relative(s.parent_path(), parentFolder);
		} else {
			folder = fs::relative(s.parent_path(), getOFRoot());
			s = fixPath(s);
		}
		string f { ofPathToString(s) };
		srcFiles.emplace_back(f);
		filesToFolders[f] = ofPathToString(folder);
	}

	// so addons will never be system.
	for (const auto & f : frameworks) {
		// knowing if we are system framework or not is important....
		bool bIsSystemFramework = false;
		size_t foundUnixPath = f.find('/');
		size_t foundWindowsPath = f.find('\\');
		if (foundUnixPath==string::npos &&
			foundWindowsPath==string::npos){
			bIsSystemFramework = true;                  // we have no "path" so we are system
		}

		if (bIsSystemFramework){
			; // do we need to do anything here?
		} else {
			// if addon is local, it is relative to the project folder, and if it is not, it is related to the project folder, ex: addons/ofxSvg
			fs::path rel = fs::relative (f, isLocalAddon ? pathToProject : pathToOF);
			fs::path folder = rel.parent_path();

			if (isLocalAddon) {
				fs::path fFS { f };
				folder = fs::path { "local_addons" } / fs::relative(fFS.parent_path(), parentFolder);
			}

			filesToFolders[f] = ofPathToString(folder);
		}
	}

	for (const auto & f : xcframeworks) {
		
		fs::path rel = fs::relative(f, isLocalAddon ? pathToProject : pathToOF);
		fs::path folder = rel.parent_path();

		if (isLocalAddon) {
			fs::path fFS { f };
			folder = fs::path { "local_addons" } / fs::relative(fFS.parent_path(), parentFolder);
		}

		filesToFolders[f] = ofPathToString(folder);
	}
}
	
bool ofAddon::fromFS(const fs::path & path, const string & platform){
	//alert("ofAddon::fromFS path : " + path.string(), 33);
	
	if (!fs::exists(path)) {
		return false;
	}
	
	clear();
	this->platform = platform;

	addonPath = path;

	
	name = isLocalAddon ? ofPathToString(path.stem()) : ofPathToString(path.filename());

	fs::path srcPath { path / "src" };
	if (fs::exists(srcPath)) {
		getFilesRecursively(srcPath, srcFiles);
	}

	// MARK: srcFiles to fs::path
	// not possible today because there are string based exclusion functions

	fs::path parentFolder { path.parent_path() };

	for (auto & s : srcFiles) {
		fs::path folder;
		if (isLocalAddon) {
			fs::path sFS { s };
			folder = fs::path { "local_addons" } / fs::relative(sFS.parent_path(), parentFolder);
		} else {
			fs::path sFS { fixPath(s) };
			s = sFS;
			folder = fs::relative(sFS.parent_path(), getOFRoot());
		}
		filesToFolders[s] = folder;
	}

	
	if (platform == "vs" || platform == "msys2") {
		// here addonPath is the same as path.
		getPropsRecursively(addonPath, propsFiles, platform);
	}



	fs::path libsPath { path / "libs" };

	// paths that are needed for the includes.
	std::list < fs::path > paths;

	// get every folder in addon/src and addon/libs
	vector < fs::path > libFolders;
	if (fs::exists(libsPath)) {
		getFoldersRecursively(libsPath, libFolders, platform);
		for (auto & path : libFolders) {
			paths.emplace_back( isLocalAddon ? path : fixPath(path) );
		}
	}

	vector < fs::path > srcFolders;
	if (fs::exists(srcPath)) {
		getFoldersRecursively(srcPath, srcFolders, platform);
		for (auto & path : srcFolders) {
			paths.emplace_back( isLocalAddon ? path : fixPath(path) );
		}
	}

	
	// FIXME: MARK: - HACK:
	preParseConfig();

	parseLibsPath(libsPath, parentFolder);
    
    // lib paths are directories to parse for libs
    for (auto & a : libsPaths) {
        parseLibsPath((path / a), parentFolder);
    }

	for (auto & a : additionalLibsFolder) {
//		parseLibsPath(fs::weakly_canonical(path / a), parentFolder);
		parseLibsPath((path / a), parentFolder);
	}

	paths.sort();

	for (auto & p : paths) {
		includePaths.emplace_back( ofPathToString(p) );
	}
	
	parseConfig();

	exclude(includePaths, excludeIncludes);
	
	// Dimitre. I've added this here to exclude some srcFiles from addons,
	// ofxAssimpModelLoader was adding some files from libs/assimp/include/assimp/port/AndroidJNI
	// even when the folder was excluded from includePaths
	exclude(srcFiles, excludeIncludes);
	
	exclude(srcFiles, excludeSources);
	exclude(csrcFiles, excludeSources);
	exclude(cppsrcFiles, excludeSources);
	exclude(objcsrcFiles, excludeSources);
	exclude(headersrcFiles, excludeSources);
//	exclude(propsFiles, excludeSources);
	exclude(frameworks, excludeFrameworks);
	exclude(xcframeworks, excludeXCFrameworks);
	exclude(libs, excludeLibs);

	ofLogVerbose("ofAddon") << "libs after exclusions " << libs.size();

	for (auto & lib: libs) {
		ofLogVerbose("ofAddon") << lib.path;
	}

	return true;
}


void ofAddon::clear(){
	filesToFolders.clear();
	srcFiles.clear();
	propsFiles.clear();
	libs.clear();
	includePaths.clear();
    libsPaths.clear();
	name.clear();
}

fs::path ofAddon::fixPath(const fs::path & path) {
	/*
	 I was using this before
	 pathToOF / fs::relative(path, getOFRoot())
	 but the problem is fs::relative actually calculate symlink paths, modifying filename.
	 which is not good for macos dylibs, like ofxHapPlayer, so I had to replace with the original filename back
	 */
	return normalizePath(( pathToOF / fs::relative(path, getOFRoot()) ).parent_path() / path.filename());
}
