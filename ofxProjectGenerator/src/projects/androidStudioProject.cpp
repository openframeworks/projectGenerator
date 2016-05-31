#include "androidStudioProject.h"
#include "ofLog.h"
#include "ofFileUtils.h"
#include "Utils.h"
#include <regex>

string AndroidStudioProject::LOG_NAME = "AndroidStudioProject";

AndroidStudioProject::AndroidStudioProject(string target)
    : baseProject(target){

}

bool AndroidStudioProject::createProjectFile(){
    
    // Make sure project name doesn't include "-"
    string packageName = projectName;
    ofStringReplace(packageName, "-", "");
    
    ofDirectory dir(projectDir);
    if(!dir.exists()) dir.create(true);

    // build.gradle
    ofFile gradleFile(ofFilePath::join(projectDir, "build.gradle"));
    string src = ofFilePath::join(templatePath,"build.gradle");
    string dst = gradleFile.path();

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
    

    
    // config.make
    ofFile config(ofFilePath::join(projectDir,"config.make"));
    if(!config.exists()){
        src = ofFilePath::join(templatePath,"config.make");
        dst = config.path();
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying config.make template from " << src << " to " << dst;
        }
    }

    
    // launcher image
   /* ofFile launcher(ofFilePath::join(projectDir,"ic_launcher-web.png"));
    if(!launcher.exists()){
        src = ofFilePath::join(templatePath,"ic_launcher-web.png");
        dst = launcher.path();
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying ic_launcher-web.png from " << src << " to " << dst;
            return false;
        }
    }*/
    
    ofFile makefile(ofFilePath::join(projectDir,"Makefile"));
    if(!makefile.exists()){
        src = ofFilePath::join(templatePath,"Makefile");
        dst = makefile.path();
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying Makefile template from " << src << " to " << dst;
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


    
    // jni folder
    ofDirectory(ofFilePath::join(templatePath,"jni")).copyTo(ofFilePath::join(projectDir,"jni"));

    // res folder
    ofDirectory(ofFilePath::join(templatePath,"res")).copyTo(ofFilePath::join(projectDir,"res"));
    findandreplaceInTexfile(ofFilePath::join(projectDir,"res/values/strings.xml"), "TEMPLATE_APP_NAME", projectName);
    
    // srcJava folder
    ofDirectory(ofFilePath::join(templatePath,"srcJava")).copyTo(ofFilePath::join(projectDir,"srcJava"));
    
    string from = ofFilePath::join(projectDir,"srcJava/cc/openFrameworks/APP_NAME");
    string to = ofFilePath::join(projectDir,"srcJava/cc/openFrameworks/"+projectName);
    
    findandreplaceInTexfile(ofFilePath::join(from,"OFActivity.java"), "TEMPLATE_APP_NAME", projectName);
    ofDirectory(from).moveTo(to, true, true);
    
    return true;

   
}
