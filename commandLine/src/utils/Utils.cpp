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

string generateUUID(string input){
	return uuidxx::uuid::Generate().ToString(false);
}

void findandreplace( string& tInput, string tFind, string tReplace ) {
	size_t uPos = 0;
	size_t uFindLen = tFind.length();
	size_t uReplaceLen = tReplace.length();

	if( uFindLen == 0 ){
		return;
	}

	for( ;(uPos = tInput.find( tFind, uPos )) != string::npos; ){
		tInput.replace( uPos, uFindLen, tReplace );
		uPos += uReplaceLen;
	}
}

string LoadFileAsString(const string & fn) {
	std::ifstream fin(fn.c_str());
	if(!fin) {
		// throw exception
	}
	std::ostringstream oss;
	oss << fin.rdbuf();
	return oss.str();
}

void findandreplaceInTexfile (const fs::path & fileName, string tFind, string tReplace ){
//	cout << "findandreplaceInTexfile " << fileName << " : " << tFind << " : " << tReplace << endl;
	if (fs::exists( fileName )) {
		std::ifstream t(fileName.c_str());
		std::stringstream buffer;
		buffer << t.rdbuf();
		string bufferStr = buffer.str();
		t.close();
		findandreplace(bufferStr, tFind, tReplace);
		std::ofstream myfile;
		myfile.open (fileName.c_str());
		myfile << bufferStr;
		myfile.close();

	} else {
		cout << "findandreplaceInTexfile file not found " << fileName << endl;
	   ; // some error checking here would be good.
   }
}

bool doesTagAndAttributeExist(pugi::xml_document & doc, string tag, string attribute, string newValue){
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

pugi::xml_node appendValue(pugi::xml_document & doc, string tag, string attribute, string newValue, bool overwriteMultiple){
	if (overwriteMultiple == true){
		// find the existing node...
		char xpathExpression[1024];
		sprintf(xpathExpression, "//%s[@%s='%s']", tag.c_str(), attribute.c_str(), newValue.c_str());
		pugi::xpath_node node = doc.select_node(xpathExpression);
		if(string(node.node().attribute(attribute.c_str()).value()).size() > 0){ // for some reason we get nulls here?
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

// TODO: This can be removed in the future, but not now.
// Still needed now because srcFiles is vector of string.
// it can't be changed to fs::path because of addReplaceStringVector
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
			fileNames.emplace_back(f.string());
		}
	}
}

void getFilesRecursively(const fs::path & path, std::vector < fs::path > & fileNames){
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (f.filename().c_str()[0] == '.') continue; // avoid hidden files .DS_Store .vscode  .git etc
		if (ofIsStringInString(f.filename().string(),".framework")) continue; // ignore frameworks
		
		if (fs::is_directory(f)) {
			getFilesRecursively(f, fileNames);
		} else {
			fileNames.emplace_back(f);
		}
	}
}

static std::vector <string> platforms;
bool isFolderNotCurrentPlatform(string folderName, string platform){
	if( platforms.size() == 0 ){
		platforms = {
			"osx",
			"msys2",
			"vs",
			"ios",
			"linux",
			"linux64",
			"android",
			"iphone",
		};
	}

	for(int i = 0; i < platforms.size(); i++){
		if( folderName == platforms[i] && folderName != platform ){
			return true;
		}
	}
	return false;
}

void splitFromLast(string toSplit, string deliminator, string & first, string & second){
	size_t found = toSplit.find_last_of(deliminator.c_str());
	first = toSplit.substr(0,found);
	second = toSplit.substr(found+1);
}

void splitFromFirst(string toSplit, string deliminator, string & first, string & second){
	size_t found = toSplit.find(deliminator.c_str());
	first = toSplit.substr(0,found );
	second = toSplit.substr(found+deliminator.size());
}


void getFoldersRecursively(const fs::path & path, std::vector < string > & folderNames, string platform){
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

void getFrameworksRecursively(const fs::path & path, std::vector < string > & frameworks, string platform) {
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (f.filename().c_str()[0] == '.') continue; // avoid hidden files .DS_Store .vscode .git etc
		
		if (fs::is_directory(f)) {
			if (f.extension() == ".framework") {
//				cout << "adding framework " << f << endl;
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

void getPropsRecursively(const fs::path & path, std::vector < fs::path > & props, const string & platform) {
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
//				cout << ">>> getPropsRecursively FOUND PROP::: " << f << endl;
//				cout << ">>> path = " << path << endl;
				props.emplace_back(f);
			}
		}
	}
}

void getDllsRecursively(const fs::path & path, std::vector < string > & dlls, string platform) {
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	if (path.filename().c_str()[0] == '.') return; // avoid hidden files .DS_Store .vscode .git etc
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (fs::is_directory(f)) {
			getDllsRecursively(f, dlls, platform);
		} else {
			if (f.extension() == ".dll") {
				alert("getDLLs " + f.string());

				dlls.emplace_back(f.string());
			}
		}
	}
}

