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

#include "uuidxx.h"

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


std::string LoadFileAsString(const std::string & fn)
{
	std::ifstream fin(fn.c_str());

	if(!fin)
	{
		// throw exception
	}

	std::ostringstream oss;
	oss << fin.rdbuf();

	return oss.str();
}

using std::cout;
using std::endl;

void findandreplaceInTexfile (const of::filesystem::path & fileName, std::string tFind, std::string tReplace ){
//void findandreplaceInTexfile (std::string fileName, std::string tFind, std::string tReplace ){
	cout << "findandreplaceInTexfile " << fileName << endl;
	if (of::filesystem::exists( fileName )) {
		cout << "exists!" << endl;
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

	/*
	std::ifstream ifile(ofToDataPath(fileName).c_str(),std::ios::binary);
	ifile.seekg(0,std::ios_base::end);
	long s=ifile.tellg();
	char *buffer=new char[s];
 	ifile.seekg(0);
	ifile.read(buffer,s);
	ifile.close();
	std::string txt(buffer,s);
	delete[] buffer;
	findandreplace(txt, tFind, tReplace);
	std::ofstream ofile(ofToDataPath(fileName).c_str());
	ofile.write(txt.c_str(),txt.size());
	*/
		//return 0;
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

// todo -- this doesn't use ofToDataPath -- so it's broken a bit.  can we fix?
void getFilesRecursively(const of::filesystem::path & path, std::vector < std::string > & fileNames){
//	cout << "---- getFilesRecursively :: " << path << endl;
	ofDirectory dir;
	dir.listDir(path);
	for (int i = 0; i < dir.size(); i++){
		ofFile temp(dir.getFile(i));
		if (dir.getName(i) == ".svn" || dir.getName(i)==".git") continue; // ignore svn and git
		if (ofIsStringInString(dir.getName(i),".framework")) continue; // ignore frameworks

		if (temp.isFile()){
//			cout << dir.getPath(i) << endl;
			fileNames.push_back(dir.getPath(i));
		} else if (temp.isDirectory()){
//			cout << "this is directory : " << dir.getPath(i) << endl;
			getFilesRecursively(dir.getPath(i), fileNames);
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


void getFoldersRecursively(const of::filesystem::path & path, std::vector < std::string > & folderNames, std::string platform){
	ofDirectory dir;

	if (!ofIsStringInString(path.string(), ".framework")){
		dir.listDir(path);
		for (int i = 0; i < dir.size(); i++){
			ofFile temp(dir.getFile(i));
			if (temp.isDirectory() && isFolderNotCurrentPlatform(temp.getFileName(), platform) == false ){
				getFoldersRecursively(dir.getPath(i), folderNames, platform);
			}
		}
		// FIXME: convert folderNames to path
		folderNames.push_back(path.string());
	}
}


void getFrameworksRecursively(const of::filesystem::path & path, std::vector < std::string > & frameworks, std::string platform){


	ofDirectory dir;
	dir.listDir(path);

	for (int i = 0; i < dir.size(); i++){

		ofFile temp(dir.getFile(i));

		if (temp.isDirectory()){
			//getLibsRecursively(dir.getPath(i), folderNames);

			// on osx, framework is a directory, let's not parse it....
		std::string ext = "";
		std::string first = "";
			splitFromLast(dir.getPath(i), ".", first, ext);
			if (ext != "framework")
				getFrameworksRecursively(dir.getPath(i), frameworks, platform);
			else
				frameworks.push_back(dir.getPath(i));
		}

	}
}



void getPropsRecursively(const of::filesystem::path & path, std::vector < std::string > & props, const std::string & platform) {

	if(!ofDirectory::doesDirectoryExist(path)) return; //check for dir existing before listing to prevent lots of "source directory does not exist" errors printed on console
	ofDirectory dir;
	dir.listDir(path);

	for (auto & temp : dir) {
		if (temp.isDirectory()) {
			//skip example directories - this is needed as we are search all folders in the addons root path
			if( temp.getFileName().rfind("example", 0) == 0) continue;
			getPropsRecursively(temp.path(), props, platform);
		}
		else {
			std::string ext = "";
			std::string first = "";
			splitFromLast(temp.path(), ".", first, ext);
			if (ext == "props") {
				props.push_back(temp.path());
			}
		}

	}
}


void getDllsRecursively(const of::filesystem::path & path, std::vector < std::string > & dlls, std::string platform) {
	ofDirectory dir;
	dir.listDir(path);

	for (auto & temp : dir) {
		if (temp.isDirectory()) {
			getDllsRecursively(temp.path(), dlls, platform);
		}
		else {
			std::string ext = "";
			std::string first = "";
			splitFromLast(temp.path(), ".", first, ext);
			if (ext == "dll") {
				dlls.push_back(temp.path());
			}
		}

	}
}




void getLibsRecursively(const of::filesystem::path & path, std::vector < std::string > & libFiles, std::vector < LibraryBinary > & libLibs, std::string platform, std::string arch, std::string target){
	ofDirectory dir;
	dir.listDir(path);



	for (int i = 0; i < dir.size(); i++){

	std::vector<std::string> splittedPath = ofSplitString(dir.getPath(i), of::filesystem::path("/").make_preferred().string());

		ofFile temp(dir.getFile(i));

		if (temp.isDirectory()){
			//getLibsRecursively(dir.getPath(i), folderNames);

			// on osx, framework is a directory, let's not parse it....
		std::string ext = "";
		std::string first = "";
			auto stem = of::filesystem::path(dir.getFile(i)).stem();
			splitFromLast(dir.getPath(i), ".", first, ext);
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
				getLibsRecursively(dir.getPath(i), libFiles, libLibs, platform, arch, target);
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

			//std::string ext = ofFilePath::getFileExt(temp.getFile(i));
		std::string ext;
		std::string first;
			splitFromLast(dir.getPath(i), ".", first, ext);

			if (ext == "a" || ext == "lib" || ext == "dylib" || ext == "so" || (ext == "dll" && platform != "vs")){
				if (platformFound){
					libLibs.push_back({ dir.getPath(i), arch, target });

					//TODO: THEO hack
					if( platform == "ios" ){ //this is so we can add the osx libs for the simulator builds

						std::string currentPath = dir.getPath(i);

						//TODO: THEO double hack this is why we need install.xml - custom ignore ofxOpenCv
						if( currentPath.find("ofxOpenCv") == std::string::npos ){
							ofStringReplace(currentPath, "ios", "osx");
							if( of::filesystem::exists(currentPath) ){
								libLibs.push_back({ currentPath,arch,target });
							}
						}
					}
				}
			} else if (ext == "h" || ext == "hpp" || ext == "c" || ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "m" || ext == "mm"){
				libFiles.push_back(dir.getPath(i));
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


static std::string OFRoot = "../../..";

std::string getOFRoot(){
	return ofFilePath::removeTrailingSlash(OFRoot);
}

std::string getAddonsRoot(){
	return ofFilePath::join(getOFRoot(), "addons");
}

void setOFRoot(std::string path){
	OFRoot = path;
}

// FIXME: - in the future this can be the getOFRelPath
of::filesystem::path getOFRelPathFS(const of::filesystem::path & from) {
	return of::filesystem::relative(getOFRoot(), from);
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
