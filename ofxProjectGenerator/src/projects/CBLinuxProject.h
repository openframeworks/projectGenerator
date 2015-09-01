/*
 * CBLinuxProject.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#ifndef CBLINUXPROJECT_H_
#define CBLINUXPROJECT_H_

#include "ofConstants.h"
#include "ofAddon.h"
#include "CBWinProject.h"
#include "LibraryBinary.h"

class CBLinuxProject: public CBWinProject {
public:
	CBLinuxProject(std::string target):CBWinProject(target){};

    bool createProjectFile();
    void addInclude(std::string includeName){};
    void addLibrary(const LibraryBinary & lib){};

	static std::string LOG_NAME;
};

#endif /* CBLINUXPROJECT_H_ */
