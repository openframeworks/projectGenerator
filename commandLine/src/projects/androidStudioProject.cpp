#include "androidStudioProject.h"
#include "ofLog.h"
#include "ofFileUtils.h"
#include "Utils.h"
#include <regex>

std::string AndroidStudioProject::LOG_NAME = "AndroidStudioProject";

AndroidStudioProject::AndroidStudioProject(std::string target)
	: baseProject(target){
}

bool AndroidStudioProject::createProjectFile(){

	// Make sure project name doesn't include "-"
	std::string packageName = projectName;
	ofStringReplace(packageName, "-", "");

	if (!fs::exists(projectDir)) {
		fs::create_directory(projectDir);
	}

	vector <string> fileNames = {
		"build.gradle",
		"settings.gradle",
		"AndroidManifest.xml",
		".gitignore"
	};
	
	for (auto & f : fileNames) {

		fs::path from { templatePath / f };
		fs::path to { projectDir / f };

		if (!fs::exists(to)) {
			try {
				fs::copy(from, to);
			} catch(fs::filesystem_error& e) {
				if (f == "AndroidManifest.xml") {
					findandreplaceInTexfile(to, "TEMPLATE_PACKAGE_NAME", packageName);
				} else {
					ofLogError(LOG_NAME) << "error copying template from " << from << " to " << to << e.what();
				}
			}
		}
	}

	// res folder
	ofDirectory( templatePath / "res" ).copyTo( projectDir / "res" );
	findandreplaceInTexfile( projectDir / "res/values/strings.xml", "TEMPLATE_APP_NAME", projectName);

	// srcJava folder
	ofDirectory( templatePath / "srcJava" ).copyTo( projectDir / "srcJava" );

	fs::path from = projectDir / "srcJava/cc/openframeworks/APP_NAME";
	fs::path to = projectDir / ("srcJava/cc/openframeworks/"+projectName);

	findandreplaceInTexfile(from / "OFActivity.java", "TEMPLATE_APP_NAME", projectName);
	ofDirectory(from).moveTo(to, true, true);

	// Gradle wrapper
	ofDirectory(templatePath / "gradle").copyTo( projectDir / "gradle" );
	ofFile::copyFromTo(templatePath / "gradlew",  projectDir / "gradlew" );
	ofFile::copyFromTo(templatePath / "gradlew.bat",  projectDir / "gradlew.bat" );

	return true;
}
