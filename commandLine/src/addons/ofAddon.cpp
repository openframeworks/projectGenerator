/*
 * ofAddon.cpp
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#include "ofUtils.h"
#include "ofFileUtils.h"
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
	currentParseState = Unknown;
}

ofAddon::ConfigParseState ofAddon::stateFromString(string name){
	if(name=="meta") return Meta;
	if(name=="common") return Common;
	if(name=="linux64") return Linux64;
	if(name=="linux") return Linux;
	if(name=="msys2") return MinGW;
	if(name=="vs") return VS;
	if(name=="linuxarmv6l") return LinuxARMv6;
	if(name=="linuxarmv7l") return LinuxARMv7;
	if(name=="linuxaarch64") return LinuxAArch64;
	if(name=="android/armeabi") return AndroidARMv5;
	if(name=="android/armeabi-v7a") return AndroidARMv7;
	if(name=="android/x86") return Androidx86;
	if(name=="emscripten") return Emscripten;
	if(name=="ios") return iOS;
	if(name=="osx") return OSX;
	return Unknown;
}

string ofAddon::stateName(ofAddon::ConfigParseState state){
	switch(state){
	case Meta:
		return "meta";
	case Common:
		return "common";
	case Linux:
		return "linux";
	case Linux64:
		return "linux64";
	case MinGW:
		return "msys2";
	case VS:
		return "vs";
	case LinuxARMv6:
		return "linuxarmv6";
	case LinuxARMv7:
		return "linuxarmv7";
	case LinuxAArch64:
		return "linuxaarch64";
	case AndroidARMv5:
		return "android/armeabi";
	case AndroidARMv7:
		return "android/armeabi-v7a";
	case Androidx86:
		return "android/x86";
	case Emscripten:
		return "emscripten";
	case iOS:
		return "ios";
	case OSX:
		return "osx";
	case Unknown:
	default:
		return "unknown";
	}
}

bool ofAddon::checkCorrectPlatform(ConfigParseState state){
	switch(state){
	case Meta:
		return true;
	case Common:
		return true;
	case Linux:
	case Linux64:
	case MinGW:
	case VS:
	case LinuxARMv6:
	case LinuxARMv7:
	case AndroidARMv5:
	case AndroidARMv7:
	case Androidx86:
	case Emscripten:
	case iOS:
	case OSX:
		return platform==stateName(state);
	case Unknown:
	default:
		return false;
	}
}


bool ofAddon::checkCorrectVariable(string variable, ConfigParseState state){
	switch(state){
	case Meta:
			return std::find(AddonMetaVariables.begin(),
							 AddonMetaVariables.end(),
							 variable) != AddonMetaVariables.end();
	case Common:
	case Linux:
	case Linux64:
	case MinGW:
	case VS:
	case LinuxARMv6:
	case LinuxARMv7:
	case AndroidARMv5:
	case AndroidARMv7:
	case Androidx86:
	case Emscripten:
	case iOS:
	case OSX:
			return std::find(AddonProjectVariables.begin(),
							 AddonProjectVariables.end(),
							 variable) != AddonProjectVariables.end();
	case Unknown:
	default:
		return false;
	}
}

void ofAddon::addReplaceString(string & variable, string value, bool addToVariable){
	if(addToVariable) variable += value;
	else variable = value;
}

void ofAddon::addReplaceStringVector(std::vector<string> & variable, string value, string prefix, bool addToVariable){
//	cout << "----------" << endl;
//	cout << "addReplaceStringVector value=" << value << ", prefix=" << prefix << ", addToVariable=" << addToVariable << endl;
//	cout << "----------" << endl;

	vector<string> values;
	if(value.find("\"")!=string::npos){
		values = ofSplitString(value,"\"",true,true);
	}else{
		values = ofSplitString(value," ",true,true);
	}

	if(!addToVariable) variable.clear();
	//value : -F$(OF_ROOT)/addons/ofxSyphon/libs/Syphon/lib/osx/

	//\$\(.+\)
	//(?<=\$\().+(?=\)) // now with positive look behind and look ahead to get rid of $( and )
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
						varValue = pathToOF.string();
					}else if(getenv(varName.c_str())){
						varValue = getenv(varName.c_str());
					}
					ofStringReplace(values[i],"$("+varName+")",varValue);
					ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << values[i] << std::endl;
				}
			}

			if(prefix=="" || values[i].find(pathToOF.string())==0 || fs::path{values[i]}.is_absolute()) {
//			if(prefix=="" || values[i].find(pathToOF.string())==0 || ofFilePath::isAbsolute(values[i])) {
				variable.emplace_back(values[i]);
			} else {
				fs::path p = fs::path{ prefix } / values[i];
				variable.emplace_back(p.string());
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
						varValue = pathToOF.string();
					}else if(getenv(varName.c_str())){
						varValue = getenv(varName.c_str());
					}
					ofStringReplace(v,"$("+varName+")",varValue);
					ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << v << std::endl;
				}
			}

			
			if (prefix == "" || v.find(pathToOF.string()) == 0 || fs::path{v}.is_absolute()) {
//			if (prefix == "" || v.find(pathToOF.string()) == 0 || ofFilePath::isAbsolute(v)) {
				variable.push_back({ v, "", "" });
			} else {
				fs::path p = fs::path { prefix } / v;
				variable.push_back( { p.string(), "", "" } );
			}
		}
	}
}

void ofAddon::parseVariableValue(string variable, string value, bool addToValue, string line, int lineNum){
	if(variable == ADDON_NAME){
		if(value!=name){
			ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
						<< "line " << lineNum << ": " << line << "\n\t\t"
						<< "addon name in filesystem " << name << " doesn't match with addon_config.mk " << value;
		}
		return;
	}


	fs::path addonRelPath;
	if (!isLocalAddon) {
		// addonRelPath = ofFilePath::addTrailingSlash(pathToOF) + "addons/" + name;
		addonRelPath = pathToOF / "addons" / name;
//		cout << "-----> this is not local addon" << endl;
//		cout << pathToOF << endl;
//		cout << addonRelPath << endl;

	}
	else addonRelPath = addonPath;

	if(variable == ADDON_DESCRIPTION){
		addReplaceString(description,value,addToValue);
		return;
	}

	if(variable == ADDON_AUTHOR){
		addReplaceString(author,value,addToValue);
		return;
	}

	if(variable == ADDON_TAGS){
		addReplaceStringVector(tags,value,"",addToValue);
		return;
	}

	if(variable == ADDON_URL){
		addReplaceString(url,value,addToValue);
		return;
	}

	if(variable == ADDON_DEPENDENCIES){
		addReplaceStringVector(dependencies,value,"",addToValue);
	}

	if(variable == ADDON_INCLUDES){
		addReplaceStringVector(includePaths, value, addonRelPath.string(), addToValue);
	}

	if(variable == ADDON_CFLAGS){
		addReplaceStringVector(cflags,value,"",addToValue);
	}

	if(variable == ADDON_CPPFLAGS){
		addReplaceStringVector(cppflags,value,"",addToValue);
	}

	if(variable == ADDON_LDFLAGS){
		addReplaceStringVector(ldflags,value,"",addToValue);
	}

	if(variable == ADDON_LIBS){
		addReplaceStringVector(libs, value, addonRelPath.string(), addToValue);
	}

	if(variable == ADDON_DLLS_TO_COPY){
		addReplaceStringVector(dllsToCopy,value,"",addToValue);
	}

	if(variable == ADDON_PKG_CONFIG_LIBRARIES){
		addReplaceStringVector(pkgConfigLibs,value,"",addToValue);
	}

	if(variable == ADDON_FRAMEWORKS){
		addReplaceStringVector(frameworks,value,"",addToValue);
	}

	if(variable == ADDON_SOURCES){
		addReplaceStringVector(srcFiles, value, addonRelPath.string() ,addToValue);
	}

	if(variable == ADDON_C_SOURCES){
		addReplaceStringVector(csrcFiles, value, addonRelPath.string() ,addToValue);
	}

	if(variable == ADDON_CPP_SOURCES){
		addReplaceStringVector(cppsrcFiles, value, addonRelPath.string() ,addToValue);
	}

	if(variable == ADDON_HEADER_SOURCES){
		addReplaceStringVector(headersrcFiles, value, addonRelPath.string() ,addToValue);
	}

	if(variable == ADDON_OBJC_SOURCES){
		addReplaceStringVector(objcsrcFiles, value, addonRelPath.string() ,addToValue);
	}

	if(variable == ADDON_DATA){
		addReplaceStringVector(data,value,"",addToValue);
	}

	if(variable == ADDON_LIBS_EXCLUDE){
		addReplaceStringVector(excludeLibs,value,"",addToValue);
	}

	if(variable == ADDON_SOURCES_EXCLUDE){
		addReplaceStringVector(excludeSources,value,"",addToValue);
	}

	if(variable == ADDON_INCLUDES_EXCLUDE){
		addReplaceStringVector(excludeIncludes,value,"",addToValue);
	}

	if (variable == ADDON_FRAMEWORKS_EXCLUDE) {
		addReplaceStringVector(excludeFrameworks, value, "", addToValue);
	}

	if (variable == ADDON_DEFINES) {
		addReplaceStringVector(defines, value, "", addToValue);
	}
}

// TODO: exclude based on vector of fs::path
void ofAddon::exclude(vector<string> & variables, vector<string> exclusions){
	for(auto & exclusion: exclusions){
		ofStringReplace(exclusion,"\\","/");
		ofStringReplace(exclusion,".","\\.");
		ofStringReplace(exclusion,"%",".*");
		exclusion =".*"+ exclusion;

//		cout << "EXCLUDE " << exclusion << endl;
//		cout << variables.size() << endl;
//		for (auto & v : variables) {
//			cout << v << endl;
//		}

		std::regex findVar(exclusion);
		std::smatch varMatch;
		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const string & variable){
			auto forwardSlashedVariable = variable;
			ofStringReplace(forwardSlashedVariable, "\\", "/");
			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
		}), variables.end());
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
			auto forwardSlashedVariable = variable.path;
			ofStringReplace(forwardSlashedVariable, "\\", "/");
			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
		}), variables.end());
	}
}

void ofAddon::parseConfig(){
	fs::path thisFilePath;
	if(isLocalAddon){
		thisFilePath = pathToProject / addonPath / "addon_config.mk";
	}else{
		thisFilePath = addonPath / "addon_config.mk";
	}

	if (!fs::exists(thisFilePath)) return;
//	if(!addonConfig.exists()) return;

	int lineNum = 0;
	
	for (auto & originalLine : fileToStrings(thisFilePath)) {
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
			currentParseState = stateFromString(line);
			if(currentParseState == Unknown){
				ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
								<< "line " << lineNum << ": " << originalLine << "\n\t\t"
								<< "sectionName " << stateName(currentParseState) << " not recognized";
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

			if(!checkCorrectPlatform(currentParseState)){
				continue;
			}

			if(!checkCorrectVariable(variable,currentParseState)){
				ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
								<< "line " << lineNum << ": " << originalLine << "\n\t\t"
								<< "variable " << variable << " not recognized for section " << stateName(currentParseState);
				continue;
			}
			parseVariableValue(variable, value, addToValue, originalLine, lineNum);
		}
	}

	exclude(includePaths,excludeIncludes);
	exclude(srcFiles, excludeSources);
	exclude(csrcFiles,excludeSources);
	exclude(cppsrcFiles,excludeSources);
	exclude(objcsrcFiles,excludeSources);
	exclude(headersrcFiles,excludeSources);
//	exclude(propsFiles, excludeSources);
	exclude(frameworks, excludeFrameworks);
	exclude(libs,excludeLibs);

	ofLogVerbose("ofAddon") << "libs after exclusions " << libs.size();
	for(auto & lib: libs){
		ofLogVerbose("ofAddon") << lib.path;
	}
}


bool ofAddon::fromFS(const fs::path & path, const string & platform){
//	alert("ofAddon::fromFS path : " + path.string());
	
	clear();
	this->platform = platform;

	addonPath = path;
	if (isLocalAddon) {
		name = path.stem().string();
	} else {
		name = path.filename().string();
	}

	if (!fs::exists(path)) {
		return false;
	}

	fs::path srcPath { path / "src" };
	if (fs::exists(srcPath)) {
		getFilesRecursively(srcPath, srcFiles);
	}

	// MARK: srcFiles to fs::path
	// not possible today because there are string based exclusion functions
	for (auto & s : srcFiles) {
		fs::path sFS { s };
		fs::path folder;
		if (isLocalAddon) {
			folder = sFS.parent_path();
		} else {
			sFS = fixPath(s);
			s = sFS.string();
			folder = fs::relative(sFS.parent_path(), getOFRoot());
		}
		filesToFolders[s] = folder.string();
	}

	if (platform == "vs" || platform == "msys2") {
		// here addonPath is the same as path.
		getPropsRecursively(addonPath, propsFiles, platform);
	}
	
	// TODO: Remove comments
//	int i = 0;
//	for (auto & s : propsFiles) {
//		fs::path sFS { s };
//		fs::path folder;
//		if (isLocalAddon) {
//			folder = sFS.parent_path();
//		} else {
//			folder = fs::relative(sFS.parent_path(), getOFRoot());
//		}
//		cout << s << endl;
//		cout << folder << endl;
//		propsFiles[i] = folder;
//		i++;
//	}

	fs::path libsPath = path / "libs";
	vector < fs::path > libFiles;
	
	if (fs::exists(libsPath)) {
		getLibsRecursively(libsPath, libFiles, libs, platform);
		if (platform == "osx" || platform == "ios"){
			getFrameworksRecursively(libsPath, frameworks, platform);
		}
		if(platform == "vs" || platform == "msys2"){
			getDllsRecursively(libsPath, dllsToCopy, platform);
		}
	}
	
	// TODO: this is not needed even if it is local addon but project is outside OF root path
	// Absolute paths will be used in this case too.
	if (!isLocalAddon) {
		for (auto & l : libs) {
//			alert("fixpath " + l.path);
			l.path = fixPath(l.path).string();
//			alert("fixpath " + l.path);
		}
	}

	
	for (auto & s : libFiles) {
		fs::path folder;
		if (isLocalAddon) {
			folder = s.parent_path();
		} else {
			folder = fs::relative(s.parent_path(), getOFRoot());
			s = fixPath(s);
		}
		srcFiles.emplace_back(s);
		filesToFolders[s.string()] = folder.string();
	}
	
	for (auto & f : frameworks) {
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
			fs::path folderFS = rel.parent_path();
			filesToFolders[f] = folderFS.string();
		}
	}

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
	
	paths.sort();

	for (auto & p : paths) {
		includePaths.emplace_back(p.string());
	}
	parseConfig();
	return true;
}


void ofAddon::clear(){
	filesToFolders.clear();
	srcFiles.clear();
	propsFiles.clear();
	libs.clear();
	includePaths.clear();
	name.clear();
}

fs::path ofAddon::fixPath(const fs::path & path) {
//	cout << "fixPath before " << path << endl;
//	cout << "fixPath after  " << fs::path { pathToOF / fs::relative(path, getOFRoot()) } << endl;
	// This actually was resolving symlink susbtituting name, so I'll use a more complex function
	//  pathToOF / fs::relative(path, getOFRoot())
	return ( pathToOF / fs::relative(path, getOFRoot()) ).parent_path() / path.filename();
}
