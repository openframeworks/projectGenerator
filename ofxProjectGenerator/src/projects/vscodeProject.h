#pragma once
#include "baseProject.h"
#include <set>

#define QUICK_TEST
#ifdef QUICK_TEST
#include "Utils.h"
#endif

class vscodeProject : public baseProject
{
public:
    vscodeProject(std::string target);

    bool createProjectFile();
    void addInclude(std::string includeName){}
    void addLibrary(const LibraryBinary & lib){}
    void addSrc(std::string srcFile, std::string folder, SrcType type=DEFAULT);
    bool loadProjectFile();
    bool saveProjectFile();
    static std::string LOG_NAME;

#ifdef QUICK_TEST
    virtual std::string getPlatformTemplateDir(){
        //return ofFilePath::join(getOFRoot(),"scripts/templates/" + target);
        return ofFilePath::join(getOFRoot(), "apps/projectGenerator/templates/" + target);
    }
#endif

private:
    void addAddon(ofAddon & addon);
    using baseProject::addAddon;
    std::set<std::string> projectFiles;
};
