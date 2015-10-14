#include "qtcreatorproject.h"
#include "ofLog.h"
#include "ofFileUtils.h"
#include "Utils.h"
#include <regex>

string QtCreatorProject::LOG_NAME = "QtCreatorProject";

QtCreatorProject::QtCreatorProject(string target)
    : baseProject(target){

}

bool QtCreatorProject::createProjectFile(){
    ofDirectory dir(projectDir);
    if(!dir.exists()) dir.create(true);

    ofFile project(ofFilePath::join(projectDir, projectName + ".qbs"));
    string src = ofFilePath::join(templatePath,"qtcreator.qbs");
    string dst = project.path();

    if(!project.exists()){
        if(!ofFile::copyFromTo(src,dst)){
            ofLogError(LOG_NAME) << "error copying qbs template from " << src << " to " << dst;
            return false;
        }else{
            findandreplaceInTexfile(dst, "emptyExample", projectName);
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
    string relRoot = getOFRelPath(ofFilePath::removeTrailingSlash(projectDir));
    if (relRoot != "../../../"){
        string relPath2 = relRoot;
        relPath2.erase(relPath2.end()-1);
        findandreplaceInTexfile(projectDir + "qtcreator.qbs", "../../..", relPath2);
        findandreplaceInTexfile(projectDir + "Makefile", "../../..", relPath2);
        findandreplaceInTexfile(projectDir + "config.make", "../../..", relPath2);
    }

    return true;
}

void QtCreatorProject::addSrc(std::string srcFile, std::string folder, baseProject::SrcType type){
    qbsProjectFiles.insert(srcFile);
}

bool QtCreatorProject::loadProjectFile(){
    ofFile project(projectDir + projectName + ".qbs",ofFile::ReadOnly,true);
    if(!project.exists()){
        ofLogError(LOG_NAME) << "error loading" << project.path() << "doesn't exist";
        return false;
    }
    auto ret = qbs.set(project);
    // parse files in current .qbs
    std::regex filesregex("files[ \t\r\n]*:[ \t\r\n]*\\[[ \t\r\n]*([\"'][^\\]\"']*[\"'][ \t\r\n]*,?[ \t\r\n]*)*\\]");
    std::smatch matches;
    std::string qbsStr = qbs.getText();
    if(std::regex_search(qbsStr, matches, filesregex)){
        std::string fullmatch = matches[0];
        originalFilesStr = fullmatch;
        while(std::regex_search(fullmatch, matches, std::regex("[ \t\r\n]*[\"']([^\\]\"']*)[\"'][ \t\r\n]*,?"))){
            qbsProjectFiles.insert(matches[1]);
            fullmatch = matches.suffix().str();
        }
    }

    // parse addons in current .qbs
    std::regex addonsregex("of\\.addons[ \t\r\n]*:[ \t\r\n]*\\[[ \t\r\n]*([\"'][^\\]\"']*[\"'][ \t\r\n]*,?[ \t\r\n]*)*\\]");
    if(std::regex_search(qbsStr, matches, addonsregex)){
        std::string fullmatch = matches[0];
        originalAddonsStr = fullmatch;
        while(std::regex_search(fullmatch, matches, std::regex("[ \t\r\n]*[\"']([^\\]\"']*)[\"'][ \t\r\n]*,?"))){
            bool alreadyExists=false;
            for(auto & a: addons){
                auto addonStr = a.isLocalAddon?a.addonPath:a.name;
                if(addonStr==matches[1]){
                    alreadyExists = true;
                    break;
                }
            }
            if(!alreadyExists){
                addAddon(matches[1].str());
                fullmatch = matches.suffix().str();
            }
        }
    }
    return ret;
}

bool QtCreatorProject::saveProjectFile(){
    auto qbsStr = qbs.getText();

    // create files str with all files
    std::string filesStr = "files: [\n";
    for(auto & f: qbsProjectFiles){
        filesStr += "            '" + f + "',\n";
    }
    filesStr += "        ]";

    // create addons str with all addons
    std::string addonsStr = "of.addons: [\n";
    for(auto & a: addons){
        auto addonStr = a.isLocalAddon?a.addonPath:a.name;
        addonsStr += "            '" + addonStr + "',\n";
    }
    addonsStr += "        ]";

    // if there were no addons str just append it to files
    if(originalAddonsStr!=""){
        ofStringReplace(qbsStr,originalAddonsStr,addonsStr);
    }else{
        filesStr += "\n\n        " + addonsStr;
    }
    if(originalFilesStr!=""){
    	ofStringReplace(qbsStr,originalFilesStr,filesStr);
    }

    // save final project
    qbs.set(qbsStr);
    ofFile project(projectDir + projectName + ".qbs",ofFile::WriteOnly,true);
    project.writeFromBuffer(qbs);
    return true;
}

void QtCreatorProject::addAddon(ofAddon & addon){
    for(int i=0;i<(int)addons.size();i++){
        if(addons[i].name==addon.name) return;
    }

    addons.push_back(addon);
}
