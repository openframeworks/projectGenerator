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



void ofAddon::getFrameworksRecursively(const fs::path & path, string platform) {
//	alert ("getFrameworksRecursively for " + platform + " : " + path.string(), 34);
	if (!fs::exists(path) || !fs::is_directory(path)) return;

	for (const auto & f : dirList(path)) {
		if (fs::is_directory(f)) {
			if (f.extension() == ".framework" || f.extension() == ".xcframework") {
//				alert ("found XCF " + f.string(), 31);
				bool platformFound = false;
				
//				if (ofIsStringInString(platform), f.string()) {
				if (!platform.empty() && f.string().find(platform) != std::string::npos) {
				   platformFound = true;
				}

				if(platformFound) {
					if (f.extension() == ".framework") {
						frameworks.emplace_back(f.string());
					}
					if (f.extension() == ".xcframework") {
						xcframeworks.emplace_back(f.string());
					}
				}
			}
		}
	}
}

static std::string toString(const std::string& str){
	return str;
}

static std::string toString(const fs::path& path){
	return ofPathToString(path);
}

static std::string toString(const LibraryBinary& lib){
	return ofPathToString(lib.path);
}

template<typename T>
static inline void removeDuplicates(std::vector<T> & vec){
	std::unordered_set<std::string> seen;
	std::vector<T> output;

	for (const auto& value : vec) {
		if (seen.insert(toString(value)).second) { // If insertion is successful (element not seen before)
			output.push_back(value);
		}
	}
	vec = std::move(output);
}


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

//ofAddon::ofAddon(){
//	isLocalAddon = false;
//	pathToProject = fs::path { "." };
//	pathToOF = fs::path { "../../../ "};
//	= std::unordered_map<fs::path, fs::path>();
//}

ofAddon::ofAddon(const ofAddon& other): 
	additionalLibsFolder(other.additionalLibsFolder),
	libFiles(other.libFiles),
	filesToFolders(other.filesToFolders),
	srcFiles(other.srcFiles),
	csrcFiles(other.csrcFiles),
	cppsrcFiles(other.cppsrcFiles),
	headersrcFiles(other.headersrcFiles),
	objcsrcFiles(other.objcsrcFiles),
	propsFiles(other.propsFiles),
	libs(other.libs),
	dllsToCopy(other.dllsToCopy),
	includePaths(other.includePaths),
	libsPaths(other.libsPaths),
	dependencies(other.dependencies),
	cflags(other.cflags),
	cppflags(other.cppflags),
	ldflags(other.ldflags),
	pkgConfigLibs(other.pkgConfigLibs),
	frameworks(other.frameworks),
	xcframeworks(other.xcframeworks),
	data(other.data),
	defines(other.defines),
	definesCMAKE(other.definesCMAKE),
	name(other.name),
	addonPath(other.addonPath),
	description(other.description),
	author(other.author),
	tags(other.tags),
	url(other.url),
	pathToOF(other.pathToOF),
	pathToProject(other.pathToProject),
	isLocalAddon(other.isLocalAddon),
	addonMakeName(other.addonMakeName),
	currentParseState(other.currentParseState),
	emptyString(other.emptyString),
	platform(other.platform),
	excludeLibs(other.excludeLibs),
	excludeSources(other.excludeSources),
	excludeIncludes(other.excludeIncludes),
	excludeFrameworks(other.excludeFrameworks),
	excludeXCFrameworks(other.excludeXCFrameworks)

{
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
}


bool ofAddon::checkCorrectVariable(const string & variable, const string & state){
	if (state == "meta") {
		return std::find(AddonMetaVariables.begin(),
						 AddonMetaVariables.end(),
						 variable) != AddonMetaVariables.end();
	}
	else if (state == "osx") {// Why only checking for osx?
		return std::find(AddonProjectVariables.begin(),
						 AddonProjectVariables.end(),
						 variable) != AddonProjectVariables.end();
	} else {
		return checkCorrectPlatform(state);
	}
}

