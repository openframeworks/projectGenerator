#pragma once

#include "baseProject.h"
#include <set>

class QtCreatorProject : public baseProject {
public:
	QtCreatorProject(const std::string & target);

	bool createProjectFile();
	void addInclude(std::string includeName){}
	void addLibrary(const LibraryBinary & lib){}
	void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT);
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
