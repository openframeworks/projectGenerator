#pragma once

#include "ofAddon.h"
#include "ofFileUtils.h"
#include "pugixml.hpp"

#include <map>
namespace fs = of::filesystem;
using std::string;


class baseProject {
public:
	enum LibType{
		DEBUG_LIB = 0,
		RELEASE_LIB
	};

	enum SrcType{
		DEFAULT,
		HEADER,
		CPP,
		C,
		OBJC
	};

	struct Template{
		ofDirectory dir;
		string name;
		std::vector<string> platforms;
		string description;
		std::map<fs::path, fs::path> renames;
		bool operator<(const Template & other) const{
			return dir<other.dir;
		}
	};

	baseProject(string _target);

	virtual ~baseProject(){}

	bool create(const fs::path & path, string templateName="");
	void parseAddons();
	void parseConfigMake();
	bool save();

	// this shouldn't be called by anyone.  call "create(...), save" etc
private:

	virtual bool createProjectFile()=0;
	virtual bool loadProjectFile()=0;
	virtual bool saveProjectFile()=0;

	// virtual void renameProject();
	// this should get called at the end.

public:

	virtual void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) = 0;
	virtual void addInclude(string includeName) = 0;
	virtual void addLibrary(const LibraryBinary & lib) = 0;
	virtual void addLDFLAG(string ldflag, LibType libType = RELEASE_LIB){}
	virtual void addCFLAG(string cflag, LibType libType = RELEASE_LIB){} // C_FLAGS
	virtual void addCPPFLAG(string cppflag, LibType libType = RELEASE_LIB){} // CXX_FLAGS
	virtual void addAfterRule(string script){}
	virtual void addDefine(string define, LibType libType = RELEASE_LIB) {}

	virtual void addAddon(string addon);
	virtual void addAddon(ofAddon & addon);
	virtual void addSrcRecursively(const fs::path & srcPath);

	string getName() { return projectName;}
	fs::path getPath() { return projectDir; }

	std::vector<Template> listAvailableTemplates(string target);
	std::unique_ptr<baseProject::Template> parseTemplate(const fs::path & templateDir);
	virtual fs::path getPlatformTemplateDir();

	pugi::xml_document doc;
	bool bLoaded;

	fs::path projectDir;
	fs::path templatePath;
	string projectName;
	string target;
	
	bool bMakeRelative = false;

protected:
	void recursiveCopyContents(const fs::path & srcDir, const fs::path & destDir);
	void recursiveTemplateCopy(const fs::path & srcDir, const fs::path & destDir);
	bool recursiveCopy(const fs::path & srcDir, const fs::path & destDir);

	std::vector<ofAddon> addons;
	std::vector<string> extSrcPaths;

	//cached addons - if an addon is requested more than once, avoid loading from disk as it's quite slow
	std::unordered_map<string,std::unordered_map<string, ofAddon>> addonsCache; //indexed by [platform][supplied path]
	bool isAddonInCache(const string & addonPath, const string platform); //is this addon in the mem cache?
};
