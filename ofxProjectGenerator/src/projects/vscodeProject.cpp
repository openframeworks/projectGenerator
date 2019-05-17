#include "vscodeProject.h"
#include "ofLog.h"
#include "ofFileUtils.h"
#include "Utils.h"
#include <regex>

std::string vscodeProject::LOG_NAME = "vscodeProject";

vscodeProject::vscodeProject(std::string target)
    : baseProject(target){

}

bool vscodeProject::createProjectFile(){
    ofDirectory dir(projectDir);
    if(!dir.exists()) dir.create(true);

    ofFile project(ofFilePath::join(projectDir, projectName + ".code-workspace"));
    std::string src = ofFilePath::join(templatePath,"emptyExample.code-workspace");
    std::string dst = project.path();

    if(!project.exists()){
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying vscode workspace file from " << src << " to " << dst;
            return false;
        }
    }

    ofDirectory vscodeDir(ofFilePath::join(projectDir, ".vscode"));
    if(!vscodeDir.exists()){
        src = ofFilePath::join(templatePath,"vscode");
        dst = vscodeDir.path();
        bool ret = ofFile::copyFromTo(src,dst);
        if(!ret){
            ofLogError(LOG_NAME) << "error copying vscode template from "<< src << " to " << dst;
            return false;
        }else{
            for(auto & f : vscodeDir.getFiles()){
                std::string jsonFilePath = f.getAbsolutePath();
                findandreplaceInTexfile(jsonFilePath, "emptyExample", projectName);
            }
        }
    }
    
    ofFile makefile(ofFilePath::join(projectDir,"Makefile"));
    if(!makefile.exists()){
        src = ofFilePath::join(templatePath,"Makefile");
        dst = makefile.path();
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying Makefile template from " << src << " to " << dst;
            return false;
        }
    }

    ofFile config(ofFilePath::join(projectDir,"config.make"));
    if(!config.exists()){
        src = ofFilePath::join(templatePath,"config.make");
        dst = config.path();
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying config.make template from " << src << " to " << dst;
            return false;
        }
    }


    // handle the relative roots.
    std::string relRoot = getOFRelPath(ofFilePath::removeTrailingSlash(projectDir));
    if (relRoot != "../../../"){
        std::string relPath2 = relRoot;
        relPath2.erase(relPath2.end()-1);
        findandreplaceInTexfile(projectDir + "Makefile", "../../..", relPath2);
        findandreplaceInTexfile(projectDir + "config.make", "../../..", relPath2);
    }

    return true;
}

void vscodeProject::addSrc(std::string srcFile, std::string folder, baseProject::SrcType type){
    projectFiles.insert(srcFile);
}

bool vscodeProject::loadProjectFile(){
    
    // not implemented yet
    return false;
}

bool vscodeProject::saveProjectFile(){
    // not implemented yet
    return true;
}

void vscodeProject::addAddon(ofAddon & addon){
    for(int i=0;i<(int)addons.size();i++){
        if(addons[i].name==addon.name) return;
    }

    addons.push_back(addon);
}