void ofAddon::addReplaceString(std::string &variable, const std::string &value, bool addToVariable) {
	if (addToVariable) variable += value;
	else variable = value;
}
//
//void ofAddon::addReplaceStringPath(fs::path & variable, const std::string & value, bool addToVariable) {
//	if (addToVariable)
//		variable = fs::path {
//			variable / value
//		};
//	else
//		variable = fs::path { value };
//}

//void ofAddon::addReplaceStringVectorPre(std::vector<std::string> &variable, const std::string &value, fs::path &prefix, bool addToVariable) {
//	addReplaceStringVector(variable, value, prefix.string(), addToVariable);
//}


void ofAddon::addReplaceStringVector(std::vector<std::string> &variable, const std::string &value, const std::string &prefix, bool addToVariable) {
	if (value == prefix) return;

	std::vector<std::string> values;
	if (value.find("\"") != std::string::npos) {
		values = ofSplitString(value, "\"", true, true);
	} else {
		values = ofSplitString(value, " ", true, true);
	}

	if (!addToVariable) variable.clear();

	std::regex findVar("(\\$\\()(.+)(\\))");
	for (auto &val : values) {
		if (val != "") {
			std::smatch varMatch;
			if (std::regex_search(val, varMatch, findVar)) {
				if (varMatch.size() > 2) {
					std::string varName = varMatch[2].str();
					std::string varValue;
					if (varName == "OF_ROOT") {
						varValue = ofPathToString(pathToOF);
					} else if (!ofGetEnv(varName.c_str()).empty()) {
						varValue = ofGetEnv(varName.c_str());
					}
					if(!varValue.empty()){
						ofStringReplace(val, "$(" + varName + ")", varValue);
						ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << val;
					}
				}
			}

			if (prefix == "" || val.find(ofPathToString(pathToOF)) == 0 || fs::path{val}.is_absolute() || (val.size() && val[0] == '$')) {
				variable.emplace_back(val);
			} else {
				fs::path p = fs::path{prefix} / val;
				variable.emplace_back(ofPathToString(p));
			}
		}
	}
}

void ofAddon::addReplaceStringVectorPathStr(std::vector<fs::path> &variable, fs::path &value, const std::string &prefix, bool addToVariable) {
	if (value.string() == prefix) return;

	std::vector<std::string> values;
	if (value.string().find("\"") != std::string::npos) {
		values = ofSplitString(value.string(), "\"", true, true);
	} else {
		values = ofSplitString(value.string(), " ", true, true);
	}

	if (!addToVariable) variable.clear();

	std::regex findVar("(\\$\\()(.+)(\\))");
	for (auto & val : values) {
		if (val != "") {
			std::smatch varMatch;
			if (std::regex_search(val, varMatch, findVar)) {
				if (varMatch.size() > 2) {
					std::string varName = varMatch[2].str();
					std::string varValue;
					if (varName == "OF_ROOT") {
						varValue = ofPathToString(pathToOF);
					} else if (!ofGetEnv(varName.c_str()).empty()) {
						varValue = ofGetEnv(varName.c_str());
					}
					if(!varValue.empty()){
						ofStringReplace(val, "$(" + varName + ")", varValue);
						ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << val;
					}
				}
			}
			if (prefix == "" || val.find(ofPathToString(pathToOF)) == 0 || fs::path { val }.is_absolute() || (val.size() && val[0] == '$')) {
				variable.emplace_back(fs::path { val });
			} else {
				fs::path p = fs::path { prefix } / val;
				variable.emplace_back(p);
			}
		}
	}
}

void ofAddon::addReplaceStringVectorPath(std::vector<fs::path> &variable, const std::string &value, const std::string &prefix, bool addToVariable) {
	fs::path pathValue = fs::path { value };
	addReplaceStringVectorPathStr(variable, pathValue, prefix, addToVariable);
}

//void ofAddon::addReplaceStringVectorPathPrefix(std::vector<fs::path> &variable, const std::string &value, fs::path &prefix, bool addToVariable) {
//	fs::path pathValue = fs::path { value };
//	addReplaceStringVectorPathStr(variable, pathValue, prefix.string(), addToVariable);
//}

