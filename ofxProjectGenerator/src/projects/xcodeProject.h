

#pragma once

#include "baseProject.h"
#include "pugixml.hpp"

class xcodeProject : public baseProject {

public:

    xcodeProject(std::string target);

private:

    bool createProjectFile();
    bool loadProjectFile();
    bool saveProjectFile();
    void saveMakefile();

public:

    void addSrc(std::string srcFile, std::string folder, SrcType type=DEFAULT);
    void addInclude(std::string includeName);
    void addLibrary(const LibraryBinary & lib);
    void addLDFLAG(std::string ldflag, LibType libType = RELEASE_LIB);
    void addCFLAG(std::string cflag, LibType libType = RELEASE_LIB); // Other C Flags
    void addCPPFLAG(std::string cppflag, LibType libType = RELEASE_LIB); // Other C++ Flags
    void addAfterRule(std::string script);
	void addDefine(std::string define, LibType libType = RELEASE_LIB);
    
    // specific to OSX
    void addFramework(std::string name, std::string path, std::string folder="");  // folder if for non system frameworks
    
        
    
    void addAddon(ofAddon & addon);

    void saveWorkspaceXML();
    void saveScheme();
    void renameProject();

    std::string srcUUID;
    std::string addonUUID;
    std::string localAddonUUID;
    std::string resourcesUUID;
    std::string buildPhaseUUID;                 // note this UUID keeps refs all things to build
    std::string frameworksUUID;
    std::string buildPhaseResourcesUUID;
    std::string frameworksBuildPhaseUUID;
    std::string afterPhaseUUID;
    std::string buildPhasesUUID;                // note this UUID is in an array of *all* the build steps 
    
    
    

    pugi::xml_node findOrMakeFolderSet( pugi::xml_node nodeToAddTo, std::vector < std::string > & folders, std::string pathForHash);
	pugi::xml_node insertPoint;         // where are we inserting items (at the second dict tag,
                                        // /plist[1]/dict[1]/dict[2])
    bool findArrayForUUID(std::string UUID, pugi::xml_node & nodeMe);

};




