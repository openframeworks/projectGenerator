#include "ofMain.h"
#include <Poco/Path.h>
#include "cxxopts.hpp"

#include "qtcreatorproject.h"
#include "visualStudioProject.h"
#include "xcodeProject.h"
#include "androidStudioProject.h"
#include "Utils.h"

#define EXIT_OK 0
#define EXIT_USAGE 64
#define EXIT_DATAERR 65

#define STRINGIFY(A) #A

//-----------------------------------------------------
enum pgMode
{
    PG_MODE_NONE,
    PG_MODE_CREATE,
    PG_MODE_UPDATE
};

float startTime;
int nProjectsUpdated;
int nProjectsCreated;

std::string directoryForRecursion;
std::string projectPath;
std::string currentWorkingDirectory;
std::string ofPath;
std::string ofPathEnv;
std::vector<std::string> addons;
std::vector<ofTargetPlatform> targets;
std::string templateName;

bool busingEnvVar;
bool bVerbose;
bool bAddonsPassedIn;
bool bForce;         // force even if things like ofRoot seem wrong of if update folder looks wonky
int mode;            // what mode are we in?
bool bRecursive;     // do we recurse in update mode?
bool bHelpRequested; // did we request help?
bool bListTemplates; // did we request printing templates?
bool bDryRun;        // do dry run (useful for debugging recursive update)

cxxopts::Options options("projectGenerator", "openFrameworks's project generator");

//-------------------------------------------
void consoleSpace()
{
    std::cout << std::endl;
}

bool printTemplates()
{
    std::cout << "getOFRoot() " << getOFRoot() << std::endl;

    if (targets.size() > 1)
    {
        std::vector<std::vector<baseProject::Template>> allPlatformsTemplates;
        for (auto &target : targets)
        {
            auto templates = getTargetProject(target)->listAvailableTemplates(getTargetString(target));
            allPlatformsTemplates.push_back(templates);
        }
        std::set<baseProject::Template> commonTemplates;
        for (auto &templates : allPlatformsTemplates)
        {
            for (auto &t : templates)
            {
                bool foundInAll = true;
                for (auto &otherTemplates : allPlatformsTemplates)
                {
                    auto found = false;
                    for (auto &otherT : otherTemplates)
                    {
                        if (otherT.name == t.name)
                        {
                            found = true;
                            continue;
                        }
                    }
                    foundInAll &= found;
                }
                if (foundInAll)
                {
                    commonTemplates.insert(t);
                }
            }
        }
        if (commonTemplates.empty())
        {
            ofLogNotice() << "No templates available for all targets";
            return false;
        }
        else
        {
            ofLogNotice() << "Templates available for all targets";
            for (auto &t : commonTemplates)
            {
                ofLogNotice() << t.name << "\t\t" << t.description;
            }
            return true;
        }
    }
    else
    {
        bool templatesFound = false;
        for (auto &target : targets)
        {
            ofLogNotice() << "Templates for target " << getTargetString(target);
            auto templates = getTargetProject(target)->listAvailableTemplates(getTargetString(target));
            for (auto &templateConfig : templates)
            {
                ofLogNotice() << templateConfig.name << "\t\t" << templateConfig.description;
            }
            consoleSpace();
            templatesFound = !templates.empty();
        }
        return templatesFound;
    }
}

