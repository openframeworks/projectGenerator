#pragma once
#include "baseProject.h"

class QtCreatorProject : public baseProject
{
public:
    QtCreatorProject(std::string target);

    bool createProjectFile();
    void addInclude(std::string includeName){}
    void addLibrary(const LibraryBinary & lib){}
    void addSrc(std::string srcFile, std::string folder, SrcType type=DEFAULT);
    bool loadProjectFile();
    bool saveProjectFile();
    static std::string LOG_NAME;

private:
    void addAddon(ofAddon & addon);
    using baseProject::addAddon;
    std::set<std::string> qbsProjectFiles;
    ofBuffer qbs;
    std::string originalFilesStr, originalAddonsStr;
};
