#include "android2024.h"
#include "ofLog.h"
#include "ofFileUtils.h"
#include "Utils.h"
#include "ofUtils.h"
#include <regex>

std::string android2024Project::LOG_NAME = "android2024Project";

android2024Project::android2024Project(const std::string & target) : baseProject(target) {}

bool android2024Project::createProjectFile(){
	// Make sure project name doesn't include "-"
	std::string packageName { projectName };
	ofStringReplace(packageName, "-", "");

	if (!fs::exists(projectDir)) {
		fs::create_directory(projectDir);
	}

//	std::vector <std::string> fileNames {
//		"build.gradle",
//		"settings.gradle",
//		"AndroidManifest.xml",
//		".gitignore",
//		"gradlew",
//		"gradlew.bat",
//	};
	
	try {
		fs::copy(templatePath, projectDir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
	} catch (std::exception& e) {
		std::cout << e.what();
		std::cout << "unable to copy android2024 template recursively" << std::endl;
	}

//	for (auto & f : fileNames) {
//		fs::path to { projectDir / f };
//		if (!fs::exists(to)) {
//			fs::path from { templatePath / f };
//			try {
//				fs::copy(from, to);
//			} catch(fs::filesystem_error & e) {
//				if (f == "AndroidManifest.xml") {
//					findandreplaceInTexfile(to, "TEMPLATE_PACKAGE_NAME", packageName);
//				} else {
//					ofLogError(LOG_NAME) << "error copying template from " << from << " to " << to << e.what();
//				}
//			}
//		}
//	}
//
//	for (auto & p : { string("res") , string("srcJava"), string("gradle") }) {
//		fs::copy (templatePath / p, projectDir / p, fs::copy_options::recursive);
//	}
//
//	findandreplaceInTexfile( projectDir / "res/values/strings.xml", "TEMPLATE_APP_NAME", projectName);
//
//	fs::path from { projectDir / "srcJava/cc/openframeworks/APP_NAME" };
//	fs::path to { projectDir / ("srcJava/cc/openframeworks/" + projectName) };
//	// TODO: try catch
//	fs::rename ( from, to );
//	findandreplaceInTexfile(to / "OFActivity.java", "TEMPLATE_APP_NAME", projectName);

	return true;
}
