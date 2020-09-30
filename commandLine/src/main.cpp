#include "ofMain.h"
#include "optionparser.h"
#include "defines.h"
enum  optionIndex { UNKNOWN, HELP, PLUS, RECURSIVE, LISTTEMPLATES, PLATFORMS, ADDONS, OFPATH, VERBOSE, TEMPLATE, DRYRUN, SRCEXTERNAL, VERSION};

constexpr option::Descriptor usage[] =
{
    {UNKNOWN, 0, "", "",option::Arg::None, "Options:\n" },
    {HELP, 0,"h", "help",option::Arg::None, "  --help  \tPrint usage and exit." },
    {RECURSIVE, 0,"r","recursive",option::Arg::None, "  --recursive, -r  \tupdate recursively (applies only to update)" },
    {LISTTEMPLATES, 0,"l","listtemplates",option::Arg::None, "  --listtemplates, -l  \tlist templates available for the specified or current platform(s)" },
    {PLATFORMS, 0,"p","platforms",option::Arg::Optional, "  --platforms, -p  \tplatform list (such as osx, ios, winvs)" },
    {ADDONS, 0,"a","addons",option::Arg::Optional, "  --addons, -a  \taddon list (such as ofxOpenCv, ofxGui, ofxXmlSettings)" },
    {OFPATH, 0,"o","ofPath",option::Arg::Optional, "  --ofPath, -o  \tpath to openframeworks (relative or absolute). This *must* be set, or you can also alternatively use an environment variable PG_OF_PATH and if this isn't set, it will use that value instead" },
    {VERBOSE, 0,"v","verbose",option::Arg::None, "  --verbose, -v  \trun verbose" },
    {TEMPLATE, 0,"t","template",option::Arg::Optional, "  --template, -t  \tproject template" },
    {DRYRUN, 0,"d","dryrun",option::Arg::None, "  --dryrun, -d  \tdry run, don't change files" },
    {SRCEXTERNAL, 0,"s","source",option::Arg::Optional, "  --source, -s  \trelative or absolute path to source or include folders external to the project (such as ../../../../common_utils/" },
    {VERSION, 0, "w", "version", option::Arg::None, "  --version, -w  \treturn the current version"},
    {0,0,0,0,0,0}
};


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
#include "androidStudioProject.h"
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

std::string              directoryForRecursion;
std::string              projectPath;
std::string              ofPath;
std::vector <std::string>     addons;
std::vector <std::string>     srcPaths;
std::vector <ofTargetPlatform>        targets;
std::string              ofPathEnv;
std::string              currentWorkingDirectory;
std::string              templateName;

bool busingEnvVar;
bool bVerbose;
bool bAddonsPassedIn;
bool bForce;                            // force even if things like ofRoot seem wrong of if update folder looks wonky
int mode;                               // what mode are we in?
bool bRecursive;                        // do we recurse in update mode?
bool bHelpRequested;                    // did we request help?
bool bListTemplates;                    // did we request help?
bool bDryRun;                           // do dry run (useful for debugging recursive update)




//-------------------------------------------
void consoleSpace() {
    std::cout << std::endl;
}

void printVersion()
{
    std::cout << OFPROJECTGENERATOR_MAJOR_VERSION << "." << OFPROJECTGENERATOR_MINOR_VERSION << "." << OFPROJECTGENERATOR_PATCH_VERSION << std::endl;
}

bool printTemplates() {
    
    std::cout << "getOFRoot() " << getOFRoot() << std::endl;
    
    if(targets.size()>1){
	std::vector<std::vector<baseProject::Template>> allPlatformsTemplates;
        for(auto & target: targets){
            auto templates = getTargetProject(target)->listAvailableTemplates(getTargetString(target));
            allPlatformsTemplates.push_back(templates);
        }
	std::set<baseProject::Template> commonTemplates;
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

void addPlatforms(std::string value) {

    targets.clear();
    std::vector < std::string > platforms = ofSplitString(value, ",", true, true);

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
            targets.push_back(OF_TARGET_ANDROID);
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
            targets.push_back(OF_TARGET_ANDROID);
        }else{
            ofLogError() << "platform " << platforms[i] << " not valid";
        }
    }
}


