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
#include "Poco/RegularExpression.h"
#include <list>
#include <regex>
using namespace std;

vector<string> splitStringOnceByLeft(const string &source, const string &delimiter) {
	size_t pos = source.find(delimiter);
	vector<string> res;
	if(pos == string::npos) {
		res.push_back(source);
		return res;
	}
	
	res.push_back(source.substr(0, pos));
	res.push_back(source.substr(pos + delimiter.length()));
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

void ofAddon::addReplaceStringVector(std::vector<std::string> & variable, std::string value, std::string prefix, bool addToVariable){
	
//	cout << "======= addReplaceStringVector " << endl;
//	cout << "value : " << value << endl;
//	cout << "prefix : " << prefix << endl;
//	cout << "addToVariable : " << addToVariable << endl;
//	cout << "___INPUT" << endl;
//	for (auto & v : variable) {
//		cout << v << endl;
//	}
	vector<string> values;
	if(value.find("\"")!=string::npos){
		values = ofSplitString(value,"\"",true,true);
	}else{
		values = ofSplitString(value," ",true,true);
	}

	if(!addToVariable) variable.clear();
//	Poco::RegularExpression regEX("(?<=\\$\\()[^\\)]*");
	
	//value : -F$(OF_ROOT)/addons/ofxSyphon/libs/Syphon/lib/osx/

	//\$\(.+\)
	//(?<=\$\().+(?=\)) // now with positive look behind and look ahead to get rid of $( and )
//	std::regex findVar("(?<=\\$\().+(?=\\))");
//	std::regex findVar("\\$\\(.+\\)");
//	(\$\()(.+)(\)) // now three capture groups here. we use only the second
	std::regex findVar("(\\$\\()(.+)(\\))");
	for(int i=0;i<(int)values.size();i++){
		if(values[i]!=""){
			
//			cout << values[i] << endl;
//			cout << "----- VARMATCH" << endl;
			std::smatch varMatch;
			if(std::regex_search(values[i], varMatch, findVar)) {
				if (varMatch.size() > 2) {
//					cout << ">>> " << varMatch[2].str() << endl;
					string varName = varMatch[2].str();
					string varValue;
					if(varName == "OF_ROOT"){
						varValue = pathToOF.string();
					}else if(getenv(varName.c_str())){
						varValue = getenv(varName.c_str());
					}
					ofStringReplace(values[i],"$("+varName+")",varValue);
					ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << values[i] << endl;
				}
			}
				
//			cout << "----- POCO" << endl;
//			Poco::RegularExpression::Match match;
//			if(regEX.match(values[i],match)){
//				string varName = values[i].substr(match.offset,match.length);
//				cout << "values[i] :: " << values[i] << endl;
//				cout << "varName :: " << varName << endl;
//
//				string varValue;
//				if(varName == "OF_ROOT"){
//					varValue = pathToOF.string();
//				}else if(getenv(varName.c_str())){
//					varValue = getenv(varName.c_str());
//				}
//				ofStringReplace(values[i],"$("+varName+")",varValue);
//				ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << values[i] << endl;
//			}

			if(prefix=="" || values[i].find(pathToOF.string())==0 || ofFilePath::isAbsolute(values[i])) variable.push_back(values[i]);
			else variable.push_back(ofFilePath::join(prefix,values[i]));
		}
	}
//	cout << "___OUTPUT" << endl;
//	for (auto & v : variable) {
//		cout << v << endl;
//	}
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
	Poco::RegularExpression regEX("(?<=\\$\\()[^\\)]*");
	for (int i = 0; i<(int)values.size(); i++) {
		if (values[i] != "") {
			Poco::RegularExpression::Match match;
			if (regEX.match(values[i], match)) {
				string varName = values[i].substr(match.offset, match.length);
				string varValue;
				if(varName == "OF_ROOT"){
					varValue = pathToOF.string();
				}else if (getenv(varName.c_str())) {
					varValue = getenv(varName.c_str());
				}
				ofStringReplace(values[i], "$(" + varName + ")", varValue);
				ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << values[i];
			}

			if (prefix == "" || values[i].find(pathToOF.string()) == 0 || ofFilePath::isAbsolute(values[i])) {
				variable.push_back({ values[i], "", "" });
			} else {
				variable.push_back({ ofFilePath::join(prefix, values[i]), "", "" });
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


	of::filesystem::path addonRelPath;
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

void ofAddon::exclude(vector<string> & variables, vector<string> exclusions){
	for(auto & exclusion: exclusions){
		ofStringReplace(exclusion,"\\","/");
		ofStringReplace(exclusion,".","\\.");
		ofStringReplace(exclusion,"%",".*");
		exclusion =".*"+ exclusion;
		Poco::RegularExpression regExp(exclusion);
		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const string & variable){
			auto forwardSlashedVariable = variable;
			ofStringReplace(forwardSlashedVariable, "\\", "/");
			return regExp.match(forwardSlashedVariable);
		}), variables.end());
	}
}

void ofAddon::exclude(vector<LibraryBinary> & variables, vector<string> exclusions) {
	for(auto & exclusion: exclusions){
		ofStringReplace(exclusion,"\\","/");
		ofStringReplace(exclusion,".","\\.");
		ofStringReplace(exclusion,"%",".*");
		exclusion =".*"+ exclusion;
		Poco::RegularExpression regExp(exclusion);
		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const LibraryBinary & variable){
			auto forwardSlashedVariable = variable.path;
			ofStringReplace(forwardSlashedVariable, "\\", "/");
			return regExp.match(forwardSlashedVariable);
		}), variables.end());
	}
}

