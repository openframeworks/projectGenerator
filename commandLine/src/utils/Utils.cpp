/*
 * Utils.cpp
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#include "Utils.h"
#include "ofUtils.h"
#include "qtcreatorproject.h"
#include "CBWinProject.h"
#include "xcodeProject.h"
#include "visualStudioProject.h"
#include "androidStudioProject.h"
#include "uuidxx.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

#ifdef TARGET_WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#elif defined(TARGET_LINUX)
#include <unistd.h>
#define GetCurrentDir getcwd
#else
#include <mach-o/dyld.h>	/* _NSGetExecutablePath */
#include <limits.h>		/* PATH_MAX */
#endif

using std::unique_ptr;
namespace fs = of::filesystem;

using std::cout;
using std::endl;

std::string generateUUID(std::string input){
	return uuidxx::uuid::Generate().ToString();
}

void findandreplace( std::string& tInput, std::string tFind, std::string tReplace ) {
	size_t uPos = 0;
	size_t uFindLen = tFind.length();
	size_t uReplaceLen = tReplace.length();

	if( uFindLen == 0 ){
		return;
	}

	for( ;(uPos = tInput.find( tFind, uPos )) != std::string::npos; ){
		tInput.replace( uPos, uFindLen, tReplace );
		uPos += uReplaceLen;
	}

}


std::string LoadFileAsString(const std::string & fn) {
	std::ifstream fin(fn.c_str());

	if(!fin) {
		// throw exception
	}

	std::ostringstream oss;
	oss << fin.rdbuf();

	return oss.str();
}

void findandreplaceInTexfile (const fs::path & fileName, std::string tFind, std::string tReplace ){
//void findandreplaceInTexfile (std::string fileName, std::string tFind, std::string tReplace ){
	if (fs::exists( fileName )) {
		std::ifstream t(ofToDataPath(fileName).c_str());
		std::stringstream buffer;
		buffer << t.rdbuf();
		std::string bufferStr = buffer.str();
		t.close();
		findandreplace(bufferStr, tFind, tReplace);
		std::ofstream myfile;
		myfile.open (ofToDataPath(fileName).c_str());
		myfile << bufferStr;
		myfile.close();

	} else {
	   ; // some error checking here would be good.
   }
}




bool doesTagAndAttributeExist(pugi::xml_document & doc, std::string tag, std::string attribute, std::string newValue){
	char xpathExpressionExists[1024];
	sprintf(xpathExpressionExists, "//%s[@%s='%s']", tag.c_str(), attribute.c_str(), newValue.c_str());
	//cout <<xpathExpressionExists <<endl;
	pugi::xpath_node_set set = doc.select_nodes(xpathExpressionExists);
	if (set.size() != 0){
		return true;
	} else {
		return false;
	}
}

pugi::xml_node appendValue(pugi::xml_document & doc, std::string tag, std::string attribute, std::string newValue, bool overwriteMultiple){

	if (overwriteMultiple == true){
		// find the existing node...
		char xpathExpression[1024];
		sprintf(xpathExpression, "//%s[@%s='%s']", tag.c_str(), attribute.c_str(), newValue.c_str());
		pugi::xpath_node node = doc.select_node(xpathExpression);
		if(std::string(node.node().attribute(attribute.c_str()).value()).size() > 0){ // for some reason we get nulls here?
			// ...delete the existing node
			std::cout << "DELETING: " << node.node().name() << ": " << " " << node.node().attribute(attribute.c_str()).value() << std::endl;
			node.node().parent().remove_child(node.node());
		}
	}

	if (!doesTagAndAttributeExist(doc, tag, attribute, newValue)){
		// otherwise, add it please:
		char xpathExpression[1024];
		sprintf(xpathExpression, "//%s[@%s]", tag.c_str(), attribute.c_str());
		//cout << xpathExpression << endl;
		pugi::xpath_node_set add = doc.select_nodes(xpathExpression);
		pugi::xml_node node = add[add.size()-1].node();
		pugi::xml_node nodeAdded = node.parent().append_copy(node);
		nodeAdded.attribute(attribute.c_str()).set_value(newValue.c_str());
		return nodeAdded;
	}else{
		return pugi::xml_node();
	}

}


void getFilesRecursively(const fs::path & path, std::vector < string > & fileNames){
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (f.filename().c_str()[0] == '.') continue; // avoid hidden files .DS_Store .vscode .git etc
		if (ofIsStringInString(f.filename().string(),".framework")) continue; // ignore frameworks
		
		if (fs::is_directory(f)) {
			getFilesRecursively(f, fileNames);
		} else {
			// FIXME - update someday to fs::path
			fileNames.emplace_back(f);
		}
	}
}


void getFilesRecursively(const fs::path & path, std::vector < fs::path > & fileNames){
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (f.filename().c_str()[0] == '.') continue; // avoid hidden files .DS_Store .vscode .git etc
		if (ofIsStringInString(f.filename().string(),".framework")) continue; // ignore frameworks
		
		if (fs::is_directory(f)) {
			if (f.filename() != fs::path(".git")) { // ignore git dir
				getFilesRecursively(f, fileNames);
			}
		} else {
			fileNames.emplace_back(f);
		}
	}
}


