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
//	std::string packageName { projectName };
//	ofStringReplace(packageName, "-", "");


	for (auto & f : vector<fs::path> {
       	"build.gradle",
        "gradle",
        "gradle.properties",
        "local.properties",
        "ofApp/gradle.properties",
        "ofApp/proguard-rules.pro",
        "proguard.cfg",
        "settings.gradle",
	}) {
        copyTemplateFiles.push_back({
			templatePath / f,
			projectDir / f
		});
	}

	copyTemplateFiles.push_back({
		templatePath / "ofApp" / "build.gradle",
		projectDir /  "ofApp" / "build.gradle",
		{ { "emptyExample", projectName } }
	});
	
	// copy and replace where needed
	for (auto & c : copyTemplateFiles) {
		c.run();
	}

	// TODO: try
	fs::create_directory(projectDir /  "ofApp");
	// copy recursively and try not overwrite code.
	try {
		fs::copy(
				 templatePath / "ofApp" / "src",
				 projectDir / "ofApp" / "src",
				 fs::copy_options::recursive | fs::copy_options::skip_existing
		);
	} catch(fs::filesystem_error & e) {
		ofLogError() << "copy failed " << e.what() << endl;
	}

	
	// Leftovers from other

//	try {
//		fs::copy(templatePath, projectDir, fs::copy_options::overwrite_existing | fs::copy_options::recursive);
//	} catch (std::exception& e) {
//		std::cout << e.what();
//		std::cout << "unable to copy android2024 template recursively" << std::endl;
//	}

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