void ofAddon::parseConfig(){
	ofFile addonConfig;
	if(isLocalAddon){
		addonConfig.open(pathToProject / addonPath / "addon_config.mk");
	}else{
		addonConfig.open(addonPath / "addon_config.mk");
	}

	if(!addonConfig.exists()) return;

	string line, originalLine;
	int lineNum = 0;
	while(addonConfig.good()){
		lineNum++;
		std::getline(addonConfig,originalLine);
		line = originalLine;
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
	exclude(propsFiles, excludeSources);
	exclude(frameworks, excludeFrameworks);
	exclude(libs,excludeLibs);

	ofLogVerbose("ofAddon") << "libs after exclusions " << libs.size();
	for(auto & lib: libs){
		ofLogVerbose("ofAddon") << lib.path;
	}
}

bool ofAddon::fromFS(of::filesystem::path path, const std::string & platform){
	clear();
	this->platform = platform;

	of::filesystem::path prefixPath;
	of::filesystem::path containedPath;

	if(isLocalAddon){
		name = path.stem().string();
		addonPath = path;
		// containedPath = ofFilePath::addTrailingSlash(pathToProject); //we need to add a trailing slash for the erase to work properly
		containedPath = pathToProject; //we need to add a trailing slash for the erase to work properly
		path = pathToProject / path;
	}else{
		name = ofFilePath::getFileName(path);
		addonPath = path;
		containedPath = getOFRoot(); //we need to add a trailing slash for the erase to work properly
		prefixPath = pathToOF;
	}

	if(!ofDirectory::doesDirectoryExist(path)){
		return false;
	}

	auto srcPath = path / "src";

	if (ofDirectory(srcPath).exists()){
		getFilesRecursively(srcPath, srcFiles);
	}

	// FIXME: srcFiles to fs::path
	for (auto & s : srcFiles) {
		of::filesystem::path folder;
		if (isLocalAddon) {
			// FIXME: test if local addons is working ok
			folder = of::filesystem::path("local_addons") / of::filesystem::path(s).parent_path();
		} else {
			folder = of::filesystem::path(s).parent_path();
			folder = of::filesystem::relative(folder, containedPath);
		}
		s = of::filesystem::path(prefixPath / of::filesystem::relative(s, containedPath)).string();
		filesToFolders[s] = folder.string();
	}
	

	if (platform == "vs" || platform == "msys2") {
		getPropsRecursively(addonPath.string(), propsFiles, platform);
	}
	
	// FIXME: propsFiles to fs::path
	for (auto & s : propsFiles) {
		of::filesystem::path folder;
		if (isLocalAddon) {
			// FIXME: test if local addons is working ok
			folder = of::filesystem::path("local_addons") / of::filesystem::path(s).parent_path();
		} else {
			folder = of::filesystem::path(s).parent_path();
			folder = of::filesystem::relative(folder, containedPath);
		}
		s = of::filesystem::path(prefixPath / of::filesystem::relative(s, containedPath)).string();
	}

//	for (int i = 0; i < (int)propsFiles.size(); i++) {
//		propsFiles[i].erase(propsFiles[i].begin(), propsFiles[i].begin() + containedPath.string().length());
//		int end = propsFiles[i].rfind(of::filesystem::path("/").make_preferred().string());
//		int init = 0;
//		string folder;
//		if (!isLocalAddon) {
//			folder = propsFiles[i].substr(init, end);
//		}
//		else {
//			init = propsFiles[i].find(name);
//			folder = ofFilePath::join("local_addons", propsFiles[i].substr(init, end - init));
//		}
//		propsFiles[i] = prefixPath.string() + propsFiles[i];
//	}
	

	of::filesystem::path libsPath = path / "libs";
	vector < string > libFiles;

	if (ofDirectory(libsPath).exists()){
		getLibsRecursively(libsPath, libFiles, libs, platform);

		if (platform == "osx" || platform == "ios"){
			getFrameworksRecursively(libsPath, frameworks, platform);
		}

		if(platform == "vs" || platform == "msys2"){
			getDllsRecursively(libsPath, dllsToCopy, platform);
		}
	}
	
	
	for (auto & s : libFiles) {
		of::filesystem::path folder;
		if (isLocalAddon) {
			// FIXME: test if local addons is working ok
			folder = of::filesystem::path("local_addons") / of::filesystem::path(s).parent_path();
		} else {
			folder = of::filesystem::path(s).parent_path();
			folder = of::filesystem::relative(folder, containedPath);
		}
		s = of::filesystem::path(prefixPath / of::filesystem::relative(s, containedPath)).string();
		srcFiles.emplace_back(s);
		filesToFolders[s] = folder.string();
	}
	

	// changing libs folder from absolute to relative.
	for (auto & l : libs) {
		l.path = of::filesystem::path(prefixPath / of::filesystem::relative(l.path, containedPath)).string();
	}
	
	
//	for (int i = 0; i < (int)libs.size(); i++){
//
//		cout << "libs path before " <<  libs[i].path << endl;
//		// does libs[] have any path ? let's fix if so.
//		int end = libs[i].path.rfind(of::filesystem::path("/").make_preferred().string());
//		if (end > 0){
//			libs[i].path.erase (libs[i].path.begin(), libs[i].path.begin() + containedPath.string().length());
//			libs[i].path = prefixPath.string() + libs[i].path;
//		}
//		cout << "libs path after " <<  libs[i].path << endl;
//
//	}

	for (int i = 0; i < (int)frameworks.size(); i++){

		// knowing if we are system framework or not is important....
		
		bool bIsSystemFramework = false;
		size_t foundUnixPath = frameworks[i].find('/');
		size_t foundWindowsPath = frameworks[i].find('\\');
		if (foundUnixPath==std::string::npos &&
			foundWindowsPath==std::string::npos){
			bIsSystemFramework = true;                  // we have no "path" so we are system
		}
		
		if (bIsSystemFramework){
			
			; // do we need to do anything here?
			
		} else {
			
			
			frameworks[i].erase (frameworks[i].begin(), frameworks[i].begin() + containedPath.string().length());
			
			int init = 0;
			int end = frameworks[i].rfind(of::filesystem::path("/").make_preferred().string());
			
			string folder;
			if (!isLocalAddon) {
				folder = frameworks[i].substr(init, end);
			}
			else {
				init = frameworks[i].find(name);
				folder = ofFilePath::join("local_addons", frameworks[i].substr(init, end - init));
			}
			
			frameworks[i] = prefixPath.string() + frameworks[i];
			

			filesToFolders[frameworks[i]] = folder;

		}
	}

	// paths that are needed for the includes.
	list < of::filesystem::path > paths;
	
	// get every folder in addon/src and addon/libs
	vector < string > libFolders;
	if(ofDirectory(libsPath).exists()){
		getFoldersRecursively(libsPath, libFolders, platform);
	}

	vector < string > srcFolders;
	if(ofDirectory(srcPath).exists()){
		getFoldersRecursively(path / "src", srcFolders, platform);
	}

	// convert paths to relative
	for (auto & l : libFolders) {
		paths.push_back({ prefixPath / of::filesystem::relative(of::filesystem::path(l), containedPath) });
	}

	for (auto & l : srcFolders) {
		paths.push_back({ prefixPath / of::filesystem::relative(of::filesystem::path(l), containedPath) });
	}

	paths.sort(); //paths.unique(); // unique not needed anymore. everything is carefully inserted now.

	for (auto & p : paths) {
//		cout << p << endl;
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
