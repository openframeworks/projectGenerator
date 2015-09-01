#include "ofMain.h"
#include "ofApp.h"
#include "ofAppGlutWindow.h"
#include "ofAppNoWindow.h"

#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/AutoPtr.h"
#include <Poco/Path.h>
#include <iostream>

#include "CBLinuxProject.h"
#include "CBWinProject.h"
#include "visualStudioProject.h"
#include "xcodeProject.h"
#include "Utils.h"


using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::AutoPtr;
using Poco::Path;

/*

todo:

- really check OF root
- check if folder exits....

*/


class ofColorsLoggerChannel: public ofBaseLoggerChannel{
    std::string CON_DEFAULT="\033[0m";
    std::string CON_BOLD="\033[1m";
    std::string CON_RED="\033[31m";
    std::string CON_YELLOW="\033[33m";
    std::string CON_GREEN="\033[32m";
    std::string getColor(ofLogLevel level) const{
        switch(level){
        case OF_LOG_FATAL_ERROR:
        case OF_LOG_ERROR:
            return CON_RED;
        case OF_LOG_WARNING:
            return CON_YELLOW;
        case OF_LOG_NOTICE:
            return "";
        default:
            return CON_DEFAULT;
        }
    }
public:
    void log(ofLogLevel level, const std::string & module, const std::string & message){
        if(level>=OF_LOG_WARNING){
            std::cout << CON_BOLD << getColor(level);
        }else{
            std::cout << getColor(level);
        }
        if(module != ""){
            std::cout << module << ": ";
        }
        if(level>=OF_LOG_WARNING){
            std::cout << message << CON_DEFAULT << std::endl;
        }else{
            std::cout << message << std::endl;
        }
    }

    void log(ofLogLevel level, const std::string & module, const char* format, ...){
        va_list args;
        va_start(args, format);
        log(level, module, format, args);
        va_end(args);
    }

    void log(ofLogLevel level, const std::string & module, const char* format, va_list args){
        if(level>=OF_LOG_WARNING){
            fprintf(stdout, "%s", (CON_BOLD + getColor(level)).c_str());
        }
        if(module != ""){
            fprintf(stdout, "%s: ", module.c_str());
        }
        vfprintf(stdout, format, args);
        if(level>=OF_LOG_WARNING){
            fprintf(stdout, "%s", CON_DEFAULT.c_str());
        }
        fprintf(stdout, "\n");
    }
};


class commandLineProjectGenerator : public Application {
public:


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

	bool busingEnvVar;
    bool bVerbose;
    bool bAddonsPassedIn;
	bool bForce;                            // force even if things like ofRoot seem wrong of if update folder looks wonky
	int mode;                               // what mode are we in?
	bool bRecursive;                        // do we recurse in update mode?
	bool bHelpRequested;                    // did we request help?
    bool bListTemplates;                    // did we request help?
	bool bDryRun;                           // do dry run (useful for debugging recursive update)

	commandLineProjectGenerator() {
        
        // this is called before params have been parsed.
        
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
        
	}

	void initialize(Application& self) {
        // this is called after params have been parsed.
        
        startTime = ofGetElapsedTimef();
        nProjectsUpdated = 0;
        nProjectsCreated = 0;
        of::priv::setWorkingDirectoryToDefault();
		consoleSpace();
		ofSetLoggerChannel(std::make_shared<ofColorsLoggerChannel>());


		if (!bHelpRequested) {

			//-------------------------------------------------------------------------------
			// try to get the OF_PATH as an environt variable
			char* pPath;
			pPath = getenv("PG_OF_PATH");
			if (pPath != NULL) {
				ofPathEnv = string(pPath);
			}
			//-------------------------------------------------------------------------------

		}



		currentWorkingDirectory = Poco::Path::current();
		loadConfiguration(); // load default configuration files, if present
		Application::initialize(self);

	}

	void uninitialize() {
		Application::uninitialize();
	}

