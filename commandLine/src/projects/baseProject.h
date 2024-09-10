#pragma once

#include "defines.h"
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

	virtual void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) = 0;
	virtual void addInclude(const fs::path & includeName) = 0;
	virtual void addLibrary(const LibraryBinary & lib) = 0;

	// FIXME: change some strings to const &
	virtual void addLDFLAG(std::string ldflag, LibType libType = RELEASE_LIB){}
	virtual void addCFLAG(std::string cflag, LibType libType = RELEASE_LIB){} // C_FLAGS
	virtual void addCPPFLAG(std::string cppflag, LibType libType = RELEASE_LIB){} // CXX_FLAGS
	virtual void addAfterRule(std::string script){}
	virtual void addDefine(std::string define, LibType libType = RELEASE_LIB) {}

	virtual void addAddon(std::string addon);
	virtual void addAddon(ofAddon & addon);
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

	bool bMakeRelative = false;
	
	bool bOverwrite = true;





	// this shouldn't be called by anyone.  call "create(...), save" etc
private:

	virtual bool createProjectFile()=0;
	virtual bool loadProjectFile()=0;
	virtual bool saveProjectFile()=0;
	
	

	// virtual void renameProject();
	// this should get called at the end.

protected:
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
		
		bool run();
	};

	vector <copyTemplateFile> copyTemplateFiles;
};
