/*
 * CBLinuxProject.h
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */
#pragma once

#include "CBWinProject.h"
#include "LibraryBinary.h"

class CBLinuxProject: public CBWinProject {
public:
	CBLinuxProject(const std::string & target) : CBWinProject(target) {};

	bool createProjectFile();
	void addInclude(std::string includeName){};
	void addLibrary(const LibraryBinary & lib){};

	static std::string LOG_NAME;
};