bool isGoodProjectPath(std::string path) {

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

bool isGoodOFPath(std::string path) {

    ofDirectory dir(path);
    if (!dir.isDirectory()) {
        ofLogError() << "ofPath seems wrong... not a directory";
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
    }
    else {
        ofLogError() << "ofPath seems wrong... no scripts / templates directory";
        return false;
    }

}



void updateProject(std::string path, ofTargetPlatform target, bool bConsiderParameterAddons = true) {

    // bConsiderParameterAddons = do we consider that the user could call update with a new set of addons
    // either we read the addons.make file, or we look at the parameter list.
    // if we are updating recursively, we *never* consider addons passed as parameters.


    ofLogNotice() << "updating project " << path;
    auto project = getTargetProject(target);

    if (!bDryRun) project->create(path, templateName);

    if(bConsiderParameterAddons && bAddonsPassedIn){
        for(auto & addon: addons){
            project->addAddon(addon);
        }
    }else{
        ofLogNotice() << "parsing addons.make";
        project->parseAddons();
    }
    
    if(!bDryRun){
        for(auto & srcPath : srcPaths){
            project->addSrcRecursively(srcPath);
        }
    }

    if (!bDryRun) project->save();
}


void recursiveUpdate(std::string path, ofTargetPlatform target) {
    
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
    
    std::string header = "";
    header += "\tprojectGenerator [options] pathName\n\n";
    header += "if pathName exists, project is updated\n";
    header += "if pathName doesn't exist, project is created\n";
    header += "(pathName must follow options, which can come in any order)";
    std::cout << header << std::endl;
    
    consoleSpace();
    
    option::printUsage(std::cout, usage);
    
    consoleSpace();
    
    std::string footer = "";
    footer += "examples:\n\n";
    footer +=
    STRINGIFY(
        projectGenerator -o"../../../../" ../../../../apps/myApps/newExample
    );
    footer += "\n";
    footer += "(create a project called newExample using a relative path for the OF root and the project. note the relative path may be different depending on where this app is located)";
    footer += "\n\n";
    footer +=
    STRINGIFY(
              projectGenerator -r -o"../../../../" ../../../../examples
              );
    footer += "\n";
    footer += "(recursively update the examples folder)";
    footer += "\n\n";
    footer +=
    STRINGIFY(
              projectGenerator -o"../../../../" -a"ofxXmlSettings, ofxOpenCv" ../../../../apps/myApps/newExample
              );
    footer += "\n";
    footer += "(create / update an example with addons)";
    std::cout << footer << std::endl;
    
    consoleSpace();
}


//-------------------------------------------
int main(int argc, char* argv[]){
    
    
    //------------------------------------------- pre-parse
    bAddonsPassedIn = false;
    bDryRun = false;
    busingEnvVar = false;
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
    std::string projectName = "";
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
    if (options[HELP] || argc == 0) {
		ofLogError() << "No arguments";
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
    
    if (options[DRYRUN].count() > 0){
        bDryRun = true;
    }
    if (options[VERSION].count() > 0){
        printVersion();
        return EXIT_OK;
    }
    
    if (options[VERBOSE].count() > 0){
        bVerbose = true;
    }
    
    if (options[PLATFORMS].count() > 0){
        if (options[PLATFORMS].arg != NULL){
	    std::string platformString(options[PLATFORMS].arg);
            addPlatforms(platformString);
        }
    }
    
    if (options[ADDONS].count() > 0){
        bAddonsPassedIn = true; // could be empty
        if (options[ADDONS].arg != NULL){
	    std::string addonsString(options[ADDONS].arg);
            addons = ofSplitString(addonsString, ",", true, true);
        }
    }
    
    if (options[SRCEXTERNAL].count() > 0){
        if (options[SRCEXTERNAL].arg != NULL){
            std::string srcString(options[SRCEXTERNAL].arg);
            srcPaths = ofSplitString(srcString, ",", true, true);
        }
    }
    
    if (options[OFPATH].count() > 0){
        if (options[OFPATH].arg != NULL){
	    std::string ofPathString(options[OFPATH].arg);
            ofPath = ofPathString;
        }
    }
    
    if (options[TEMPLATE].count() > 0){
        if (options[TEMPLATE].arg != NULL){
	    std::string templateString(options[TEMPLATE].arg);
            templateName = templateString;
        }
    }


    if (parse.nonOptionsCount() > 0){
        projectName = parse.nonOption(0);
    }
    
    
    // ------------------------------------------------------ post parse
    
    nProjectsUpdated = 0;
    nProjectsCreated = 0;
    of::priv::initutils();
    startTime = ofGetElapsedTimef();
    consoleSpace();

    // try to get the OF_PATH as an environt variable
    char* pPath;
    pPath = getenv("PG_OF_PATH");
    if (pPath != NULL) {
        ofPathEnv = std::string(pPath);
    }
    
    if (ofPath == "" && ofPathEnv != "") {
        busingEnvVar = true;
        ofPath = ofPathEnv;
    }
    
 
    currentWorkingDirectory = Poco::Path::current();

    if (ofPath == "") {

        consoleSpace();
        ofLogError() << "no OF path set... please use -o or --ofPath or set a PG_OF_PATH environment variable";
        consoleSpace();
        
        //------------------------------------ fix me
        //printHelp();
        
        //return
		
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
    }
    else {
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
            ofLogNotice() << "setting OF path to: " << ofPath;
            if(busingEnvVar){
                ofLogNotice() << "from PG_OF_PATH environment variable";
            }else{
                ofLogNotice() << "from -o option";
            }
            ofLogNotice() << "target platform is: " << target;
            ofLogNotice() << "project path is: " << projectPath;
            
            if(templateName!=""){
                ofLogNotice() << "using additional template " << templateName;
            }
            

            ofLogNotice() << "setting up new project " << projectPath;
            if (!bDryRun) project->create(projectPath, templateName);

            if (!bDryRun){
                for(auto & addon: addons){
                    project->addAddon(addon);
                }
                for(auto & srcPath : srcPaths){
                    project->addSrcRecursively(srcPath);
                }
            }
            if (!bDryRun) project->save();

            ofLogNotice() << "project created! ";
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
                    ofLogNotice() << "setting OF path to: " << ofPath;
                    if(busingEnvVar){
                        ofLogNotice() << "from PG_OF_PATH environment variable";
                    }else{
                        ofLogNotice() << "from -o option";
                    }
                    ofLogNotice() << "target platform is: " << getTargetString(targets[i]);

                    if(templateName!=""){
                        ofLogNotice() << "using additional template " << templateName;
                    }
                    updateProject(projectPath,targets[i]);

                    ofLogNotice() << "project updated! ";
                    ofLogNotice() << "-----------------------------------------------";
                    consoleSpace();

                }
            }
            else {
                ofLogError() << "there's no src folder in this project path to update, maybe use create instead? (or use force to force updating)";
            }
        }
        else {
            for (int i = 0; i < (int)targets.size(); i++) {
                ofLogNotice() << "-----------------------------------------------";
                ofLogNotice() << "updating an existing project";
                ofLogNotice() << "target platform is: " << getTargetString(targets[i]);

                recursiveUpdate(projectPath, targets[i]);
                
                ofLogNotice() << "project updated! ";
                ofLogNotice() << "-----------------------------------------------";

            }
        }


    }

    consoleSpace();
    float elapsedTime = ofGetElapsedTimef() - startTime;
    if (nProjectsCreated > 0) std::cout << nProjectsCreated << " project created ";
    if (nProjectsUpdated == 1) std::cout << nProjectsUpdated << " project updated ";
    if (nProjectsUpdated > 1) std::cout << nProjectsUpdated << " projects updated ";
    ofLogNotice() << "in " << elapsedTime << " seconds" << std::endl;
    consoleSpace();

	
    return EXIT_OK;
}







