
#ifndef VSWINPROJECT_H_
#define VSWINPROJECT_H_

#include "ofConstants.h"
#include "ofAddon.h"
#include "baseProject.h"

class visualStudioProject : public baseProject {

public:
    visualStudioProject(std::string target):baseProject(target){};

    bool createProjectFile();
    bool loadProjectFile();
    bool saveProjectFile();

    void addSrc(std::string srcFile, std::string folder, SrcType type=DEFAULT);
    void addInclude(std::string includeName);
    void addLibrary(const LibraryBinary & lib);
    void addCFLAG(std::string cflag, LibType libType = RELEASE_LIB); // C
    void addCPPFLAG(std::string cppflag, LibType libType = RELEASE_LIB); // C++

    void addAddon(ofAddon & addon);

	static std::string LOG_NAME;

	pugi::xml_document filterXmlDoc;


	void appendFilter(std::string folderName);
    

private:

};

#endif
