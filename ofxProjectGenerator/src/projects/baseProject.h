

#pragma once

#include <set>

#include "ofAddon.h"
#include "ofConstants.h"
#include "ofFileUtils.h"
#include "pugixml.hpp"

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

    baseProject(std::string _target);

    virtual ~baseProject(){};

    bool create(std::string path, std::string templateName="standard");
    void parseAddons();
    bool save();

    // this shouldn't be called by anyone.  call "create(...), save" etc
private:

    virtual bool createProjectFile()=0;
    virtual bool loadProjectFile()=0;
    virtual bool saveProjectFile()=0;

    // virtual void renameProject();
    // this should get called at the end.

public:

    virtual void addSrc(std::string srcFile, std::string folder, SrcType type=DEFAULT) = 0;
    virtual void addInclude(std::string includeName) = 0;
    virtual void addLibrary(const LibraryBinary & lib) = 0;
    virtual void addLDFLAG(std::string ldflag, LibType libType = RELEASE_LIB){}
    virtual void addCFLAG(std::string cflag, LibType libType = RELEASE_LIB){}; // C_FLAGS
    virtual void addCPPFLAG(std::string cppflag, LibType libType = RELEASE_LIB){}; // CXX_FLAGS

    virtual void addAddon(std::string addon);
	virtual void addAddon(ofAddon & addon);

	std::string getName() { return projectName;};
	std::string getPath() { return projectDir; };

	vector<ofDirectory> listAvailableTemplates(std::string target);
	virtual std::string getPlatformTemplateDir();

    pugi::xml_document doc;
    bool bLoaded;

    std::string projectDir;
    std::string projectName;
    std::string templatePath;
    std::string target;

protected:

    vector<ofAddon> addons;
};