void addPlatforms(std::string value)
{
    targets.clear();
    std::vector<std::string> platforms = ofSplitString(value, ",", true, true);

    for (size_t i = 0; i < platforms.size(); i++)
    {
        if (platforms[i] == "linux")
        {
            targets.push_back(OF_TARGET_LINUX);
        }
        else if (platforms[i] == "linux64")
        {
            targets.push_back(OF_TARGET_LINUX64);
        }
        else if (platforms[i] == "linuxarmv6l")
        {
            targets.push_back(OF_TARGET_LINUXARMV6L);
        }
        else if (platforms[i] == "linuxarmv7l")
        {
            targets.push_back(OF_TARGET_LINUXARMV7L);
        }
        else if (platforms[i] == "msys2")
        {
            targets.push_back(OF_TARGET_MINGW);
        }
        else if (platforms[i] == "vs")
        {
            targets.push_back(OF_TARGET_WINVS);
        }
        else if (platforms[i] == "osx")
        {
            targets.push_back(OF_TARGET_OSX);
        }
        else if (platforms[i] == "ios")
        {
            targets.push_back(OF_TARGET_IPHONE);
        }
        else if (platforms[i] == "android")
        {
            targets.push_back(OF_TARGET_ANDROID);
        }
        else if (platforms[i] == "allplatforms")
        {
            targets.push_back(OF_TARGET_LINUX);
            targets.push_back(OF_TARGET_LINUX64);
            targets.push_back(OF_TARGET_LINUXARMV6L);
            targets.push_back(OF_TARGET_LINUXARMV7L);
            targets.push_back(OF_TARGET_MINGW);
            targets.push_back(OF_TARGET_WINVS);
            targets.push_back(OF_TARGET_OSX);
            targets.push_back(OF_TARGET_IOS);
            targets.push_back(OF_TARGET_ANDROID);
        }
        else
        {
            ofLogError() << "platform " << platforms[i] << " not valid";
        }
    }
}

