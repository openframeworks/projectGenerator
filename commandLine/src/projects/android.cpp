#include "android.h"
#include "ofLog.h"
#include "Utils.h"
#include "ofUtils.h"
#include <regex>

std::string androidProject::LOG_NAME = "androidProject";

androidProject::androidProject(const std::string & target) : baseProject(target) {}

bool androidProject::createProjectFile(){

	bOverwrite = false;
	for (auto & f : vector<fs::path> {
		"build.gradle",
		"gradle",
		"ofApp/gradle.properties",
		"ofApp/proguard-rules.pro",
		"ofApp/src/AndroidManifest.xml",
		"ofApp/src/ic_launcher-playstore.png",
		"proguard.cfg",
		"settings.gradle",
		"template.config",
		"src/main.cpp",
	}) {
		copyTemplateFiles.push_back({
			templatePath / f,
			projectDir / f
		});
	}
	for (auto & f : vector<fs::path> {
		"src/ofApp.h",
		"src/ofApp.cpp",
		"ofApp/src/CMakeLists.txt",
		"ofApp/src/java/cc/openframeworks/android/OFActivity.java",
		"local.properties",
		"gradle.properties",
		"ofApp/build.gradle",
	}) {
		if (!fs::exists(projectDir / f)) {
			copyTemplateFiles.push_back({
				templatePath / f,
				projectDir / f
			});
		}
	}
	copyTemplateFiles.push_back({
		templatePath / "ofApp" / "build.gradle",
		projectDir / "ofApp" / "build.gradle",
		{ { "emptyExample", projectName } }
	});
	for (auto & c : copyTemplateFiles) {
		c.run();
	}

	try {
		fs::create_directories(projectDir / "ofApp");
	} catch (const std::exception & e) {
		ofLogError(LOG_NAME) << "Error creating directories: " << e.what();
		return false;
	}

	// Copy the `src/` folder from the template
	try {
		fs::copy(
			templatePath / "ofApp",
			projectDir / "ofApp",
			fs::copy_options::recursive | (bOverwrite ? fs::copy_options::overwrite_existing : fs::copy_options::update_existing)
		);
	} catch (fs::filesystem_error & e) {
		ofLogError(LOG_NAME) << "Copy failed: " << e.what();
		return false;
	}
	
	

	try {
		fs::copy(templatePath / "ofApp/src/res", projectDir / "ofApp/src/res", fs::copy_options::recursive);
	} catch (fs::filesystem_error & e) {
		ofLogError(LOG_NAME) << "Error copying res folder: " << e.what();
	}

	// Copy additional Android-specific template folders
	for (const auto & p : { "srcJava", "gradle" }) {  // ✅ Removed "res" from this loop
		try {
			fs::copy(templatePath / p, projectDir / p, fs::copy_options::recursive);
		} catch (fs::filesystem_error & e) {
			ofLogError(LOG_NAME) << "Error copying " << p << ": " << e.what();
		}
	}

	findandreplaceInTexfile(projectDir / "ofApp/res/values/strings.xml", "TEMPLATE_APP_NAME", projectName);
	fs::path from = projectDir / "ofApp/src/java/cc/openframeworks/android";
	fs::path to = projectDir / ("ofApp/src/java/cc/openframeworks/android");
	
	try {
		fs::create_directories(to.parent_path());
		fs::rename(from, to);
	} catch (const std::exception & e) {
		ofLogError(LOG_NAME) << "Error renaming package directory: " << e.what();
	}

	ofLogNotice(LOG_NAME) << "Android Project created successfully!";
	return true;
}
