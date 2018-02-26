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
    
    ofDirectory dir(projectDir);
    if(!dir.exists()) dir.create(true);

    // build.gradle
    ofFile gradleFile(ofFilePath::join(projectDir, "build.gradle"));
    std::string src = ofFilePath::join(templatePath,"build.gradle");
    std::string dst = gradleFile.path();

    if(!gradleFile.exists()){
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying gradle template from " << src << " to " << dst;
        }
    }
    
    // settings.gradle
    ofFile settings(ofFilePath::join(projectDir, "settings.gradle"));
    if(!settings.exists()){
        src = ofFilePath::join(templatePath,"settings.gradle");
        dst = settings.path();
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying settings gradle template from " << src << " to " << dst;
        }
    }
    
    // Android.manifest
    ofFile manifest(ofFilePath::join(projectDir,"AndroidManifest.xml"));
    if(!manifest.exists()){
        src = ofFilePath::join(templatePath,"AndroidManifest.xml");
        dst = manifest.path();
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying Android.manifest template from " << src << " to " << dst;
        } else {
            findandreplaceInTexfile(dst, "TEMPLATE_PACKAGE_NAME", packageName);
        }
    }

    // gitignore
    ofFile gitignore(ofFilePath::join(projectDir,".gitignore"));
    if(!gitignore.exists()){
        src = ofFilePath::join(templatePath,".gitignore");
        dst = gitignore.path();
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying gitignore template from " << src << " to " << dst;
        }
    }

    // res folder
    ofDirectory(ofFilePath::join(templatePath,"res")).copyTo(ofFilePath::join(projectDir,"res"));
    findandreplaceInTexfile(ofFilePath::join(projectDir,"res/values/strings.xml"), "TEMPLATE_APP_NAME", projectName);
    
    // srcJava folder
    ofDirectory(ofFilePath::join(templatePath,"srcJava")).copyTo(ofFilePath::join(projectDir,"srcJava"));
    
    std::string from = ofFilePath::join(projectDir,"srcJava/cc/openframeworks/APP_NAME");
    std::string to = ofFilePath::join(projectDir,"srcJava/cc/openframeworks/"+projectName);
    
    findandreplaceInTexfile(ofFilePath::join(from,"OFActivity.java"), "TEMPLATE_APP_NAME", projectName);
    ofDirectory(from).moveTo(to, true, true);

    // Gradle wrapper
    ofDirectory(ofFilePath::join(templatePath,"gradle")).copyTo(ofFilePath::join(projectDir,"gradle"));
    ofFile::copyFromTo(ofFilePath::join(templatePath,"gradlew"), ofFilePath::join(projectDir,"gradlew"));
    ofFile::copyFromTo(ofFilePath::join(templatePath,"gradlew.bat"), ofFilePath::join(projectDir,"gradlew.bat"));
    
    return true;

   
}