bool isGoodProjectPath(std::string path)
{
    ofDirectory dir(path);
    if (!dir.isDirectory())
    {
        return false;
    }

    dir.listDir();
    bool bHasSrc = false;
    for (size_t i = 0; i < dir.size(); i++)
    {
        if (dir.getName(i) == "src")
        {
            bHasSrc = true;
        }
    }

    if (bHasSrc == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool isGoodOFPath(std::string path)
{
    ofDirectory dir(path);
    if (!dir.isDirectory())
    {
        ofLogError() << "ofPath seems wrong... not a directory";
        return false;
    }

    dir.listDir();
    bool bHasTemplates = false;
    for (size_t i = 0; i < dir.size(); i++)
    {
        if (dir.getName(i) == "scripts")
        {
            bHasTemplates = true;
        }
    }

    if (bHasTemplates == true)
    {
        return true;
    }
    else
    {
        ofLogError() << "ofPath seems wrong... no scripts / templates directory";
        return false;
    }
}

void updateProject(std::string path, ofTargetPlatform target, bool bConsiderParameterAddons = true)
{
    // bConsiderParameterAddons = do we consider that the user could call update with a new set of addons
    // either we read the addons.make file, or we look at the parameter list.
    // if we are updating recursively, we *never* consider addons passed as parameters.

    ofLogNotice() << "updating project " << path;
    auto project = getTargetProject(target);

    if (!bDryRun)
        project->create(path, templateName);

    if (bConsiderParameterAddons && bAddonsPassedIn)
    {
        for (auto &addon : addons)
        {
            project->addAddon(addon);
        }
    }
    else
    {
        ofLogNotice() << "parsing addons.make";
        project->parseAddons();
    }

    if (!bDryRun)
        project->save();
}

void recursiveUpdate(std::string path, ofTargetPlatform target)
{
    ofDirectory dir(path);

    // first, bail if it's just a file
    if (dir.isDirectory() == false)
        return;

    // second check if this is a folder that has src in it
    if (isGoodProjectPath(path))
    {
        nProjectsUpdated++;
        auto project = getTargetProject(target);
        updateProject(path, target, false);
        return;
    }

    // finally, recursively look at this
    dir.listDir();
    for (size_t i = 0; i < dir.size(); i++)
    {
        ofDirectory subDir(dir.getPath(i));
        if (subDir.isDirectory())
        {
            recursiveUpdate(dir.getPath(i), target);
        }
    }
}

void printHelp()
{

    std::cout << options.help() << std::endl;
    consoleSpace();

    std::string footer = "";
    footer += "examples:\n\n";

    footer += "update the project at the current working directory:\n";
    footer += "\tprojectGenerator -p . -o ../../..\n\n";

    footer += "create a new project:\n";
    footer += "\tprojectGenerator -p /path/to/oF/app/myApps/nonExistingDirectory -o /path/to/oF\n\n";

    footer += "recursively update the examples folder:\n";
    footer += "\tprojectGenerator -r -p /path/to/oF/examples -o /path/to/oF\n\n";

    footer += "create or update a project with addons:\n";
    footer += "\tprojectGenerator -a ofxOsc,ofxOpenCv -p /path/to/oF/apps/myApps/appWithAddons -o /path/to/oF\n";
    footer += "\tprojectGenerator -a \"ofxOsc, ofxOpenCv\" -p /path/to/oF/apps/myApps/appWithAddons -o /path/to/oF\n\n";

    std::cout << footer << std::endl;
}

//-------------------------------------------
int main(int argc, char *argv[])
{
    options.add_options()                                                                                                    //
        ("p,project", "path to project (required)", cxxopts::value<std::string>())                                           //
        ("o,ofPath", "path to openFrameworks directory (required, unless PG_OF_PATH is set)", cxxopts::value<std::string>()) //
        ("a,addons", "comma separated list of addons", cxxopts::value<std::string>())                                        //
        ("s,platforms", "comma separated list of platforms", cxxopts::value<std::string>())                                  //
        ("t,template", "template", cxxopts::value<std::string>())                                                            //
        ("h,help", "prints usage")                                                                                           //
        ("v,verbose", "verbose output")                                                                                      //
        ("d,dryrun", "do dry run (useful for debugging recursive update)", cxxopts::value<bool>()->default_value("false"))   //
        ("r,recursive", "recursively updates projects")                                                                      //
        ("l,listtemplates", "prints a list of available templates")                                                          //
        ;
    auto result = options.parse(argc, argv);

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
    if (result["help"].as<bool>() || argc == 0)
    {
        printHelp();
        return EXIT_OK;
    }

    bListTemplates = result["listtemplates"].as<bool>();
    bRecursive = result["recursive"].as<bool>();
    bDryRun = result["dryrun"].as<bool>();
    bVerbose = result["verbose"].as<bool>();

    if (result["platforms"].count() > 0)
    {
        addPlatforms(result["platforms"].as<std::string>());
    }

    if (result["addons"].count() > 0)
    {
        bAddonsPassedIn = true; // could be empty
        addons = ofSplitString(result["addons"].as<std::string>(), ",", true, true);
    }

    if (result["ofPath"].count() > 0)
    {
        ofPath = result["ofPath"].as<std::string>();
    }

    if (result["template"].count() > 0)
    {
        templateName = result["template"].as<std::string>();
    }
    if (result["project"].count() > 0)
    {
        projectName = result["project"].as<std::string>();
        projectName = ofFilePath::getPathForDirectory(projectName); // append trailing slash
        Poco::Path cwd = Poco::Path::current();                     // get the current path
        projectName = cwd.resolve(projectName).toString();          // resolve ofPath vs that.
        auto resolvedPath = Poco::Path(projectName).absolute();     // make that new path absolute
        projectName = resolvedPath.toString();
    }

    // ------------------------------------------------------ post parse

    nProjectsUpdated = 0;
    nProjectsCreated = 0;
    of::priv::initutils();
    startTime = ofGetElapsedTimef();
    consoleSpace();

    // try to get the OF_PATH as an environt variable
    auto pPath = getenv("PG_OF_PATH");
    if (pPath != nullptr)
    {
        ofPathEnv = std::string(pPath);
    }

    if (ofPath.empty() && !ofPathEnv.empty())
    {
        busingEnvVar = true;
        ofPath = ofPathEnv;
    }

    if (ofPath.empty())
    {
        consoleSpace();
        ofLogError() << "no OF path set... please use -o or --ofPath or set a PG_OF_PATH environment variable";
        consoleSpace();

        //------------------------------------ fix me
        //printHelp();

        //return

        return EXIT_USAGE;
    }
    else
    {

        // let's try to resolve this path vs the current path
        // so things like ../ can work
        // see http://www.appinf.com/docs/poco/Poco.Path.html

        // ofPath = ofFilePath::getPathForDirectory(ofPath);  // append trailing slash
        Poco::Path cwd = Poco::Path::current();            // get the current path
        ofPath = cwd.resolve(ofPath).toString();           // resolve ofPath vs that.
        auto resolvedPath = Poco::Path(ofPath).absolute(); // make that new path absolute
        ofPath = resolvedPath.toString();

        if (!isGoodOFPath(ofPath))
        {
            return EXIT_USAGE;
        }

        setOFRoot(ofPath);
    }

    if (bListTemplates)
    {
        auto ret = printTemplates();
        consoleSpace();
        if (ret)
        {

            return EXIT_OK;
        }
        else
        {

            return EXIT_DATAERR;
        }
    }

    if (!projectName.empty())
    {
        if (ofFilePath::isAbsolute(projectName))
        {
            projectPath = ofFilePath::removeTrailingSlash(projectName);
        }
        else
        {
            projectPath = ofFilePath::join(projectPath, projectName);

            // this line is arturo's ninja magic to make paths with dots make sense:
            projectPath = ofFilePath::removeTrailingSlash(ofFilePath::getPathForDirectory(ofFilePath::getAbsolutePath(projectPath, false)));
            projectPath = Poco::Path(projectPath).absolute().toString(); // make absolute...
        }
    }
    else
    {
        ofLogError() << "Missing project path";
        printHelp();
        consoleSpace();

        return EXIT_USAGE;
    }

    if (ofDirectory(projectPath).exists())
    {
        mode = PG_MODE_UPDATE;
    }
    else
    {
        mode = PG_MODE_CREATE;
    }

    if (bVerbose)
    {
        ofSetLogLevel(OF_LOG_VERBOSE);
    }

    if (mode == PG_MODE_CREATE)
    {

        nProjectsCreated += 1;

        for (int i = 0; i < (int)targets.size(); i++)
        {
            auto project = getTargetProject(targets[i]);
            auto target = getTargetString(targets[i]);

            ofLogNotice() << "-----------------------------------------------";
            ofLogNotice() << "setting OF path to: " << ofPath;
            if (busingEnvVar)
            {
                ofLogNotice() << "from PG_OF_PATH environment variable";
            }
            else
            {
                ofLogNotice() << "from -o option";
            }
            ofLogNotice() << "target platform is: " << target;
            ofLogNotice() << "project path is: " << projectPath;

            if (!templateName.empty())
            {
                ofLogNotice() << "using additional template " << templateName;
            }

            ofLogNotice() << "setting up new project " << projectPath;
            if (!bDryRun)
                project->create(projectPath, templateName);

            if (!bDryRun)
            {
                for (auto &addon : addons)
                {
                    project->addAddon(addon);
                }
            }
            if (!bDryRun)
                project->save();

            ofLogNotice() << "project created! ";
            ofLogNotice() << "-----------------------------------------------";
            consoleSpace();
        }
    }
    else if (mode == PG_MODE_UPDATE)
    {
        if (!bRecursive)
        {
            if (isGoodProjectPath(projectPath) || bForce)
            {
                nProjectsUpdated += 1;

                for (int i = 0; i < (int)targets.size(); i++)
                {
                    ofLogNotice() << "-----------------------------------------------";
                    ofLogNotice() << "setting OF path to: " << ofPath;
                    if (busingEnvVar)
                    {
                        ofLogNotice() << "from PG_OF_PATH environment variable";
                    }
                    else
                    {
                        ofLogNotice() << "from -o option";
                    }
                    ofLogNotice() << "target platform is: " << getTargetString(targets[i]);

                    if (!templateName.empty())
                    {
                        ofLogNotice() << "using additional template " << templateName;
                    }
                    updateProject(projectPath, targets[i]);

                    ofLogNotice() << "project updated! ";
                    ofLogNotice() << "-----------------------------------------------";
                    consoleSpace();
                }
            }
            else
            {
                ofLogError() << "there's no src folder in this project path to update, maybe use create instead? (or use force to force updating)";
            }
        }
        else
        {
            for (int i = 0; i < (int)targets.size(); i++)
            {
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
    if (nProjectsCreated > 0)
        std::cout << nProjectsCreated << " project created ";
    if (nProjectsUpdated == 1)
        std::cout << nProjectsUpdated << " project updated ";
    if (nProjectsUpdated > 1)
        std::cout << nProjectsUpdated << " projects updated ";
    ofLogNotice() << "in " << elapsedTime << " seconds" << std::endl;
    consoleSpace();

    return EXIT_OK;
}
