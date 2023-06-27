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
				variable.emplace_back((fs::path(prefix) / values[i]).string());
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
				variable.push_back( {
					( fs::path ( prefix ) / v ).string()
					, "", "" } );
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
//	ofFile addonConfig;
	fs::path thisFilePath;
	if(isLocalAddon){
		thisFilePath = pathToProject / addonPath / "addon_config.mk";
//		addonConfig.open(pathToProject / addonPath / "addon_config.mk");
	}else{
		thisFilePath = addonPath / "addon_config.mk";
//		addonConfig.open(addonPath / "addon_config.mk");
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


bool ofAddon::fromFS(fs::path path, const string & platform){
	alert("ofAddon::fromFS path : " + path.string());
	
	clear();
	this->platform = platform;

	fs::path prefixPath;
	fs::path containedPath { "" };

	if(isLocalAddon){
		name = path.stem().string();
		addonPath = path;
		path = pathToProject / path;
	}else{
//		name = ofFilePath::getFileName(path);
		name = path.filename().string();
		addonPath = path;
		containedPath = getOFRoot(); //we need to add a trailing slash for the erase to work properly
		prefixPath = pathToOF;
	}
	

	if (!fs::exists(path)) {
		return false;
	}

	fs::path srcPath { path / "src" };

	if (fs::exists(srcPath)) {
		cout << " -----> getFilesRecursively " << srcPath << endl;
		getFilesRecursively(srcPath, srcFiles);
		for (auto & s : srcFiles) {
			cout << s << endl;
			cout << fs::exists(s) << endl;
#ifdef TARGET_WIN32
//			std::replace(s.begin(), s.end(), '\', '\\');
#endif
//			 cout << s << endl;
		}
		cout << " -----> getFilesRecursively end " << endl;
	}

	// MARK: srcFiles to fs::path
	// not possible today because there are string based exclusion functions
	for (auto & s : srcFiles) {
		fs::path folder;
		auto srcFS = fs::path(prefixPath / fs::relative(s, containedPath));
		if (isLocalAddon) {
			folder = srcFS.parent_path();
		} else {
			folder = fs::relative(fs::path(s).parent_path(), containedPath);
		}
		s = srcFS.string();
		filesToFolders[s] = folder.string();
	}


	if (platform == "vs" || platform == "msys2") {
		getPropsRecursively(addonPath, propsFiles, platform);
	}
	
//	cout << "---- LIST PROPS FILES" << endl;
	for (auto & p : propsFiles) {
		cout << p << endl;
	}
//	cout << "---- LIST PROPS FILES" << endl;

//	cout << "pathToProject " << pathToProject << endl;
//	for (auto & p : propsFiles) {
//		cout << "original path " << p << endl;
//		p = fs::relative(p, pathToProject);
//		cout << "new path" << p << endl;
//	}
	
//	int i = 0;
//	for (auto & s : propsFiles) {
//		fs::path folder;
//		auto srcFS = fs::path(prefixPath / fs::relative(s, containedPath));
//		if (isLocalAddon) {
////			folder = fs::path("local_addons") / fs::path(s).parent_path();
//			folder = srcFS.parent_path();
//		} else {
//			folder = fs::relative(fs::path(s).parent_path(), containedPath);
//		}
//		s = srcFS.string();
//		propsFiles[i] = folder.string();
//		i++;
//	}

	fs::path libsPath = path / "libs";
	
	vector < string > libFiles;

	if (fs::exists(libsPath)) {
//		alert("getLibsRecursively " + libsPath.string(), 31);
		cout << "getOFRoot() " << getOFRoot() << endl;
		cout << "pathToOF " << pathToOF << endl;

		getLibsRecursively(libsPath, libFiles, libs, platform);
		
		if (platform == "osx" || platform == "ios"){
			getFrameworksRecursively(libsPath, frameworks, platform);
		}
		if(platform == "vs" || platform == "msys2"){
			getDllsRecursively(libsPath, dllsToCopy, platform);
		}
	}
	
	for (auto & l : libs) {
		auto srcFS = fs::path(prefixPath / fs::relative(l.path, containedPath));
		l.path = srcFS.string();
	}

	for (auto & s : libFiles) {
//		cout << "---- >> " << endl;
//		cout << s << endl;
		fs::path folder;
		auto srcFS = fs::path(prefixPath / fs::relative(s, containedPath));
		if (isLocalAddon) {
			folder = srcFS.parent_path();
		} else {
			folder = fs::relative(fs::path(s).parent_path(), containedPath);
		}
		s = srcFS.string();
		srcFiles.emplace_back(s);
		filesToFolders[s] = folder.string();
//		cout << s << endl;
//		cout << "filesToFolders " << s << " : " << folder << endl;
	}

	
	
//	// changing libs folder from absolute to relative.
//	for (auto & l : libs) {
//		cout << "l.path " << l.path << endl;
//		l.path = fs::path(prefixPath / fs::relative(l.path, containedPath)).string();
//		cout << "l.path " << l.path << endl;
//	}

	
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
	vector < string > libFolders;
	if (fs::exists(libsPath)) {
		getFoldersRecursively(libsPath, libFolders, platform);
	}

	vector < string > srcFolders;
	if (fs::exists(srcPath)) {
		getFoldersRecursively(srcPath, srcFolders, platform);
	}

	// convert paths to relative
	for (auto & l : libFolders) {
		paths.emplace_back( prefixPath / fs::relative(fs::path(l), containedPath) );
	}

	for (auto & l : srcFolders) {
		paths.emplace_back( prefixPath / fs::relative(fs::path(l), containedPath) );
	}

	paths.sort(); //paths.unique(); // unique not needed anymore. everything is carefully inserted now.

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