static std::vector <std::string> platforms;
bool isFolderNotCurrentPlatform(std::string folderName, std::string platform){
	if( platforms.size() == 0 ){
		platforms.push_back("osx");
		platforms.push_back("msys2");
		platforms.push_back("vs");
		platforms.push_back("ios");
		platforms.push_back("linux");
		platforms.push_back("linux64");
		platforms.push_back("android");
		platforms.push_back("iphone");
	}

	for(int i = 0; i < platforms.size(); i++){
		if( folderName == platforms[i] && folderName != platform ){
			return true;
		}
	}

	return false;
}

void splitFromLast(std::string toSplit, std::string deliminator, std::string & first, std::string & second){
	size_t found = toSplit.find_last_of(deliminator.c_str());
	first = toSplit.substr(0,found);
	second = toSplit.substr(found+1);
}

void splitFromFirst(std::string toSplit, std::string deliminator, std::string & first, std::string & second){
	size_t found = toSplit.find(deliminator.c_str());
	first = toSplit.substr(0,found );
	second = toSplit.substr(found+deliminator.size());
}


void getFoldersRecursively(const fs::path & path, std::vector < std::string > & folderNames, std::string platform){
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	if (path.extension() != ".framework") {
		for (const auto & entry : fs::directory_iterator(path)) {
			auto f = entry.path();
			if (f.filename().c_str()[0] == '.') continue; // avoid hidden files .DS_Store .vscode .git etc
			if (fs::is_directory(f)  && isFolderNotCurrentPlatform(f.string(), platform) == false ) {
				getFoldersRecursively(f, folderNames, platform);
			}
		}
		folderNames.emplace_back(path.string());
	}
}

void getFrameworksRecursively(const fs::path & path, std::vector < std::string > & frameworks, std::string platform) {
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (f.filename().c_str()[0] == '.') continue; // avoid hidden files .DS_Store .vscode .git etc
		
		if (fs::is_directory(f)) {
			if (f.extension() == ".framework") {
				frameworks.emplace_back(f.string());
			} else {
				if (f.filename() == fs::path("mediaAssets")) continue;
				if (f.extension() == ".xcodeproj") continue;
				if( f.string().rfind("example", 0) == 0) continue;
				//				if (f.filename() == fs::path(".git")) continue;

				getFrameworksRecursively(f, frameworks, platform);
			}
		}
	}
}



void getPropsRecursively(const fs::path & path, std::vector < std::string > & props, const std::string & platform) {
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (f.filename().c_str()[0] == '.') continue; // avoid hidden files .DS_Store .vscode .git etc

		if (fs::is_directory(f)) {
			if (f.filename() == fs::path("mediaAssets")) continue;
			if (f.extension() == ".xcodeproj") continue;
			if( f.string().rfind("example", 0) == 0) continue;
//			if (f.filename() == fs::path(".git")) continue;

			getPropsRecursively(f, props, platform);
		} else {
			if (f.extension() == ".props") {
//				cout << f << endl;
				props.emplace_back(f);
			}
		}
	}
}


void getDllsRecursively(const fs::path & path, std::vector < std::string > & dlls, std::string platform) {
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	if (path.filename().c_str()[0] == '.') return; // avoid hidden files .DS_Store .vscode .git etc
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (fs::is_directory(f)) {
			getDllsRecursively(f, dlls, platform);
		} else {
			if (f.extension() == ".dll") {
				cout << "---->> getDLLs " << f << endl;;

				dlls.emplace_back(f);
			}
		}
	}
}


void getLibsRecursively(const fs::path & path, std::vector < std::string > & libFiles, std::vector < LibraryBinary > & libLibs, std::string platform, std::string arch, std::string target){
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		std::vector<std::string> splittedPath = ofSplitString(f, fs::path("/").make_preferred().string());

//		ofFile temp(dir.getFile(i));
		std::string ext = "";
		std::string first = "";
		splitFromLast(f.string(), ".", first, ext);
		
		if (fs::is_directory(f)) {

			// on osx, framework is a directory, let's not parse it....
			auto stem = f.stem();
			if (ext != "framework") {
				auto archFound = std::find(LibraryBinary::archs.begin(), LibraryBinary::archs.end(), stem);
				if (archFound != LibraryBinary::archs.end()) {
					arch = *archFound;
				} else {
					auto targetFound = std::find(LibraryBinary::targets.begin(), LibraryBinary::targets.end(), stem);
					if (targetFound != LibraryBinary::targets.end()) {
						target = *targetFound;
					}
				}
				getLibsRecursively(f, libFiles, libLibs, platform, arch, target);
			}

		} else {

			bool platformFound = false;

			if(platform!=""){
				for(int j=0;j<(int)splittedPath.size();j++){
					if(splittedPath[j]==platform){
						platformFound = true;
					}
				}
			}

			if (ext == "a" || ext == "lib" || ext == "dylib" || ext == "so" || (ext == "dll" && platform != "vs")){
				if (platformFound){
					libLibs.push_back({ f, arch, target });

					//TODO: THEO hack
					if( platform == "ios" ){ //this is so we can add the osx libs for the simulator builds

						std::string currentPath = f;

						//TODO: THEO double hack this is why we need install.xml - custom ignore ofxOpenCv
						if( currentPath.find("ofxOpenCv") == std::string::npos ){
							ofStringReplace(currentPath, "ios", "osx");
							if( fs::exists(currentPath) ){
								libLibs.push_back({ currentPath, arch, target });
							}
						}
					}
				}
			} else if (ext == "h" || ext == "hpp" || ext == "c" || ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "m" || ext == "mm"){
				libFiles.push_back(f);
			}
		}
	}
}

