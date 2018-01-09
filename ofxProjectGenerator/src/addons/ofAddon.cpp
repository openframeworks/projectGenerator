/*
 * ofAddon.cpp
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#include "ofAddon.h"
#include "ofUtils.h"
#include "ofFileUtils.h"
#include "Utils.h"
#include "Poco/String.h"
#include "Poco/RegularExpression.h"
#include <list>
using namespace std;

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
		return (variable=="ADDON_NAME" || variable=="ADDON_DESCRIPTION" || variable=="ADDON_AUTHOR" || variable=="ADDON_TAGS" || variable=="ADDON_URL");
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
		return (variable == "ADDON_DEPENDENCIES" || variable == "ADDON_INCLUDES" ||
				variable == "ADDON_CFLAGS" || variable == "ADDON_CPPFLAGS" ||
				variable == "ADDON_LDFLAGS"  || variable == "ADDON_LIBS" || variable == "ADDON_PKG_CONFIG_LIBRARIES" ||
				variable == "ADDON_FRAMEWORKS" ||
				variable == "ADDON_SOURCES" || variable == "ADDON_OBJC_SOURCES" || variable == "ADDON_CPP_SOURCES" || variable == "ADDON_HEADER_SOURCES" ||
				variable == "ADDON_DATA" ||
				variable == "ADDON_LIBS_EXCLUDE" || variable == "ADDON_SOURCES_EXCLUDE" || variable == "ADDON_INCLUDES_EXCLUDE" ||
				variable == "ADDON_DLLS_TO_COPY" ||
				variable == "ADDON_DEFINES");
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
	vector<string> values;
	if(value.find("\"")!=string::npos){
		values = ofSplitString(value,"\"",true,true);
	}else{
		values = ofSplitString(value," ",true,true);
	}

	if(!addToVariable) variable.clear();
	Poco::RegularExpression regEX("(?<=\\$\\()[^\\)]*");
	for(int i=0;i<(int)values.size();i++){
		if(values[i]!=""){
            Poco::RegularExpression::Match match;
            if(regEX.match(values[i],match)){
                string varName = values[i].substr(match.offset,match.length);
                string varValue;
				if(varName == "OF_ROOT"){
					varValue = pathToOF;
				}else if(getenv(varName.c_str())){
                    varValue = getenv(varName.c_str());
                }
				ofStringReplace(values[i],"$("+varName+")",varValue);
				ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << values[i] << endl;
            }

			if(prefix=="" || values[i].find(pathToOF)==0 || ofFilePath::isAbsolute(values[i])) variable.push_back(values[i]);
			else variable.push_back(ofFilePath::join(prefix,values[i]));
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
	Poco::RegularExpression regEX("(?<=\\$\\()[^\\)]*");
	for (int i = 0; i<(int)values.size(); i++) {
		if (values[i] != "") {
			Poco::RegularExpression::Match match;
			if (regEX.match(values[i], match)) {
				string varName = values[i].substr(match.offset, match.length);
				string varValue;
				if(varName == "OF_ROOT"){
					varValue = pathToOF;
				}else if (getenv(varName.c_str())) {
					varValue = getenv(varName.c_str());
				}
				ofStringReplace(values[i], "$(" + varName + ")", varValue);
				ofLogVerbose("ofAddon") << "addon config: substituting " << varName << " with " << varValue << " = " << values[i];
			}

			if (prefix == "" || values[i].find(pathToOF) == 0 || ofFilePath::isAbsolute(values[i])) {
				variable.push_back({ values[i], "", "" });
			} else {
				variable.push_back({ ofFilePath::join(prefix, values[i]), "", "" });
			}
		}
	}
}

void ofAddon::parseVariableValue(string variable, string value, bool addToValue, string line, int lineNum){
	if(variable == "ADDON_NAME"){
		if(value!=name){
			ofLogError() << "Error parsing " << name << " addon_config.mk" << "\n\t\t"
						<< "line " << lineNum << ": " << line << "\n\t\t"
						<< "addon name in filesystem " << name << " doesn't match with addon_config.mk " << value;
		}
		return;
	}


	string addonRelPath;
	if (!isLocalAddon) addonRelPath = ofFilePath::addTrailingSlash(pathToOF) + "addons/" + name;
	else addonRelPath = addonPath;

	if(variable == "ADDON_DESCRIPTION"){
		addReplaceString(description,value,addToValue);
		return;
	}

	if(variable == "ADDON_AUTHOR"){
		addReplaceString(author,value,addToValue);
		return;
	}

	if(variable == "ADDON_TAGS"){
		addReplaceStringVector(tags,value,"",addToValue);
		return;
	}

	if(variable == "ADDON_URL"){
		addReplaceString(url,value,addToValue);
		return;
	}

	if(variable == "ADDON_DEPENDENCIES"){
		addReplaceStringVector(dependencies,value,"",addToValue);
	}

	if(variable == "ADDON_INCLUDES"){
		addReplaceStringVector(includePaths,value,addonRelPath,addToValue);
	}

	if(variable == "ADDON_CFLAGS"){
		addReplaceStringVector(cflags,value,"",addToValue);
	}

	if(variable == "ADDON_CPPFLAGS"){
		addReplaceStringVector(cppflags,value,"",addToValue);
	}

	if(variable == "ADDON_LDFLAGS"){
		addReplaceStringVector(ldflags,value,"",addToValue);
	}

	if(variable == "ADDON_LIBS"){
		addReplaceStringVector(libs,value,addonRelPath,addToValue);
	}

	if(variable == "ADDON_DLLS_TO_COPY"){
		addReplaceStringVector(dllsToCopy,value,"",addToValue);
	}

	if(variable == "ADDON_PKG_CONFIG_LIBRARIES"){
		addReplaceStringVector(pkgConfigLibs,value,"",addToValue);
	}

	if(variable == "ADDON_FRAMEWORKS"){
		addReplaceStringVector(frameworks,value,"",addToValue);
	}

	if(variable == "ADDON_SOURCES"){
		addReplaceStringVector(srcFiles,value,addonRelPath,addToValue);
	}

	if(variable == "ADDON_C_SOURCES"){
		addReplaceStringVector(csrcFiles,value,addonRelPath,addToValue);
	}

	if(variable == "ADDON_CPP_SOURCES"){
		addReplaceStringVector(cppsrcFiles,value,addonRelPath,addToValue);
	}

	if(variable == "ADDON_HEADER_SOURCES"){
		addReplaceStringVector(headersrcFiles,value,addonRelPath,addToValue);
	}

	if(variable == "ADDON_OBJC_SOURCES"){
		addReplaceStringVector(objcsrcFiles,value,addonRelPath,addToValue);
	}

	if(variable == "ADDON_DATA"){
		addReplaceStringVector(data,value,addonRelPath,addToValue);
	}

	if(variable == "ADDON_LIBS_EXCLUDE"){
		addReplaceStringVector(excludeLibs,value,"",addToValue);
	}

	if(variable == "ADDON_SOURCES_EXCLUDE"){
		addReplaceStringVector(excludeSources,value,"",addToValue);
	}

	if(variable == "ADDON_INCLUDES_EXCLUDE"){
		addReplaceStringVector(excludeIncludes,value,"",addToValue);
	}

	if (variable == "ADDON_DEFINES") {
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
	    addonConfig.open(ofFilePath::join(ofFilePath::join(pathToProject,addonPath),"addon_config.mk"));
	}else{
	    addonConfig.open(ofFilePath::join(addonPath,"addon_config.mk"));
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
				varValue = ofSplitString(line,"+=");
			}else{
				addToValue = false;
				varValue = ofSplitString(line,"=");
			}
			variable = Poco::trim(varValue[0]);
			value = Poco::trim(varValue[1]);

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
	exclude(srcFiles,excludeSources);
	exclude(csrcFiles,excludeSources);
	exclude(cppsrcFiles,excludeSources);
	exclude(objcsrcFiles,excludeSources);
	exclude(headersrcFiles,excludeSources);
	exclude(libs,excludeLibs);

	ofLogVerbose("ofAddon") << "libs after exclusions " << libs.size();
	for(auto & lib: libs){
		ofLogVerbose("ofAddon") << lib.path;
	}
}

void ofAddon::fromFS(string path, string platform){
    clear();
    this->platform = platform;
	string prefixPath;

    string containedPath;
    if(isLocalAddon){
        name = std::filesystem::path(path).stem().string();
        addonPath = path;
        containedPath = ofFilePath::addTrailingSlash(pathToProject); //we need to add a trailing slash for the erase to work properly
        path = ofFilePath::join(pathToProject,path);
    }else{
        name = ofFilePath::getFileName(path);
        addonPath = path;
        containedPath = ofFilePath::addTrailingSlash(getOFRoot()); //we need to add a trailing slash for the erase to work properly
        prefixPath = pathToOF;
    }


    string srcPath = ofFilePath::join(path, "/src");
    ofLogVerbose() << "in fromFS, trying src " << srcPath;
    if (ofDirectory(srcPath).exists()){
        getFilesRecursively(srcPath, srcFiles);
    }

    for(int i=0;i<(int)srcFiles.size();i++){
        srcFiles[i].erase(srcFiles[i].begin(), srcFiles[i].begin()+containedPath.length());
        int end = srcFiles[i].rfind(std::filesystem::path("/").make_preferred().string());
        int init = 0;
		string folder;
    	if(!isLocalAddon){
            folder = srcFiles[i].substr(init,end);
    	}else{
            init = srcFiles[i].find(name);
            folder = ofFilePath::join("local_addons", srcFiles[i].substr(init,end-init));
    	}
    	srcFiles[i] = prefixPath + srcFiles[i];
    	filesToFolders[srcFiles[i]] = folder;
    }
    

    string libsPath = ofFilePath::join(path, "/libs");
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
    

    // I need to add libFiles to srcFiles
    for (int i = 0; i < (int)libFiles.size(); i++){
    	libFiles[i].erase (libFiles[i].begin(), libFiles[i].begin()+containedPath.length());
		//ofLogVerbose() << " libFiles " << libFiles[i];
    	int init = 0;
        int end = libFiles[i].rfind(std::filesystem::path("/").make_preferred().string());
        if (end > 0){
			string folder;
			if (!isLocalAddon) {
				folder = libFiles[i].substr(init, end);
			}
			else {
				init = libFiles[i].find(name);
				folder = ofFilePath::join("local_addons", libFiles[i].substr(init, end - init));
			}
            libFiles[i] = prefixPath + libFiles[i];
            srcFiles.push_back(libFiles[i]);
            filesToFolders[libFiles[i]] = folder;
        }

    }

    
    
    for (int i = 0; i < (int)libs.size(); i++){

        // does libs[] have any path ? let's fix if so.
    	int end = libs[i].path.rfind(std::filesystem::path("/").make_preferred().string());
        if (end > 0){
            libs[i].path.erase (libs[i].path.begin(), libs[i].path.begin()+containedPath.length());
            libs[i].path = prefixPath + libs[i].path;
        }

    }

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
            
            
            frameworks[i].erase (frameworks[i].begin(), frameworks[i].begin()+containedPath.length());
            
            int init = 0;
    	    int end = frameworks[i].rfind(std::filesystem::path("/").make_preferred().string());
            
			string folder;
			if (!isLocalAddon) {
				folder = frameworks[i].substr(init, end);
			}
			else {
				init = frameworks[i].find(name);
				folder = ofFilePath::join("local_addons", frameworks[i].substr(init, end - init));
			}
            
            frameworks[i] = prefixPath + frameworks[i];
            

            filesToFolders[frameworks[i]] = folder;

        }
        
       

    }






    // get a unique list of the paths that are needed for the includes.
    list < string > paths;
    for (int i = 0; i < (int)srcFiles.size(); i++){
        size_t found;
    	found = srcFiles[i].find_last_of(std::filesystem::path("/").make_preferred().string());
        paths.push_back(srcFiles[i].substr(0,found));
    }

    // get every folder in addon/src and addon/libs
    vector < string > libFolders;
    if(ofDirectory(libsPath).exists()){
        getFoldersRecursively(libsPath, libFolders, platform);
    }

    vector < string > srcFolders;
    if(ofDirectory(srcPath).exists()){
        getFoldersRecursively(ofFilePath::join(path, "/src"), srcFolders, platform);
    }

    for (int i = 0; i < (int)libFolders.size(); i++){
        libFolders[i].erase (libFolders[i].begin(), libFolders[i].begin()+containedPath.length());
        libFolders[i] = prefixPath + libFolders[i];
        paths.push_back(libFolders[i]);
    }

    for (int i = 0; i < (int)srcFolders.size(); i++){
        srcFolders[i].erase (srcFolders[i].begin(), srcFolders[i].begin()+containedPath.length());
        srcFolders[i] = prefixPath + srcFolders[i];
        paths.push_back(srcFolders[i]);
    }

    paths.sort();
    paths.unique();
    for (list<string>::iterator it=paths.begin(); it!=paths.end(); ++it){
        includePaths.push_back(*it);
    }

    parseConfig();

}

void ofAddon::fromXML(string installXmlName){
	clear();
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(ofToDataPath(installXmlName).c_str());

    // this is src to add:
    pugi::xpath_node_set add = doc.select_nodes("//add/src/folder/file");
    for (pugi::xpath_node_set::const_iterator it = add.begin(); it != add.end(); ++it){
        pugi::xpath_node node = *it;
        //std::cout << "folder name "  << node.node().parent().attribute("name").value() << " : ";
        //std::cout << "src: " << node.node().child_value() << endl;
    }


    add = doc.select_nodes("//include/path");
    for (pugi::xpath_node_set::const_iterator it = add.begin(); it != add.end(); ++it){
        pugi::xpath_node node = *it;
        //std::cout << "include: " << node.node().child_value() << endl;
    }


    add = doc.select_nodes("//link/lib[@compiler='codeblocks']");
    // this has to be smarter I guess...
    for (pugi::xpath_node_set::const_iterator it = add.begin(); it != add.end(); ++it){
        pugi::xpath_node node = *it;
        //std::cout << "link: " << node.node().child_value() << endl;
    }


}


void ofAddon::clear(){
    filesToFolders.clear();
    srcFiles.clear();
    libs.clear();
    includePaths.clear();
    name.clear();
}
