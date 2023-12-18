/*
 * Utils.cpp
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#include "Utils.h"
#include "ofUtils.h"

#include "androidStudioProject.h"
#include "CBWinProject.h"
#include "visualStudioProject.h"
#include "VSCodeProject.h"
#include "qtcreatorproject.h"
#include "xcodeProject.h"

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

string generateUUID(const string & input){
	return uuidxx::uuid::Generate().ToString(false);
}

string generateUUID(const fs::path & path){
	return generateUUID(path.string());
}

void findandreplace( string& tInput, string tFind, string tReplace ) {
	size_t uPos = 0;
	size_t uFindLen = tFind.length();
	size_t uReplaceLen = tReplace.length();

	if( uFindLen == 0 ){
		cout << "findandreplace, tFind not found " << tFind << endl;
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

void findandreplaceInTexfile( const fs::path & fileName, string tFind, string tReplace ) {
//	alert("findandreplaceInTexfile " + fileName.string() + " : " + tFind + " : " + tReplace, 33);
	if (fs::exists( fileName )) {
//		cout << "findandreplaceInTexfile " << fileName << " : " << tFind << " : " << tReplace << endl;

		std::ifstream t(fileName);
		std::stringstream buffer;
		buffer << t.rdbuf();
		string bufferStr = buffer.str();
		t.close();

		findandreplace(bufferStr, tFind, tReplace);
		std::ofstream myfile(fileName);
		myfile << bufferStr;
		myfile.close();

	} else {
		cout << "findandreplaceInTexfile file not found " << fileName << endl;
	   ; // some error checking here would be good.
   }
}

bool doesTagAndAttributeExist(pugi::xml_document & doc, string tag, string attribute, string newValue){
	string expression { "//" + tag + "[@" + attribute + "='" + newValue + "']" };
	pugi::xpath_node_set set = doc.select_nodes(expression.c_str());
	if (set.size() != 0){
		return true;
	} else {
		return false;
	}
}

pugi::xml_node appendValue(pugi::xml_document & doc, string tag, string attribute, string newValue, bool overwriteMultiple){
//	alert ("appendValue");

	if (overwriteMultiple == true){
		// find the existing node...
		string expression { "//" + tag + "[@" + attribute + "='" + newValue + "']" };
		pugi::xpath_node node = doc.select_node(expression.c_str());
		if(string(node.node().attribute(attribute.c_str()).value()).size() > 0){ // for some reason we get nulls here?
			// ...delete the existing node
			std::cout << "DELETING: " << node.node().name() << ": " << " " << node.node().attribute(attribute.c_str()).value() << std::endl;
			node.node().parent().remove_child(node.node());
		}
	}

	if (!doesTagAndAttributeExist(doc, tag, attribute, newValue)){
		// otherwise, add it please:
		string expression { "//" + tag + "[@" + attribute + "]" };
		pugi::xpath_node_set add = doc.select_nodes(expression.c_str());
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

// FIXME: ignore .framework folders
void getFilesRecursively(const fs::path & path, std::vector < string > & fileNames){
//	alert ("getFilesRecursively " + path.string());
	if (!fs::exists(path) || !fs::is_directory(path)) return;

	for (const auto & f : dirList(path)) {
		if (fs::is_regular_file(f)) {
			fileNames.emplace_back(f.string());
		}
	}
}

// same function as before, but using fs::path instead of string.
void getFilesRecursively(const fs::path & path, std::vector < fs::path > & fileNames){
	if (!fs::exists(path) || !fs::is_directory(path)) return;

	for (const auto & f : dirList(path)) {
		if (fs::is_regular_file(f)) {
			fileNames.emplace_back(f.string());
		}
	}
}

static std::vector <string> platforms;
bool isFolderNotCurrentPlatform(const string & folderName, const string & platform){
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

	for (auto & p : platforms) {
		if( folderName == p && folderName != platform ){
			return true;
		}
	}
	return false;
}

void splitFromFirst(string toSplit, string deliminator, string & first, string & second){
	size_t found = toSplit.find(deliminator.c_str());
	first = toSplit.substr(0,found );
	second = toSplit.substr(found+deliminator.size());
}

// TODO
void getFoldersRecursively(const fs::path & path, std::vector < fs::path > & folderNames, string platform){
	if (!fs::exists(path)) return;
	if (!fs::is_directory(path)) return;

	// TODO: This can be converted to recursive_directory, but we have to review if the function isFolderNotCurrentPlatform works correctly in this case.

//	for (const auto & entry : fs::recursive_directory_iterator(path)) {
//		auto f = entry.path();
//		if (f.filename().c_str()[0] == '.') continue;
//		if (f.extension() == ".framework") continue;
//	}

	// TODO: disable recursion pending... it is not recursive yet.
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
	alert ("getFrameworksRecursively " + path.string(), 34);
	if (!fs::exists(path) || !fs::is_directory(path)) return;

	for (const auto & f : dirList(path)) {
		if (fs::is_directory(f)) {
			if (f.extension() == ".framework") {
				frameworks.emplace_back(f.string());
			}
		}
	}
}

void getPropsRecursively(const fs::path & path, std::vector < fs::path > & props, const string & platform) {
//	alert ("getPropsRecursively " + path.string(), 34);

	if (!fs::exists(path) || !fs::is_directory(path)) return;

	for (const auto & f : dirList(path)) {
		if (fs::is_regular_file(f)) {
			if (f.extension() == ".props") {
				props.emplace_back(f);
			}
		}
	}
}

void getDllsRecursively(const fs::path & path, std::vector<string> & dlls, string platform) {
//	alert ("getDllsRecursively " + path.string(), 34);
//	if (!fs::exists(path) || !fs::is_directory(path)) return;
	if (!fs::exists(path) || !fs::is_directory(path)) {
//		alert ("not found!");
		return;
	}


	for (const auto & f : dirList(path)) {
		if (fs::is_regular_file(f) && (f.extension() == ".dll" || f.extension() == ".so")) {
			dlls.emplace_back(f.string());
		}
	}
}

void getLibsRecursively(const fs::path & path, std::vector < fs::path > & libFiles, std::vector < LibraryBinary > & libLibs, string platform, string arch, string target) {
//	alert ("getLibsRecursively " + path.string(), 34);
//	alert ("platform " + platform, 34);
//	alert ("arch " + arch, 34);
//	alert ("target " + target, 34);
//	if (!fs::exists(path) || !fs::is_directory(path)) return;
	if (!fs::exists(path) || !fs::is_directory(path)) {
//		alert ("not found!");
		return;
	}

	fs::recursive_directory_iterator it { path };
	fs::recursive_directory_iterator last {  };

	for(; it != last; ++it) {
		auto f = it->path();

		if (fs::is_directory(f)) {
			// on osx, framework is a directory, let's not parse it....
			if (f.extension() == ".framework") {
				it.disable_recursion_pending();
				continue;
			} else {
				auto stem = f.stem();

//				cout << "STEM " << stem << endl;
				auto archFound = std::find(LibraryBinary::archs.begin(), LibraryBinary::archs.end(), stem);
				if (archFound != LibraryBinary::archs.end()) {
					arch = *archFound;
				} else {
					auto targetFound = std::find(LibraryBinary::targets.begin(), LibraryBinary::targets.end(), stem);
					if (targetFound != LibraryBinary::targets.end()) {
						target = *targetFound;
					}
				}
			}
		} else {
			auto ext = f.extension();
			bool platformFound = false;

			if(platform!=""){
				std::vector<string> splittedPath = ofSplitString(f.string(), fs::path("/").make_preferred().string());
				for(int j=0;j<(int)splittedPath.size();j++){
					if(splittedPath[j]==platform){
						platformFound = true;
					}
				}
			}

			if (ext == ".a" || ext == ".lib" || ext == ".dylib" || ext == ".so" ||
				(ext == ".dll" && platform != "vs")){
				if (platformFound){
//					libLibs.emplace_back( f, arch, target );
					libLibs.push_back({ f.string(), arch, target });

					//TODO: THEO hack
					if( platform == "ios" ){ //this is so we can add the osx libs for the simulator builds
						string currentPath = f.string();
						//TODO: THEO double hack this is why we need install.xml - custom ignore ofxOpenCv
						if( currentPath.find("ofxOpenCv") == string::npos ){
							ofStringReplace(currentPath, "ios", "osx");
							if( fs::exists(currentPath) ){
								libLibs.push_back({ currentPath, arch, target });
							}
						}
					}
				}
			} else if (ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".m" || ext == ".mm"){
				libFiles.emplace_back(f);
			}
		}
	}
}

string convertStringToWindowsSeparator(string in) {
	std::replace(in.begin(), in.end(), '/', '\\');
	return in;
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

fs::path getOFRelPath(const fs::path & from) {
	return fs::relative(getOFRoot(), from);
}

bool checkConfigExists(){
	return fs::exists(getUserHomeDir()  / ".ofprojectgenerator/config");
}

// FIXME: remove everything because this function is never used. (FS)
bool askOFRoot(){
	ofFileDialogResult res = ofSystemLoadDialog("Select the folder of your openFrameworks install",true);
	if (res.fileName == "" || res.filePath == "") return false;

	ofDirectory config(getUserHomeDir() / ".ofprojectgenerator");
	config.create(true);
	ofFile configFile(getUserHomeDir() / ".ofprojectgenerator/config",ofFile::WriteOnly);
	configFile << res.filePath;
	return true;
}

// Unused. remove?
string getOFRootFromConfig(){
	if(!checkConfigExists()) return "";
	ofFile configFile( getUserHomeDir() / ".ofprojectgenerator/config",ofFile::ReadOnly);
	ofBuffer filePath = configFile.readToBuffer();
	return filePath.getLines().begin().asString();
}


unique_ptr<baseProject> getTargetProject(const string & targ) {
//	cout << "getTargetProject :" << getTargetString(targ) << endl;
//	typedef xcodeProject pgProject;

	if (targ == "osx" || targ == "ios") {
		return unique_ptr<xcodeProject>(new xcodeProject(targ));
	} else if (targ == "msys2") {
//		return unique_ptr<QtCreatorProject>(new QtCreatorProject(targ));
		return unique_ptr<VSCodeProject>(new VSCodeProject(targ));
	} else if (targ == "vs") {
		return unique_ptr<visualStudioProject>(new visualStudioProject(targ));
	} else if (targ == "linux" ||
			   targ == "linux64" ||
			   targ == "linuxarmv6l" ||
			   targ == "linuxarmv7l" ||
			   targ == "linuxaarch64"
			   ) {
//		return unique_ptr<QtCreatorProject>(new QtCreatorProject(targ));
		return unique_ptr<VSCodeProject>(new VSCodeProject(targ));
	} else if (targ == "android") {
		return unique_ptr<AndroidStudioProject>(new AndroidStudioProject(targ));
	} else if (targ == "vscode") {
		return unique_ptr<VSCodeProject>(new VSCodeProject(targ));
	} else if (targ == "qtcreator") {
			return unique_ptr<QtCreatorProject>(new QtCreatorProject(targ));
	} else {
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



// TODO: Maybe rename this function to a more descriptive name.
vector <fs::path> dirList(const fs::path & path) {
	// map to cache recursive directory listing for subsequent usage
	static std::map<fs::path, vector <fs::path >> dirListMap;

	if (dirListMap.find(path) == dirListMap.end()) {
//		alert ("will list dir " + path.string(), 35);
		fs::recursive_directory_iterator it { path };
		fs::recursive_directory_iterator last {  };

		for(; it != last; ++it) {
			// this wont' allow hidden directories files like .git to be added, and stop recursivity at this folder level.
			if (it->path().filename().c_str()[0] == '.') {
//				alert ("will disable recursion pending " + it->path().filename().string(), 34);
				it.disable_recursion_pending();
				continue;
			}
//			alert ("keep going " + it->path().filename().string(), 33);

			dirListMap[path].emplace_back(it->path());
		}
	} else {
//		alert("IN CACHE " + path.string());
	}
	return dirListMap[path];
}

vector <fs::path> folderList(const fs::path & path) {
	static std::map<fs::path, vector <fs::path >> folderListMap;

	if (folderListMap.find(path) == folderListMap.end()) {
		fs::recursive_directory_iterator it { path };
		fs::recursive_directory_iterator last {  };

		for(; it != last; ++it) {
			// this wont' allow hidden directories files like .git to be added, and stop recursivity at this folder level.
			if ( it->path().filename().c_str()[0] == '.' || it->path().extension() == ".framework" ) {
				it.disable_recursion_pending();
				continue;
			}

			if (fs::is_directory(it->path())) {
				folderListMap[path].emplace_back(it->path());
			}
		}
	} else {
//		alert("IN CACHE " + path.string());
	}
	return folderListMap[path];
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


fs::path getUserHomeDir() {
	return fs::path { ofFilePath::getUserHomeDir() };
}


std::string getPGVersion() {
	return PG_VERSION;
}


bool ofIsPathInPath(const std::filesystem::path & path, const std::filesystem::path & base) {
//	alert ("ofIsPathInPath " + base.string() + " : " + path.string(), 35);
	auto rel = std::filesystem::relative(path, base);
//	cout << (!rel.empty() && rel.native()[0] != '.') << endl;
	return !rel.empty() && rel.native()[0] != '.';
}