void fixSlashOrder(std::string & toFix){
	std::replace(toFix.begin(), toFix.end(),'/', '\\');
}


std::string unsplitString (std::vector < std::string > strings, std::string deliminator ){
	std::string result;
	for (int i = 0; i < (int)strings.size(); i++){
		if (i != 0) result += deliminator;
		result += strings[i];
	}
	return result;
}


static fs::path OFRoot { "../../.." };

fs::path getOFRoot(){
	return OFRoot;
}

std::string getAddonsRoot(){
	return ofFilePath::join(getOFRoot(), "addons");
}

void setOFRoot(const fs::path & path){
	OFRoot = path;
}

// FIXME: - in the future this can be the getOFRelPath
fs::path getOFRelPathFS(const fs::path & from) {
	return fs::relative(getOFRoot(), from);
}

std::string getOFRelPath(const std::string & from) {
	return ofFilePath::addTrailingSlash(getOFRelPathFS(from).string());
}

bool checkConfigExists(){
	ofFile config(ofFilePath::join(ofFilePath::getUserHomeDir(),".ofprojectgenerator/config"));
	return config.exists();
}

bool askOFRoot(){
	ofFileDialogResult res = ofSystemLoadDialog("Select the folder of your openFrameworks install",true);
	if (res.fileName == "" || res.filePath == "") return false;

	ofDirectory config(ofFilePath::join(ofFilePath::getUserHomeDir(),".ofprojectgenerator"));
	config.create(true);
	ofFile configFile(ofFilePath::join(ofFilePath::getUserHomeDir(),".ofprojectgenerator/config"),ofFile::WriteOnly);
	configFile << res.filePath;
	return true;
}

std::string getOFRootFromConfig(){
	if(!checkConfigExists()) return "";
	ofFile configFile(ofFilePath::join(ofFilePath::getUserHomeDir(),".ofprojectgenerator/config"),ofFile::ReadOnly);
	ofBuffer filePath = configFile.readToBuffer();
	return filePath.getLines().begin().asString();
}

std::string getTargetString(ofTargetPlatform t){
	switch (t) {
	case OF_TARGET_OSX:
		return "osx";
	case OF_TARGET_MINGW:
		return "msys2";
	case OF_TARGET_WINVS:
		return "vs";
	case OF_TARGET_IOS:
		return "ios";
	case OF_TARGET_ANDROID:
		return "android";
	case OF_TARGET_LINUX:
		return "linux";
	case OF_TARGET_LINUX64:
		return "linux64";
	case OF_TARGET_LINUXARMV6L:
		return "linuxarmv6l";
	case OF_TARGET_LINUXARMV7L:
		return "linuxarmv7l";
	default:
		return "";
	}
}


unique_ptr<baseProject> getTargetProject(ofTargetPlatform targ) {
	switch (targ) {
	case OF_TARGET_OSX:
		return unique_ptr<xcodeProject>(new xcodeProject(getTargetString(targ)));
	case OF_TARGET_MINGW:
		return unique_ptr<QtCreatorProject>(new QtCreatorProject(getTargetString(targ)));
	case OF_TARGET_WINVS:
		return unique_ptr<visualStudioProject>(new visualStudioProject(getTargetString(targ)));
	case OF_TARGET_IOS:
		return unique_ptr<xcodeProject>(new xcodeProject(getTargetString(targ)));
	case OF_TARGET_LINUX:
		return unique_ptr<QtCreatorProject>(new QtCreatorProject(getTargetString(targ)));
	case OF_TARGET_LINUX64:
		return unique_ptr<QtCreatorProject>(new QtCreatorProject(getTargetString(targ)));
	case OF_TARGET_LINUXARMV6L:
		return unique_ptr<QtCreatorProject>(new QtCreatorProject(getTargetString(targ)));
	case OF_TARGET_LINUXARMV7L:
		return unique_ptr<QtCreatorProject>(new QtCreatorProject(getTargetString(targ)));
	case OF_TARGET_ANDROID:
		return unique_ptr<AndroidStudioProject>(new AndroidStudioProject(getTargetString(targ)));
	default:
		return unique_ptr<baseProject>();
	}
}

std::string colorText(const std::string & s, int color) {
	std::string c = std::to_string(color);
	return "\033[1;"+c+"m" + s + "\033[0m";
}

void alert(std::string msg, int color) {
	std::cout << colorText(msg, color) << std::endl;
}

