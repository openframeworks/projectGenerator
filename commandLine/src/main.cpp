

#include <iostream>
#include <string>
#include <vector>
#include "optionparser.h"
enum  optionIndex { UNKNOWN, HELP, PLUS, RECURSIVE, LISTTEMPLATES, PLATFORMS, AVAILPLATFORMS, ADDONS, OFPATH, VERBOSE, TEMPLATE, FORCE, DRYRUN };
constexpr option::Descriptor usage[] =
{
    {UNKNOWN, 0, "", "", option::Arg::None, "Options:\n" },
    {HELP, 0, "h", "help", option::Arg::None, "  --help, -h  \tPrint usage and exit" },
    {RECURSIVE, 0, "r", "recursive", option::Arg::None, "  --recursive, -r  \tUpdate recursively (applies only to update)" },
    {LISTTEMPLATES, 0, "l", "listtemplates", option::Arg::None, "  --listtemplates, -l  \tList templates available for the specified or current platform(s)" },
    {PLATFORMS, 0, "p", "platforms", option::Arg::Optional, "  --platforms, -p  \tSpecify a list of target platforms (see --available-platforms)" },
    {AVAILPLATFORMS, 0, "", "available-platforms", option::Arg::Optional, "  --available-platforms \tShow the currently available platforms" },
    {ADDONS, 0, "a", "addons", option::Arg::Optional, "  --addons, -a  \tAddon list (such as ofxOpenCv, ofxGui, ofxXmlSettings)" },
    {OFPATH, 0, "o", "ofPath", option::Arg::Optional, "  --ofPath, -o  \tPath to openframeworks (relative or absolute). If this argument is not given, the value of the PG_OF_PATH environment variable will be used" },
    {VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose, -v  \tShow detailed output" },
    {TEMPLATE, 0, "t", "template", option::Arg::Optional, "  --template, -t  \tUse a project template" },
    {FORCE, 0, "f", "force", option::Arg::Optional, "  --force, -f  \tForce updating, even if used folders don't seem right" },
    {DRYRUN, 0, "d", "dryrun", option::Arg::None, "  --dryrun, -d  \tOnly do a dry run (don't change files)" },
    {0,0,0,0,0,0}
};

#include "ofMain.h"

#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/AutoPtr.h"
#include <Poco/Path.h>

#include "qtcreatorproject.h"
#include "visualStudioProject.h"
#include "xcodeProject.h"
#include "Utils.h"


#define EXIT_OK 0
#define EXIT_USAGE 64
#define EXIT_DATAERR 65

using Poco::Path;

#define STRINGIFY(A)  #A

//-----------------------------------------------------
enum pgMode {
    PG_MODE_NONE,
    PG_MODE_CREATE,
    PG_MODE_UPDATE
};


float               startTime;
int                 nProjectsUpdated;
int                 nProjectsCreated;

string              directoryForRecursion;
string              projectPath;
string              ofPath;
vector <string>     addons;
vector <ofTargetPlatform>        targets;
string              ofPathEnv;
string              currentWorkingDirectory;
string              templateName;

bool bUsingEnvVar;                      // do we use the environment variable for the OF path?
bool bVerbose;                          // show extra output?
bool bAddonsPassedIn;                   // do we need to add any addons to the project?
bool bForce;                            // force even if things like ofRoot seem wrong of if update folder looks wonky
int mode;                               // what mode are we in?
bool bRecursive;                        // do we recurse in update mode?
bool bHelpRequested;                    // did we request help?
bool bListTemplates;                    // did we request help?
bool bDryRun;                           // do dry run (useful for debugging recursive update)

//-------------------------------------------
void consoleSpace() {
    cout << endl;
}

bool printTemplates() {

    cout << "getOFRoot() " << getOFRoot() << endl;

    if(targets.size()>1){
        vector<vector<baseProject::Template>> allPlatformsTemplates;
        for(auto & target: targets){
            auto templates = getTargetProject(target)->listAvailableTemplates(getTargetString(target));
            allPlatformsTemplates.push_back(templates);
        }
        set<baseProject::Template> commonTemplates;
        for(auto & templates: allPlatformsTemplates){
            for(auto & t: templates){
                bool foundInAll = true;
                for(auto & otherTemplates: allPlatformsTemplates){
                    auto found = false;
                    for(auto & otherT: otherTemplates){
                        if(otherT.name == t.name){
                            found = true;
                            continue;
                        }
                    }
                    foundInAll &= found;
                }
                if(foundInAll){
                    commonTemplates.insert(t);
                }
            }
        }
        if(commonTemplates.empty()){
            ofLogNotice() << "No templates available for all targets";
            return false;
        }else{
            ofLogNotice() << "Templates available for all targets";
            for(auto & t: commonTemplates){
                ofLogNotice() << t.name << "\t\t" << t.description;
            }
            return true;
        }

    }else{
        bool templatesFound = false;
        for(auto & target: targets){
            ofLogNotice() << "Templates for target " << getTargetString(target);
            auto templates = getTargetProject(target)->listAvailableTemplates(getTargetString(target));
            for(auto & templateConfig: templates){
                ofLogNotice() << templateConfig.name << "\t\t" << templateConfig.description;
            }
            consoleSpace();
            templatesFound = !templates.empty();
        }
        return templatesFound;
    }
}

void addPlatforms(string value) {

    targets.clear();
    vector < string > platforms = ofSplitString(value, ",", true, true);

    for (size_t i = 0; i < platforms.size(); i++) {
        if (platforms[i] == "linux") {
            targets.push_back(OF_TARGET_LINUX);
        }
        else if (platforms[i] == "linux64") {
            targets.push_back(OF_TARGET_LINUX64);
        }
        else if (platforms[i] == "linuxarmv6l") {
            targets.push_back(OF_TARGET_LINUXARMV6L);
        }
        else if (platforms[i] == "linuxarmv7l") {
            targets.push_back(OF_TARGET_LINUXARMV7L);
        }
        else if (platforms[i] == "msys2") {
            targets.push_back(OF_TARGET_MINGW);
        }
        else if (platforms[i] == "vs") {
            targets.push_back(OF_TARGET_WINVS);
        }
        else if (platforms[i] == "osx") {
            targets.push_back(OF_TARGET_OSX);
        }
        else if (platforms[i] == "ios") {
            targets.push_back(OF_TARGET_IPHONE);
        }
        else if (platforms[i] == "android") {
            ofLogError() << "android platform not supported yet" << endl;
            std::exit(1);
        }
        else if (platforms[i] == "allplatforms") {
            targets.push_back(OF_TARGET_LINUX);
            targets.push_back(OF_TARGET_LINUX64);
            targets.push_back(OF_TARGET_LINUXARMV6L);
            targets.push_back(OF_TARGET_LINUXARMV7L);
            targets.push_back(OF_TARGET_MINGW);
            targets.push_back(OF_TARGET_WINVS);
            targets.push_back(OF_TARGET_OSX);
            targets.push_back(OF_TARGET_IOS);
        }else{
            ofLogError() << "platform " << platforms[i] << " not valid";
        }
    }
}


bool isGoodProjectPath(string path) {

    ofDirectory dir(path);
    if (!dir.isDirectory()) {
        return false;
    }

    dir.listDir();
    bool bHasSrc = false;
    for (size_t i = 0; i < dir.size(); i++) {
        if (dir.getName(i) == "src") {
            bHasSrc = true;
        }
    }

    if (bHasSrc == true) {
        return true;
    }
    else {
        return false;
    }


}

bool isGoodOFPath(string path) {

    ofDirectory dir(path);
    if (!dir.isDirectory()) {
        ofLogError() << "The provided OF Path is not a directory.";
        return false;
    }

    dir.listDir();
    bool bHasTemplates = false;
    for (size_t i = 0; i < dir.size(); i++) {
        if (dir.getName(i) == "scripts") {
            bHasTemplates = true;
        }
    }

    if (bHasTemplates == true) {
        return true;
    } else {
        ofLogError() << "The given OF path does not contain a scripts and/or templates directory.";
        return false;
    }

}


void updateProject(string path, ofTargetPlatform target, bool bConsiderParameterAddons = true) {

    // bConsiderParameterAddons = do we consider that the user could call update with a new set of addons
    // either we read the addons.make file, or we look at the parameter list.
    // if we are updating recursively, we *never* consider addons passed as parameters.

    ofLogNotice() << "Updating project " << path;
    auto project = getTargetProject(target);

    if (!bDryRun) project->create(path, templateName);

    if(bConsiderParameterAddons && bAddonsPassedIn){
        for(auto & addon: addons){
            project->addAddon(addon);
        }
    }else{
        ofLogNotice() << "Parsing addons.make";
        project->parseAddons();
    }

    if (!bDryRun) project->save();
}


void recursiveUpdate(string path, ofTargetPlatform target) {

    ofDirectory dir(path);

    // first, bail if it's just a file
    if (dir.isDirectory() == false) return;

    // second check if this is a folder that has src in it
    if (isGoodProjectPath(path)) {
        nProjectsUpdated++;
        auto project = getTargetProject(target);
        updateProject(path, target, false);
        return;
    }

    // finally, recursively look at this
    dir.listDir();
    for (size_t i = 0; i < dir.size(); i++) {
        ofDirectory subDir(dir.getPath(i));
        if (subDir.isDirectory()) {
            recursiveUpdate(dir.getPath(i), target);
        }
    }
}


void printHelp(){

    consoleSpace();

    string header = "";
    header += "\tprojectGenerator [options] pathName\n\n";
    header += "If pathName exists, project is updated. Otherwise, if pathName doesn't exist, project is created\n";
    header += "Pathname has to be the last argument. Other options can come in any order.";
    cout << header << endl;

    consoleSpace();

    option::printUsage(std::cout, usage);

    consoleSpace();

    string footer = "";
    footer += "Examples:\n\n";
    footer += "  To make a new project called newProject\n";
    footer += "    projectGenerator -o \"pathToOF\" pathOfNewProject\n";
    footer += "  To update an existing project:\n";
    footer += "    ./projectGenerator -o \"pathToOF\" pathToExistingProject\n\n";
    footer += "  To update a folder of projects:\n";
    footer += "    ./projectGenerator -o \"pathToOF\" -r pathToFolderOfProjects\n\n";
    footer += "  To specify specific platforms:\n";
    footer += "    projectGenerator -o \"pathToOF\" -p \"ios\" pathOfNewIOSProject";

    cout << footer << endl;

    consoleSpace();
}


//-------------------------------------------
int main(int argc, char* argv[]){

    //------------------------------------------- pre-parse
    bAddonsPassedIn = false;
    bDryRun = false;
    bUsingEnvVar = false;
    bVerbose = false;
    mode = PG_MODE_NONE;
    bForce = false;
    bRecursive = false;
    bHelpRequested = false;
    bListTemplates = false;
    targets.push_back(ofGetTargetPlatform());
    startTime = 0;
    nProjectsUpdated = 0;
    nProjectsCreated = 0;
    string projectName = "";
    projectPath = "";
    templateName = "";

    // ------------------------------------------------------ parse args
    argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
    option::Stats  stats(usage, argc, argv);
    std::vector<option::Option> options(stats.options_max);
    std::vector<option::Option> buffer(stats.buffer_max);
    option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);
    if (parse.error()) {
        return 1;
    }

    if (options[HELP]){
        printHelp();
        return EXIT_OK;
    }

    if (argc == 0) {
        ofLogError() << "No arguments given";
        printHelp();
        return EXIT_OK;
    }

    // templates:
    if (options[LISTTEMPLATES].count() > 0){
        bListTemplates = true;
    }

    if (options[RECURSIVE].count() > 0){
        bRecursive = true;
    }

    if (options[FORCE].count() > 0){
        bForce = true;
    }

    if (options[DRYRUN].count() > 0){
        bDryRun = true;
    }

    if (options[VERBOSE].count() > 0){
        bVerbose = true;
    }

    if (options[PLATFORMS].count() > 0){
        if (options[PLATFORMS].arg != NULL){
            string platformString(options[PLATFORMS].arg);
            addPlatforms(platformString);
        }
    }

    if (options[AVAILPLATFORMS].count() > 0){
        ofLogNotice() << "Currently available target platforms are: allplatforms, osx, ios, msys2, android (not yet supported), linux, linux64, linuxarmv6l, linuxarmv7l, vs";
        return EXIT_OK;
    }

    if (options[ADDONS].count() > 0){
        bAddonsPassedIn = true; // could be empty
        if (options[ADDONS].arg != NULL){
            string addonsString(options[ADDONS].arg);
            addons = ofSplitString(addonsString, ",", true, true);
        }
    }

    if (options[OFPATH].count() > 0){
        if (options[OFPATH].arg != NULL){
            string ofPathString(options[OFPATH].arg);
            ofPath = ofPathString;
        }
    }

    if (options[TEMPLATE].count() > 0){
        if (options[TEMPLATE].arg != NULL){
            string templateString(options[TEMPLATE].arg);
            templateName = templateString;
        }
    }

    if (parse.nonOptionsCount() > 0){
        projectName = parse.nonOption(0);
    }

    // ------------------------------------------------------ post parse

    startTime = ofGetElapsedTimef();
    nProjectsUpdated = 0;
    nProjectsCreated = 0;
    of::priv::setWorkingDirectoryToDefault();
    consoleSpace();

    // try to get the OF_PATH as an environt variable
    char* pPath;
    pPath = getenv("PG_OF_PATH");
    if (pPath != NULL) {
        ofPathEnv = string(pPath);
    }

    if (ofPath == "" && ofPathEnv != "") {
        bUsingEnvVar = true;
        ofPath = ofPathEnv;
    }

    currentWorkingDirectory = Poco::Path::current();

    if (ofPath == "") {

        consoleSpace();
        ofLogError() << "OF path not set. Please provide a valid path using the -o or --ofPath arguments, or set a PG_OF_PATH environment variable.";
        consoleSpace();

        return EXIT_USAGE;

    } else {

        // let's try to resolve this path vs the current path
        // so things like ../ can work
        // see http://www.appinf.com/docs/poco/Poco.Path.html

        Path cwd = Path::current();                  // get the current path
        ofPath = cwd.resolve(ofPath).toString();   // resolve ofPath vs that.
        Path resolvedPath = Path(ofPath).absolute();    // make that new path absolute
        ofPath = resolvedPath.toString();

        if (!isGoodOFPath(ofPath)) {
            return EXIT_USAGE;
        }

        setOFRoot(ofPath);

    }

    if(bListTemplates){
        auto ret = printTemplates();
        consoleSpace();
        if(ret){
            return EXIT_OK;
        }else{
            return EXIT_DATAERR;
        }
    }

    if (projectName != ""){
        if (ofFilePath::isAbsolute(projectName)) {
            projectPath = projectName;
        } else {
            projectPath = ofFilePath::join(projectPath, projectName);

            // this line is arturo's ninja magic to make paths with dots make sense:
            projectPath = ofFilePath::removeTrailingSlash(ofFilePath::getPathForDirectory(ofFilePath::getAbsolutePath(projectPath, false)));
            projectPath = Path(projectPath).absolute().toString();		// make absolute...
        }
    } else {
        ofLogError() << "Missing project path";
        printHelp();
        consoleSpace();

        return EXIT_USAGE;
    }

    if (ofDirectory(projectPath).exists()) {
        mode = PG_MODE_UPDATE;
    } else {
        mode = PG_MODE_CREATE;
    }

    if (bVerbose){
        ofSetLogLevel(OF_LOG_VERBOSE);
    }

    if (mode == PG_MODE_CREATE) {

        nProjectsCreated += 1;

        for (int i = 0; i < (int)targets.size(); i++) {
            auto project = getTargetProject(targets[i]);
            auto target = getTargetString(targets[i]);

            ofLogNotice() << "-----------------------------------------------";
            ofLogNotice() << "Setting OF path to: " << ofPath;
            if(bUsingEnvVar){
                ofLogNotice() << "from PG_OF_PATH environment variable";
            }else{
                ofLogNotice() << "from -o / --ofpath argument";
            }
            ofLogNotice() << "Target platform is: " << target;
            ofLogNotice() << "Project path is: " << projectPath;

            if(templateName!=""){
                ofLogNotice() << "Using additional template " << templateName;
            }

            ofLogNotice() << "Setting up new project " << projectPath;
            if (!bDryRun) project->create(projectPath, templateName);

            if (!bDryRun){
                for(auto & addon: addons){
                    project->addAddon(addon);
                }
            }

            if (!bDryRun) project->save();

            ofLogNotice() << "Project created! ";
            ofLogNotice() << "-----------------------------------------------";
            consoleSpace();
        }
    }
    else if (mode == PG_MODE_UPDATE) {

        if (!bRecursive) {
            if (isGoodProjectPath(projectPath) || bForce) {

                nProjectsUpdated += 1;

                for (int i = 0; i < (int)targets.size(); i++) {
                    ofLogNotice() << "-----------------------------------------------";
                    ofLogNotice() << "Setting OF path to: " << ofPath;
                    if(bUsingEnvVar){
                        ofLogNotice() << "from PG_OF_PATH environment variable";
                    }else{
                        ofLogNotice() << "from -o / --ofPath argument";
                    }
                    ofLogNotice() << "Target platform is: " << getTargetString(targets[i]);

                    if(templateName!=""){
                        ofLogNotice() << "Using additional template " << templateName;
                    }
                    updateProject(projectPath,targets[i]);

                    ofLogNotice() << "Project updated! ";
                    ofLogNotice() << "-----------------------------------------------";
                    consoleSpace();

                }
            }
            else {
                ofLogError() << "The given project path does not contain a src folder to update; maybe you meant to use create instead? (or use force to force updating the folder)";
            }
        }
        else {
            for (int i = 0; i < (int)targets.size(); i++) {
                ofLogNotice() << "-----------------------------------------------";
                ofLogNotice() << "Updating an existing project";
                ofLogNotice() << "Target platform is: " << getTargetString(targets[i]);

                recursiveUpdate(projectPath, targets[i]);

                ofLogNotice() << "Project updated! ";
                ofLogNotice() << "-----------------------------------------------";

            }
        }


    }

    consoleSpace();
    float elapsedTime = ofGetElapsedTimef() - startTime;
    if (nProjectsCreated > 0) cout << nProjectsCreated << " Project created ";
    if (nProjectsUpdated == 1)cout << nProjectsUpdated << " Project updated ";
    if (nProjectsUpdated > 1) cout << nProjectsUpdated << " Projects updated ";
    ofLogNotice() << "in " << elapsedTime << " seconds" << endl;
    consoleSpace();

    return EXIT_OK;
}