//void ofAddon::addReplaceStringVectorPathAll(std::vector<fs::path> & variable, fs::path & value, fs::path & prefix, bool addToVariable) {
//	if (value == prefix) return;
//
//	std::vector<std::string> values;
//	if (value.string().find("\"") != std::string::npos) {
//		values = ofSplitString(value.string(), "\"", true, true);
//	} else {
//		values = ofSplitString(value.string(), " ", true, true);
//	}
//
//	if (!addToVariable) variable.clear();
//
//	std::regex findVar("(\\$\\()(.+)(\\))");
//	for (auto &val : values) {
//		if (val != "") {
//			std::smatch varMatch;
//			if (std::regex_search(val, varMatch, findVar)) {
//				if (varMatch.size() > 2) {
//					std::string varName = varMatch[2].str();
//					std::string varValue;
//					if (varName == "OF_ROOT") {
//						varValue = ofPathToString(pathToOF);
//					} else if (!ofGetEnv(varName.c_str()).empty()) {
//						varValue = ofGetEnv(varName.c_str());
//					}
//					if(!varValue.empty()){
//						ofStringReplace(val, "$(" + varName + ")", varValue);
//						ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << val;
//					}
//				}
//			}
//
//			if (prefix.string() == "" || val.find(ofPathToString(pathToOF)) == 0 || fs::path{val}.is_absolute() || (val.size() && val[0] == '$')) {
//				variable.emplace_back(fs::path{val});
//			} else {
//				fs::path p = fs::path{prefix} / val;
//				variable.emplace_back(p);
//			}
//		}
//	}
//}
void ofAddon::addReplaceStringVectorPath(std::vector<LibraryBinary> &variable, const std::string &value,  const fs::path &prefix, bool addToVariable) {
	std::vector<std::string> values;
	if (value.find("\"") != std::string::npos) {
		values = ofSplitString(value, "\"", true, true);
	} else {
		values = ofSplitString(value, " ", true, true);
	}

	if (!addToVariable) variable.clear();
	std::regex findVar("(\\$\\()(.+)(\\))");

	for (auto &v : values) {
		if (v != "") {
			std::smatch varMatch;
			if (std::regex_search(v, varMatch, findVar)) {
				if (varMatch.size() > 2) {
					std::string varName = varMatch[2].str();
					std::string varValue;
					if (varName == "OF_ROOT") {
						varValue = ofPathToString(pathToOF);
					} else if (!ofGetEnv(varName.c_str()).empty()) {
						varValue = ofGetEnv(varName.c_str());
					}
					if(!varValue.empty()){
						ofStringReplace(v, "$(" + varName + ")", varValue);
						ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << v;
					}
				}
			}

			if (prefix.empty() || v.find(ofPathToString(pathToOF)) == 0 || fs::path{v}.is_absolute() || (v.size() && v[0] == '$')) {
				variable.push_back({ fs::path { v }, "", "" });
			} else {
				fs::path p = fs::path{prefix / v };
				variable.push_back({ofPathToString(p), "", ""});
			}
		}
	}
}

//void ofAddon::addReplaceStringVectorLibrary(std::vector<LibraryBinary> &variable, const std::string &value, const std::string &prefix, bool addToVariable) {
//	std::vector<std::string> values;
//	if (value.find("\"") != std::string::npos) {
//		values = ofSplitString(value, "\"", true, true);
//	} else {
//		values = ofSplitString(value, " ", true, true);
//	}
//
//	if (!addToVariable) variable.clear();
//	std::regex findVar("(\\$\\()(.+)(\\))");
//
//	for (auto &v : values) {
//		if (v != "") {
//			std::smatch varMatch;
//			if (std::regex_search(v, varMatch, findVar)) {
//				if (varMatch.size() > 2) {
//					std::string varName = varMatch[2].str();
//					std::string varValue;
//					if (varName == "OF_ROOT") {
//						varValue = ofPathToString(pathToOF);
//					} else if (!ofGetEnv(varName.c_str()).empty()) {
//						varValue = ofGetEnv(varName.c_str());
//					}
//					if(!varValue.empty()){
//						ofStringReplace(v, "$(" + varName + ")", varValue);
//						ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << v;
//					}
//				}
//			}
//
//			if (prefix == "" || v.find(ofPathToString(pathToOF)) == 0 || fs::path{v}.is_absolute() || (v.size() && v[0] == '$')) {
//				variable.push_back({ fs::path { v }, "", "" });
//			} else {
//				fs::path p = fs::path{prefix} / v;
//				variable.push_back({p, "", ""});
//			}
//		}
//	}
//}

