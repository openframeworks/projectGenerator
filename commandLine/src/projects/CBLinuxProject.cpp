/*
 * CBLinuxProject.cpp
 *
 *  Created on: 28/12/2011
 *      Author: arturo
 */

#include "CBLinuxProject.h"
#include "ofFileUtils.h"
#include "ofLog.h"
#include "Utils.h"

std::string CBLinuxProject::LOG_NAME { "CBLinuxProject" };

bool CBLinuxProject::createProjectFile(){
	// FIXME: This only exists here, not other projects. I think it should be removed
	if (!fs::exists(projectDir)) {
		fs::create_directory(projectDir);
	}

	vector < std::pair <fs::path, fs::path > > fromTo {
		{ templatePath / ("emptyExample_" + target + ".cbp"),   		projectDir / (projectName + ".cbp") },
		{ templatePath / ("emptyExample_" + target + ".workspace"), 	projectDir / (projectName + ".workspace") },
		{ templatePath / "Makefile", 	projectDir / "Makefile" },
		{ templatePath / "config.make", 	projectDir / "config.make" },
	};

	for (auto & p : fromTo) {
		try {
			fs::copy_file(p.first, p.second, fs::copy_options::overwrite_existing);
		} catch(fs::filesystem_error& e) {
			ofLogError(LOG_NAME) << "error copying template file " << p.first << " : " << p.second << e.what();
			return false;
		}
	}

	// fromTo[0].second is cbp project in destination path.
	findandreplaceInTexfile(fromTo[0].second, "emptyExample", projectName);
	findandreplaceInTexfile(fromTo[1].second, "emptyExample", projectName);

	// Calculate OF Root in relation to each project (recursively);
	auto relRoot = fs::relative((fs::current_path() / getOFRoot()), projectDir);

	if (!fs::equivalent(relRoot, "../../..")) {
		string root = relRoot.string();

		std::string root2 = root;

		// TODO: check this
//		root2.erase(root2.end()-1);
		findandreplaceInTexfile(projectDir / "Makefile", "../../..", root2);
		findandreplaceInTexfile(projectDir / "config.make", "../../..", root2);

		findandreplaceInTexfile(projectDir / (projectName + ".workspace"), "../../../", root);
		findandreplaceInTexfile(projectDir / (projectName + ".cbp"), "../../../", root);
	}

	return true;
}