	void reinitialize(Application& self) {
		Application::reinitialize(self);
	}

	void defineOptions(OptionSet& options) {
		Application::defineOptions(options);


		options.addOption(
			Option("recursive", "r", "update recursively (applies only to update)")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));


		options.addOption(
			Option("help", "h", "")
			.required(false)
			.repeatable(false)
			.callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));

        options.addOption(
            Option("listtemplates", "l", "list templates available for the specified or current platform(s)")
            .required(false)
            .repeatable(false)
            .callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));


		options.addOption(
			Option("platforms", "p", "platform list")
			.required(false)
			.repeatable(false)
			.argument("\"platform list\"")
			.callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));


		options.addOption(
			Option("addons", "a", "addons list")
			.required(false)
			.repeatable(false)
			.argument("\"addons list\"")
			.callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));

		options.addOption(
			Option("ofPath", "o", "openframeworks path")
			.required(false)
			.repeatable(false)
			.argument("\"OF path\"")
			.callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));

		options.addOption(
			Option("verbose", "v", "run verbose")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));

        options.addOption(
            Option("template", "t", "project template")
            .required(false)
            .repeatable(false)
            .argument("\"project_template\"")
            .callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));

		options.addOption(
			Option("dryrun", "d", "don't change files")
			.required(false)
			.repeatable(false)
			.noArgument()
			.callback(OptionCallback<commandLineProjectGenerator>(this, &commandLineProjectGenerator::handleOption)));



	}

	void handleOption(const std::string& name, const std::string& value) {
		if (name == "help") {
		    bHelpRequested = true;
		}
        else if (name == "listtemplates") {
            bListTemplates = true;
        }
		else if (name == "platforms") {
			addPlatforms(value);
		}
		else if (name == "addons") {
            bAddonsPassedIn = true;
			addons = ofSplitString(value, ",", true, true);
		}
		else if (name == "ofPath") {
			ofPath = value;
		}
		else if (name == "recursive") {
			bRecursive = true;
		}
		else if (name == "dryrun") {
			bDryRun = true;
		}
        else if (name == "verbose") {
            bVerbose = true;
		}
        else if (name == "template") {
            templateName = value;
        }
	}


	void printHelp() {
		bHelpRequested = true;
		HelpFormatter helpFormatter(options());

		consoleSpace();



		string header = "";
		header += "\n\n\t\tprojectGenerator [options] pathName\n\n";
		header += "if pathName exists, project is updated\n";
		header += "if pathName doesn't exist, project is created\n\n";
		header += "OPTIONS:\n\n";
		header += "lists should be comma seperated and in quotes if there are spaces\n";
		header += "you can use : or = for parameter based options, such as -o=/usr/...";
		helpFormatter.setHeader(header);


		helpFormatter.setFooter("\n");
		helpFormatter.format(std::cout);
		stopOptionsProcessing();
	}


    void printTemplates() {
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
                        commonTemplates.emplace(t);
                    }
                }
            }
            if(commonTemplates.empty()){
                ofLogNotice() << "No templates available for all targets";
            }else{
                ofLogNotice() << "Templates available for all targets";
                for(auto & t: commonTemplates){
                    ofLogNotice() << t.name << "\t\t" << t.description;
                }
            }
        }else{
            for(auto & target: targets){
                ofLogNotice() << "Templates for target " << getTargetString(target);
                auto templates = getTargetProject(target)->listAvailableTemplates(getTargetString(target));
                for(auto & templateConfig: templates){
                    ofLogNotice() << templateConfig.name << "\t\t" << templateConfig.description;
                }
                consoleSpace();
            }
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
			else if (platforms[i] == "win_cb") {
				targets.push_back(OF_TARGET_WINGCC);
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
				targets.push_back(OF_TARGET_WINGCC);
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

	void consoleSpace() {
		cout << endl;
	}


	void updateProject(string path, ofTargetPlatform target, bool bConsiderParameterAddons = true) {

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

		if (!bDryRun) project->save();
	}


	int main(const std::vector<std::string>& args) {


		// check for a non option command line arg

		string projectName = "";



		if (bHelpRequested) {
            printHelp();
			consoleSpace();

	        if(!bListTemplates){
	            return Application::EXIT_OK;
	        }
		}



        //-------------------------- get OF path from env variable if available
        if (ofPath == "" && ofPathEnv != "") {
            busingEnvVar = true;
            ofPath = ofPathEnv;
        }
        if (ofPath == "") {

            consoleSpace();
            ofLogError() << "no OF path set... please use -o or --ofPath or set a PG_OF_PATH environment variable";
            consoleSpace();
            printHelp();
            return Application::EXIT_USAGE;
        }
        else {

            // let's try to resolve this path vs the current path
            // so things like ../ can work
            // see http://www.appinf.com/docs/poco/Poco.Path.html

            Path cwd = Path::current();                  // get the current path
            ofPath = cwd.resolve(ofPath).toString();   // resolve ofPath vs that.
            Path resolvedPath = Path(ofPath).absolute();    // make that new path absolute
            ofPath = resolvedPath.toString();

            if (!isGoodOFPath(ofPath)) {
                return Application::EXIT_USAGE;
            }
            setOFRoot(ofPath);
        }


        if(bListTemplates){
            printTemplates();
            consoleSpace();
            return Application::EXIT_OK;
        }

        //-------------------------- get the path to the current working folder
        Path cwd = Path::current();                      // get the current path
        projectPath = cwd.resolve(projectPath).toString();  // resolve projectPath vs that.
        Path resolvedPath = Path(projectPath).absolute();         // use absolute version of this path
        projectPath = resolvedPath.toString();

        // check things
		if (args.size() > 0) {
			projectName = args[0];

			// check if it's an absolute path?
			if (ofFilePath::isAbsolute(projectName)) {
				projectPath = projectName;
			} else {
				projectPath = ofFilePath::join(projectPath, projectName);

				// this line is arturo's ninja magic to make paths with dots make sense:
				projectPath = ofFilePath::removeTrailingSlash(ofFilePath::getPathForDirectory(ofFilePath::getAbsolutePath(projectPath, false)));
			}

		} else {
		    ofLogError() << "Missing project path";
		    printHelp();

			consoleSpace();

			return Application::EXIT_USAGE;
		}



		if (!isGoodOFPath(ofPath)) {
			consoleSpace();
			ofLogError() << "path to openframeworks (" << ofPath << ") seems wrong, please check";
			consoleSpace();
			return Application::EXIT_USAGE;
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
                consoleSpace();
				ofLogNotice() << "target platform is: " << target;
				ofLogNotice() << "project path is: " << projectPath;

                if(templateName!="standard"){
                    consoleSpace();
                    ofLogNotice() << "using additional template " << templateName;
                }
                consoleSpace();


                ofLogNotice() << "setting up new project " << projectPath;
                consoleSpace();
				if (!bDryRun) project->create(projectPath, templateName);
				
                if (!bDryRun){
                    for(auto & addon: addons){
                        project->addAddon(addon);
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
                        consoleSpace();
                        ofLogNotice() << "target platform is: " << getTargetString(targets[i]);
                        
                        if(templateName!="standard"){
                            consoleSpace();
                            ofLogNotice() << "using additional template " << templateName;
                        }
                        consoleSpace();

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
        if (nProjectsCreated > 0) cout << nProjectsCreated << " project created ";
        if (nProjectsUpdated == 1)cout << nProjectsUpdated << " project updated ";
        if (nProjectsUpdated > 1) cout << nProjectsUpdated << " projects updated ";
        ofLogNotice() << "in " << elapsedTime << " seconds" << endl;
        consoleSpace();

		return Application::EXIT_OK;
	}

};



POCO_APP_MAIN(commandLineProjectGenerator)
