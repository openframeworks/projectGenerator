/*
 * VSCodeProject.h
 *
 *  Created on: 28/09/2023
 *      Author: Dimitre Lima
 */

#pragma once

#include "baseProject.h"

class VSCodeProject: public baseProject {
public:
	VSCodeProject(const std::string & target) : baseProject(target) {};

	bool createProjectFile();
	bool loadProjectFile();
	bool saveProjectFile();

	void addSrc(const fs::path & srcName, const fs::path & folder, SrcType type=DEFAULT);
	void addInclude(std::string includeName);
	void addLibrary(const LibraryBinary & lib);

	void addAddon(ofAddon & addon);

	static std::string LOG_NAME;

private:

};