void getLibsRecursively(const fs::path & path, std::vector < string > & libFiles, std::vector < LibraryBinary > & libLibs, string platform, string arch, string target) {
//	cout << ">> getLibsRecursively " << path << endl;
	if (!fs::exists(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	if (!fs::is_directory(path)) return;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		std::vector<string> splittedPath = ofSplitString(f.string(), fs::path("/").make_preferred().string());

//		ofFile temp(dir.getFile(i));
		string ext = "";
		string first = "";
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

			if (ext == "a" || ext == "lib" || ext == "dylib" || ext == "so" ||
				(ext == "dll" && platform != "vs")){
				if (platformFound){
//					libLibs.emplace_back( f, arch, target );

					// XAXA
//					cout << "----" << endl;
//					cout << f.string() << endl;
					
					libLibs.push_back({ f.string(), arch, target});

					//TODO: THEO hack
					if( platform == "ios" ){ //this is so we can add the osx libs for the simulator builds

						string currentPath = f.string();

						//TODO: THEO double hack this is why we need install.xml - custom ignore ofxOpenCv
						if( currentPath.find("ofxOpenCv") == string::npos ){
							ofStringReplace(currentPath, "ios", "osx");
							if( fs::exists(currentPath) ){
//								libLibs.emplace_back( currentPath, arch, target );
								libLibs.push_back({ currentPath, arch, target });
							}
						}
					}
				}
			} else if (ext == "h" || ext == "hpp" || ext == "c" || ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "m" || ext == "mm"){
				libFiles.emplace_back(f.string());
			}
		}
	}
}

void fixSlashOrder(string & toFix){
	std::replace(toFix.begin(), toFix.end(),'/', '\\');
}

string unsplitString (std::vector < string > strings, string deliminator ){
	string result;
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

void setOFRoot(const fs::path & path){
	OFRoot = path;
}

string convertStringToWindowsSeparator(string in) {
	std::replace(in.begin(), in.end(), '/', '\\');
	return in;
}

vector<string> fileToStrings (const fs::path & file) {
	vector<string> out;
	if (fs::exists(file)) {
		std::ifstream thisFile(file);
		string line;
		while(getline(thisFile, line)){
			out.emplace_back(line);
		}
	}
	return out;
}

fs::path getOFRelPath(const fs::path & from) {
	return fs::relative(getOFRoot(), from);
}

bool checkConfigExists(){
	return fs::exists(fs::path { ofFilePath::getUserHomeDir() }  / ".ofprojectgenerator/config");
}

bool askOFRoot(){
	ofFileDialogResult res = ofSystemLoadDialog("Select the folder of your openFrameworks install",true);
	if (res.fileName == "" || res.filePath == "") return false;

	// FIXME: FS
	ofDirectory config(ofFilePath::join(ofFilePath::getUserHomeDir(),".ofprojectgenerator"));
	config.create(true);
	ofFile configFile(ofFilePath::join(ofFilePath::getUserHomeDir(),".ofprojectgenerator/config"),ofFile::WriteOnly);
	configFile << res.filePath;
	return true;
}

string getOFRootFromConfig(){
	if(!checkConfigExists()) return "";
	ofFile configFile(ofFilePath::join(ofFilePath::getUserHomeDir(),".ofprojectgenerator/config"),ofFile::ReadOnly);
	ofBuffer filePath = configFile.readToBuffer();
	return filePath.getLines().begin().asString();
}

string getTargetString(ofTargetPlatform t){
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
		case OF_TARGET_LINUXAARCH64:
			return "linuxaarch64";
		default:
			return "";
	}
}

unique_ptr<baseProject> getTargetProject(ofTargetPlatform targ) {
//	cout << "getTargetProject :" << getTargetString(targ) << endl;
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
	case OF_TARGET_LINUXAARCH64:
		return unique_ptr<QtCreatorProject>(new QtCreatorProject(getTargetString(targ)));
	case OF_TARGET_ANDROID:
		return unique_ptr<AndroidStudioProject>(new AndroidStudioProject(getTargetString(targ)));
	default:
		return unique_ptr<baseProject>();
	}
}

string colorText(const string & s, int color) {
	string c = std::to_string(color);
	return "\033[1;"+c+"m" + s + "\033[0m";
}

void alert(string msg, int color) {
	std::cout << colorText(msg, color) << std::endl;
}

bool ofIsPathInPath(const fs::path& basePath, const fs::path& subPath) {
	return (std::search(basePath.begin(), basePath.end(), subPath.begin(), subPath.end()) != basePath.end());
}
