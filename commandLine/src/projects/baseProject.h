#pragma once

//#include "defines.h"
#include "ofAddon.h"
#include "pugixml.hpp"
#include <map>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = of::filesystem;

class baseProject {
public:
	enum LibType {
		DEBUG_LIB = 0,
		RELEASE_LIB
	};

	enum SrcType {
		DEFAULT,
		HEADER,
		CPP,
		C,
		OBJC,
		METAL,
		SWIFT,
		JAVA,
		KOTLIN
	};

	std::string toString(SrcType type){
		switch(type){
			case DEFAULT: return "DEFAULT";
			case HEADER: return "HEADER";
			case CPP: return "CPP";
			case C: return "C";
			case OBJC: return "OBJC";
			case METAL: return "METAL";
			case SWIFT: return "SWIFT";
			case JAVA: return "JAVA";
			case KOTLIN: return "KOTLIN";
		}
		return "";
    }


	struct Template {
//		ofDirectory dir;
		fs::path dir;
		std::string name;
		std::vector<string> platforms;
		std::string description;
		std::map<fs::path, fs::path> renames;
		bool operator<(const Template & other) const{
			return dir<other.dir;
		}
	};

	baseProject(const std::string & _target);

	virtual ~baseProject(){}

	bool create(const fs::path & path, std::string templateName="");
	void parseAddons();
	void parseConfigMake();
	bool save();


    void addAddon(const std::string& addon);
    void addAddon(ofAddon & addon);
	virtual void addSrcRecursively(const fs::path & srcPath);

	virtual void restoreBackup(const fs::path & srcPath){};

	bool isPlatformName(const string & platform);

	std::string getName() { return projectName; }
	fs::path getPath() { return projectDir; }

	std::vector<Template> listAvailableTemplates(std::string target);
	std::unique_ptr<baseProject::Template> parseTemplate(const fs::path & templateDir);
	virtual fs::path getPlatformTemplateDir();

	pugi::xml_document doc;
	bool bLoaded;

	fs::path projectDir;
	fs::path templatePath;
	std::string projectName;
	std::string target;

//	bool bMakeRelative = false;

	bool bOverwrite = true;

	virtual void addFramework(const fs::path & path, const fs::path & folder, bool isRelativeToSDK = false){};


#ifdef OFADDON_OUTPUT_JSON_DEBUG
    void saveAddonsToJson(){
        auto dir = ofFilePath::join(projectDir, "addonsJson");
        ofDirectory::createDirectory(dir, false, true);

        for(auto& a: addons){
            ofJson j = a;
            ofSavePrettyJson(ofFilePath::join(dir, a.name+".json"), j);
        }
    }
#endif

	// this shouldn't be called by anyone.  call "create(...), save" etc
private:

	virtual bool createProjectFile()=0;
	virtual bool loadProjectFile()=0;
	virtual bool saveProjectFile()=0;



	// virtual void renameProject();
	// this should get called at the end.

protected:

    virtual void addAddonFrameworks(const ofAddon& addon){}
//    virtual void addAddonXCFrameworks(const ofAddon& addon){}
    virtual void addAddonBegin(const ofAddon& addon){}
    virtual void addAddonLibsPaths(const ofAddon& addon);
	virtual void addAddonIncludePaths(const ofAddon& addon);
	virtual void addAddonLibs(const ofAddon& addon);
	virtual void addAddonCflags(const ofAddon& addon);
	virtual void addAddonCppflags(const ofAddon& addon);
	virtual void addAddonLdflags(const ofAddon& addon);
	virtual void addAddonSrcFiles(ofAddon& addon);
	virtual void addAddonCsrcFiles( ofAddon& addon);
	virtual void addAddonCppsrcFiles( ofAddon& addon);
	virtual void addAddonObjcsrcFiles( ofAddon& addon);
	virtual void addAddonHeadersrcFiles( ofAddon& addon);
    virtual void addAddonDllsToCopy(ofAddon& addon);
    virtual void addAddonDefines(const ofAddon& addon);

    virtual void addAddonProps(const ofAddon& addon) {};

    virtual void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) = 0;
    virtual void addInclude(const fs::path & includeName) = 0;
    virtual void addLibrary(const LibraryBinary & lib) = 0;

    virtual void addLDFLAG(const std::string& ldflag, LibType libType = RELEASE_LIB) = 0;
    virtual void addCFLAG(const std::string& cflag, LibType libType = RELEASE_LIB) = 0; // C_FLAGS
    virtual void addCPPFLAG(const std::string& cppflag, LibType libType = RELEASE_LIB) = 0; // CXX_FLAGS
    virtual void addAfterRule(const std::string& script) = 0;
    virtual void addDefine(const std::string& define, LibType libType = RELEASE_LIB) = 0;

    void copyAddonData(ofAddon& addon);


	virtual void addSrcFiles(ofAddon& addon, const vector<fs::path> &filepaths, SrcType type, bool bFindInFilesToFolder = true);



	void recursiveCopyContents(const fs::path & srcDir, const fs::path & destDir);
	void recursiveTemplateCopy(const fs::path & srcDir, const fs::path & destDir);
	bool recursiveCopy(const fs::path & srcDir, const fs::path & destDir);

	std::vector<ofAddon> addons;
	std::vector<fs::path> extSrcPaths;

	//cached addons - if an addon is requested more than once, avoid loading from disk as it's quite slow
	std::map<std::string,std::map<std::string, ofAddon>> addonsCache; //indexed by [platform][supplied path]
	bool isAddonInCache(const std::string & addonPath, const std::string platform); //is this addon in the mem cache?

	static void replaceAll(std::string& str, const std::string& from, const std::string& to) {
		if(from.empty())
			return;
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
	}

	struct copyTemplateFile {
	public:
		fs::path from;
		fs::path to;
		std::vector <std::pair <string, string>> findReplaces;
		std::vector <std::string> appends;

		bool run();
	};

	vector <copyTemplateFile> copyTemplateFiles;
};
