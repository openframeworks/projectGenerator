/*
 * CBLinuxProject.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#ifndef CBWINPROJECT_H_
#define CBWINPROJECT_H_

#include "ofConstants.h"
#include "ofAddon.h"
#include "baseProject.h"

class CBWinProject: public baseProject  {
public:
    CBWinProject(std::string target):baseProject(target){};

    bool createProjectFile();
    bool loadProjectFile();
    bool saveProjectFile();

	void addSrc(std::string srcName, std::string folder, SrcType type=DEFAULT);
	void addInclude(std::string includeName);
	void addLibrary(const LibraryBinary & lib);

	std::string getName();
	std::string getPath();

	static std::string LOG_NAME;

private:

};

#endif /* CBLINUXPROJECT_H_ */