void ofAddon::parseVariableValue(const string & variable, const string & value, bool addToValue, const string & line, int lineNum){


	if (variable == "ADDON_NAME"){
		if (value != name){
			ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
						<< "line " << lineNum << ": " << line << "\n\t\t"
						<< "addon name in filesystem " << name << " doesn't match with addon_config.mk " << value;
		}
		return;
	}

	fs::path addonRelPath = makeRelative(pathToProject, addonPath);
	std::string addonRelPathStr = ofPathToString(addonRelPath);

	if (variable == ADDON_ADDITIONAL_LIBS) {
		additionalLibsFolder.emplace_back(fs::path { value });
		return;
	}

	else if (variable == ADDON_DESCRIPTION) {
		addReplaceString(description, value, addToValue);
		return;
	}

	else if(variable == ADDON_AUTHOR){
		addReplaceString(author,value,addToValue);
		return;
	}

	else if(variable == ADDON_TAGS){
		addReplaceStringVector(tags,value,emptyString,addToValue);
		return;
	}

	else if(variable == ADDON_URL){
		addReplaceString(url,value,addToValue);
		return;
	}

	else if(variable == ADDON_DEPENDENCIES){
		addReplaceStringVector(dependencies,value,emptyString,addToValue);
	}

	else if(variable == ADDON_INCLUDES){
		addReplaceStringVectorPath(includePaths, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_CFLAGS){
		addReplaceStringVector(cflags,value,emptyString,addToValue);
	}

	else if(variable == ADDON_CPPFLAGS){
		addReplaceStringVector(cppflags,value,emptyString,addToValue);
	}

	else if(variable == ADDON_LDFLAGS){
		addReplaceStringVector(ldflags,value,emptyString,addToValue);
	}

	else if(variable == ADDON_LIBS){
		addReplaceStringVectorPath(libs, value, addonRelPath, addToValue);
	}

	else if(variable == ADDON_DLLS_TO_COPY){
		addReplaceStringVectorPath(dllsToCopy, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_PKG_CONFIG_LIBRARIES){
		addReplaceStringVector(pkgConfigLibs,value,emptyString,addToValue);
	}

	else if(variable == ADDON_FRAMEWORKS){
		size_t found=value.find('/');
		if (found==string::npos) { // This path doesn't have slashes
			addReplaceStringVector(frameworks, value, emptyString, addToValue);
		} else {
			addReplaceStringVector(frameworks, value, addonRelPathStr, addToValue);
		}
	}

	else if (variable == ADDON_XCFRAMEWORKS) {
		addReplaceStringVector(xcframeworks, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_SOURCES){
		addReplaceStringVectorPath(srcFiles, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_C_SOURCES){
		addReplaceStringVectorPath(csrcFiles, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_CPP_SOURCES){
		addReplaceStringVectorPath(cppsrcFiles, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_HEADER_SOURCES){
		addReplaceStringVectorPath(headersrcFiles, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_OBJC_SOURCES){
		addReplaceStringVectorPath(objcsrcFiles, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_DATA){
		addReplaceStringVector(data,value,emptyString,addToValue);
	}

	else if(variable == ADDON_LIBS_EXCLUDE){
		addReplaceStringVector(excludeLibs,value,emptyString,addToValue);
	}

	else if(variable == ADDON_LIBS_DIR){
		addReplaceStringVectorPath(libsPaths, value, addonRelPathStr, addToValue);
	}

	else if(variable == ADDON_SOURCES_EXCLUDE){
		addReplaceStringVector(excludeSources,value,emptyString,addToValue);
	}

	else if(variable == ADDON_INCLUDES_EXCLUDE){
		addReplaceStringVector(excludeIncludes,value,emptyString,addToValue);
	}

	else if (variable == ADDON_FRAMEWORKS_EXCLUDE) {
		addReplaceStringVector(excludeFrameworks, value, emptyString, addToValue);
	}

	else if (variable == ADDON_DEFINES) {
		addReplaceStringVector(defines, value, emptyString, addToValue);
	}
}

template<typename T>
void exclude(std::vector<T>& variables, vector<string> exclusions){
	for(auto & exclusion: exclusions){
		ofStringReplace(exclusion,"\\","/");
		ofStringReplace(exclusion,".","\\.");
		ofStringReplace(exclusion,"%",".*");
		exclusion =".*"+ exclusion;

		std::regex findVar(exclusion);
		std::smatch varMatch;
		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const T & variable){
			std::string forwardSlashedVariable = ofPathToString(variable);
			ofStringReplace(forwardSlashedVariable, "\\", "/");
			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
		}), variables.end());
	}
}
//template<typename T>
//void exclude(vector<T> & variables, vector<string> exclusions){
//    for(auto & exclusion: exclusions){
//        eraseVariable<T>(variables, exclusion);
//    }
//}
//
//void ofAddon::exclude(vector<string> & variables, vector<string> exclusions){
//	for(auto & exclusion: exclusions){
//        eraseVariable<std::string>(variables, exclusion);
////		ofStringReplace(exclusion,"\\","/");
////		ofStringReplace(exclusion,".","\\.");
////		ofStringReplace(exclusion,"%",".*");
////		exclusion =".*"+ exclusion;
////
////		std::regex findVar(exclusion);
////		std::smatch varMatch;
////
////		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const std::string & variable){
////			auto forwardSlashedVariable = variable;
////			ofStringReplace(forwardSlashedVariable, "\\", "/");
////			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
////		}), variables.end());
//	}
//}
//
//void ofAddon::excludePathStr(vector<fs::path> & variables, vector<string> exclusions){
//	for (auto & exclusion : exclusions) {
//        eraseVariable<fs::path>(variables, exclusion);
////		ofStringReplace(exclusion, "\\", "/");
////		ofStringReplace(exclusion, ".", "\\.");
////		ofStringReplace(exclusion, "%", ".*");
////		exclusion = ".*" + exclusion;
////
////		std::regex findVar(exclusion);
////		std::smatch varMatch;
////
////		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const fs::path & variable) {
////			auto forwardSlashedVariable = variable.string();
////			ofStringReplace(forwardSlashedVariable, "\\", "/");
////			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
////		}), variables.end());
//	}
//}
////void ofAddon::excludePath(vector<fs::path> & variables, vector<fs::path> exclusions){
//	for(auto & exclusion: exclusions){
//		string excluse = exclusion.string();
//		ofStringReplace(excluse,"\\","/");
//		ofStringReplace(excluse,".","\\.");
//		ofStringReplace(excluse,"%",".*");
//		excluse =".*"+ excluse;
//
//		std::regex findVar(excluse);
//		std::smatch varMatch;
//
//		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const fs::path & variable) {
//			auto forwardSlashedVariable = variable.string();
//			ofStringReplace(forwardSlashedVariable, "\\", "/");
//			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
//		}),
//			variables.end());
//	}
//}

//
//
//void ofAddon::excludeLibrary(vector<LibraryBinary> & variables, vector<string> exclusions) {
//	for(auto & exclusion: exclusions){
//        eraseVariable(variables, exclusion);
////		ofStringReplace(exclusion,"\\","/");
////		ofStringReplace(exclusion,".","\\.");
////		ofStringReplace(exclusion,"%",".*");
////		exclusion =".*"+ exclusion;
////
////		std::regex findVar(exclusion);
////		std::smatch varMatch;
////		variables.erase(std::remove_if(variables.begin(), variables.end(), [&](const LibraryBinary & variable){
////			auto forwardSlashedVariable = variable.path.string();
////			ofStringReplace(forwardSlashedVariable, "\\", "/");
////			return std::regex_search(forwardSlashedVariable, varMatch, findVar);
////		}), variables.end());
//	}
//}

//
//void ofAddon::preParseConfig(){
//	//	alert ("ofAddon::parseConfig " + addonPath.string(), 33);
//    fs::path fileName =     (addonPath / "addon_config.mk");
//
//	if (!fs::exists(fileName)) {
////		ofLogError() << "ofAddon::parseConfig() " << fileName << " not found " << ofPathToString(fileName);
//		return;
//	}
//
//	for (auto & line : fileToStrings(fileName)) {
//		line = ofTrim(line);
//
//		if (line[0]=='#' || line == "") {
//			continue;
//		} // discard comments
//
//
//		// found section?
//		if (line.back() == ':'){
//			ofStringReplace(line, ":", "");
//			currentParseState = line;
//
//			if (std::find(parseStates.begin(), parseStates.end(), currentParseState) == parseStates.end()) {
//				ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
////								<< "line " << lineNum << ": " << originalLine << "\n\t\t"
//								<< "sectionName " << currentParseState << " not recognized";
//			}
//			continue;
//		}
//
//		// found Variable
//		if (line.find("=") != string::npos){
////			bool addToValue = false;
//			string variable, value;
//			vector<string> varValue;
//			if (line.find("+=") != string::npos) {
////				addToValue = true;
//				varValue = ofSplitString(line, "+=");
//			} else {
//				varValue = ofSplitString(line, "=");
//			}
//			variable = ofTrim(varValue[0]);
//			value = ofTrim(varValue[1]);
//
//			// FIXME: This seems to be meaningless
//			if(!checkCorrectPlatform(currentParseState)){
//				continue;
//			}
//
//			if(!checkCorrectVariable(variable, currentParseState)){
//				ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
////								<< "line " << lineNum << ": " << originalLine << "\n\t\t"
//								<< "variable " << variable << " not recognized for section " << currentParseState;
//				continue;
//			}
//
//			if (variable == ADDON_ADDITIONAL_LIBS) {
//				additionalLibsFolder.emplace_back(fs::path { value });
//				//				return;
//			}
////			parseVariableValue(variable, value, addToValue, originalLine, lineNum);
//		}
//	}
//}

void ofAddon::parseConfig(){
	fs::path fileName = (addonPath / "addon_config.mk");

	if (!fs::exists(fileName)) {
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

void ofAddon::addToFolder(const fs::path& path, const fs::path & parentFolder){

	fs::path folder;
	if (isLocalAddon) {
		folder = fs::path { "local_addons" } / fs::relative(path.parent_path(), parentFolder);
	} else {
		folder = fs::relative(path.parent_path(), getOFRoot());
	}
	
	filesToFolders[path] = folder;
}

void ofAddon::parseLibsPath(const fs::path & libsPath, const fs::path & parentFolder) {
	if (!fs::exists(libsPath)) {
//		alert("file not found " + libsPath.string(), 35);
		return;
	}
//	alert ("parseLibsPath " + libsPath.string() + ", parent=" + parentFolder.string(), 35);


//	if (platform == "osx"  || platform == "macos"){
//		// Horrible hack to make it work with the bad idea of renaming osx to macos
//		getLibsRecursively(libsPath, libFiles, libs, "macos");
//		getLibsRecursively(libsPath, libFiles, libs, "osx");
//
//		getFrameworksRecursively(libsPath, "macos");
//		getFrameworksRecursively(libsPath, "osx");
////		getXCFrameworksRecursively(libsPath, "macos");
////		getXCFrameworksRecursively(libsPath, "osx");
//
//		// FIXME: This is not needed when we get libraries right.
//		// if it was needed the best was change to std::set.
//		removeDuplicates(libs);
//		removeDuplicates(libFiles);
//		removeDuplicates(frameworks);
//		removeDuplicates(xcframeworks);
//
//	} else {
//		getLibsRecursively(libsPath, libFiles, libs, platform);
//	}
	
	getLibsRecursively(libsPath, libFiles, libs, platform);
	if (platform == "osx"  || platform == "macos"){
		getFrameworksRecursively(libsPath, platform);
	}
	

	if (//platform == "osx"  ||
		platform == "ios"  ||
		platform == "tvos"){//} ||
		//platform == "macos"){

		getFrameworksRecursively(libsPath, platform);
//		getXCFrameworksRecursively(libsPath, platform);
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

	for (auto & l : libs) {
		addToFolder(l.path , parentFolder);
	}

	for (auto & s : libFiles) {
		addToFolder(s, parentFolder);
		srcFiles.emplace_back(s);
	}

	// so addons will never be system.
	for (const auto & f : frameworks) {
		// knowing if we are system framework or not is important....
		bool bIsSystemFramework = false;
		size_t foundUnixPath = f.find('/');
		size_t foundWindowsPath = f.find('\\');
		if (foundUnixPath==string::npos &&
			foundWindowsPath==string::npos){
			bIsSystemFramework = true;  // we have no "path" so we are system
		}

		if (bIsSystemFramework){
			; // do we need to do anything here?
		} else {
			addToFolder(f, parentFolder);
		}
	}

	for (const auto & f : xcframeworks) {
		addToFolder(f, parentFolder);
	}
}

string ofAddon::cleanName(const string& name){
	auto addonName = name;
#ifdef TARGET_WIN32
	//    std::replace( addonName.begin(), addonName.end(), '/', '\\' );
//    fixSlashOrder(addonName);
#endif

	{
		// in case that addonName contains a comment, get rid of it
		auto s = ofSplitString(addonName, "#");
		if(s.size()){
			addonName = s[0];
		}
	}
	return addonName;
}

// FIXME: change this. second parameter is not needed, projectDir is always CWD
bool ofAddon::load(string addonName, const fs::path& projectDir, const string& targetPlatform){
//	alert ("ofAddon::load " + addonName + " :projectDir:" + projectDir.string(), 36);
	
	// we want to set addonMakeName before cleaning the addon name, so it is preserved in the exact same way as it was passed, and the addons.make file can be (re)constructed properly
	this->addonMakeName = addonName;

//	addonName = cleanName(addonName);

	if(addonName.empty()){
		ofLogError("baseProject::addAddon") << "cant add addon with empty name";
		return false;
	}

	// a local addon can be added but it should have at least one parent folder, like
	// addons/ofxMidi if there is no separator on path PG will search in $ofw/addons path
	
	fs::path addonNamePath { addonName };
//	alert ("addonNamePath " + addonNamePath.string(), 32);
//	alert("CWD: " + fs::current_path().string(), 34);
//	alert("addonNamePath: has_parent_path " , 33);
//	cout << addonNamePath.has_parent_path() << endl;
//	alert("fs::exists " + addonNamePath.string(), 33);
//	cout << fs::exists(addonNamePath) << endl;
	
	if (addonNamePath.has_parent_path() && fs::exists(fs::current_path() / addonNamePath)) {
//		if (addonNamePath.is_absolute()) {
//			alert ("IS ABS ! " + addonNamePath.string(), 32);
//		}
		this->addonPath = addonNamePath;
		this->isLocalAddon = true;
		ofLogVerbose() << "Adding local addon: " << addonName;
//		alert ("IS LOCAL ! " + addonNamePath.string(), 34);
		//        addon.pathToProject = makeRelative(getOFRoot(), projectDir);
		//        projectDir;
	}
	
	else {
		this->addonPath = fs::path { getOFRoot() / "addons" / addonName };
//		alert ("NOT LOCAL ! " + this->addonPath.string(), 34);
	}
//	this->pathToOF = normalizePath(getOFRoot());

//	this->addonPath = normalizePath(addonPath);

	//alert ("ADDON PATH::"+this->addonPath.string(), 33);

	if (!fs::exists(this->addonPath)) {
		ofLogVerbose("ofAddon::load") << "addon does not exist!" << addonPath;
		return false;
	}
	
	this->pathToProject = projectDir;

	this->platform = targetPlatform;

	ofLogVerbose() << "addonPath to: [" << addonPath.string() << "]";
	ofLogVerbose() << "pathToOF: [" << pathToOF.string() << "]";


//	alert("ofAddon::fromFS path : " + addonPath.string(), 33);

	clear();

//	name = isLocalAddon ? ofPathToString(addonNamePath.stem()) : ofPathToString(addonNamePath.filename());
	name = ofPathToString(addonNamePath.filename());

	fs::path srcPath { addonPath / "src" };
	if (fs::exists(srcPath)) {
		getFilesRecursively(srcPath, srcFiles);
	}

//    printPaths(srcFiles, "srcFiles", 34);


	fs::path parentFolder { addonPath.parent_path() };

	for (auto & s : srcFiles) {
//		alert ("s BFR fixpath " + s.string(), 32);
//		s = fixPath(s);
//		alert ("s AFT fixpath " + s.string(), 33);

		addToFolder(s, parentFolder);
	}

	if (platform == "vs" || platform == "msys2") {
		// here addonPath is the same as path.
		getPropsRecursively(addonPath, propsFiles, platform);
	}

	fs::path libsPath { addonPath / "libs" };

	// paths that are needed for the includes.
	std::list < fs::path > paths;

	// get every folder in addon/src and addon/libs

	if (fs::exists(libsPath)) {
		vector < fs::path > libFolders;
		getFoldersRecursively(libsPath, libFolders, platform);
		for (auto & path : libFolders) {
//			paths.emplace_back( fixPath(path) );
			paths.emplace_back( path );
		}
	}

	if (fs::exists(srcPath)) {
		vector < fs::path > srcFolders;
		getFoldersRecursively(srcPath, srcFolders, platform);
		for (auto & path : srcFolders) {
//			paths.emplace_back( fixPath(path) );
			paths.emplace_back( path );
		}
	}

	paths.sort([](const fs::path & a, const fs::path & b) {
		return a.string() < b.string();
	});

	for (auto & p : paths) {
		includePaths.emplace_back(p);
	}


	// FIXME: MARK: - HACK:
//	preParseConfig();

	parseConfig();

	parseLibsPath(libsPath, parentFolder);

	// lib paths are directories to parse for libs
	for (auto & a : libsPaths) {
//		alert(a, 33);
		parseLibsPath((addonPath / a), parentFolder);
	}

	for (auto & a : additionalLibsFolder) {
//		parseLibsPath(fs::weakly_canonical(path / a), parentFolder);
		parseLibsPath((addonPath / a), parentFolder);
	}


	exclude(includePaths, excludeIncludes);

	// Dimitre. I've added this here to exclude some srcFiles from addons,
	// ofxAssimpModelLoader was adding some files from libs/assimp/include/assimp/port/AndroidJNI
	// even when the folder was excluded from includePaths
	// Roy: The purpose of not excluding the includes from srcFiles is to be able to keep a folder path out of the include paths but still have those in the IDE.
	// Which was the behaviour we had before. If an addon needs to exclude something from the src folder on a platform specific way it should use the
	// ADDON_SOURCES_EXCLUDE exclude field of the addon_config.mk file

//	excludePathStr(srcFiles, excludeIncludes);

	exclude(srcFiles, excludeSources);
	exclude(csrcFiles, excludeSources);
	exclude(cppsrcFiles, excludeSources);
	exclude(objcsrcFiles, excludeSources);
	exclude(headersrcFiles, excludeSources);
	//	exclude(propsFiles, excludeSources);
	exclude(frameworks, excludeFrameworks);
	exclude(xcframeworks, excludeXCFrameworks);
	exclude(libs, excludeLibs);

	exclude(libFiles, excludeIncludes);
	exclude(libFiles, excludeSources);



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
//	alert("of:fixPath " + path.string(), 31);
	if (path.is_absolute()) {
		return path;
	}
	
	return path;

//	alert ("ow::pathToProject " + pathToProject.string(), 31);
//	alert ("ow::pathToOF " + pathToOF.string(), 31);

//	if(isLocalAddon){
////		alert ((normalizePath(( pathToProject / fs::relative(path, pathToProject) ).parent_path() / path.filename())).string());
//		return (( pathToProject / fs::relative(path, pathToProject) ).parent_path() / path.filename());
//	}else{
//		return (( pathToOF / fs::relative(path, getOFRoot()) ).parent_path() / path.filename());
//		
//	}
}
