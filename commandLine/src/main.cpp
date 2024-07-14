#define TARGET_NO_SOUND
#define TARGET_NODISPLAY

#include "Utils.h"
#include "defines.h"
#include "ofUtils.h"
#include "optionparser.h"
#include <set>
#include <string>
#include <cstdlib>

enum optionIndex { UNKNOWN,
	HELP,
	PLUS,
	RECURSIVE,
	LISTTEMPLATES,
	PLATFORMS,
	ADDONS,
	OFPATH,
	VERBOSE,
	TEMPLATE,
	DRYRUN,
	SRCEXTERNAL,
	VERSION,
	GET_OFPATH,
	GET_HOST_PLATFORM,
	COMMAND
};

constexpr option::Descriptor usage[] = {
	{ UNKNOWN, 0, "", "", option::Arg::None, "Options:\n" },
	{ HELP, 0, "h", "help", option::Arg::None, "  --help  \tPrint usage and exit." },
	{ RECURSIVE, 0, "r", "recursive", option::Arg::None, "  --recursive, -r  \tupdate recursively (applies only to update)" },
	{ LISTTEMPLATES, 0, "l", "listtemplates", option::Arg::None, "  --listtemplates, -l  \tlist templates available for the specified or current platform(s)" },
	{ PLATFORMS, 0, "p", "platforms", option::Arg::Optional, "  --platforms, -p  \tplatform list (such as osx, ios, winvs)" },
	{ ADDONS, 0, "a", "addons", option::Arg::Optional, "  --addons, -a  \taddon list (such as ofxOpenCv, ofxGui, ofxXmlSettings)" },
	{ OFPATH, 0, "o", "ofPath", option::Arg::Optional, "  --ofPath, -o  \tpath to openframeworks (relative or absolute). This *must* be set, or you can also alternatively use an environment variable PG_OF_PATH and if this isn't set, it will use that value instead" },
	{ VERBOSE, 0, "v", "verbose", option::Arg::None, "  --verbose, -v  \trun verbose" },
	{ TEMPLATE, 0, "t", "template", option::Arg::Optional, "  --template, -t  \tproject template" },
	{ DRYRUN, 0, "d", "dryrun", option::Arg::None, "  --dryrun, -d  \tdry run, don't change files" },
	{ SRCEXTERNAL, 0, "s", "source", option::Arg::Optional, "  --source, -s  \trelative or absolute path to source or include folders external to the project (such as ../../../../common_utils/" },
	{ VERSION, 0, "w", "version", option::Arg::None, "  --version, -w  \treturn the current version" },
    { GET_OFPATH, 0, "g", "getofpath", option::Arg::None, "  --getofpath, -g  \treturn the current ofPath" },
    { GET_HOST_PLATFORM, 0, "h", "gethost", option::Arg::None, "  --gethost, -h  \treturn the current host platform" },
    { COMMAND, 0, "c", "command", option::Arg::None, "  --command, -c \truns command" },
	{ 0, 0, 0, 0, 0, 0 }
};

#define EXIT_OK 0
#define EXIT_USAGE 64
#define EXIT_DATAERR 65
#define STRINGIFY(A) #A

enum pgMode {
	PG_MODE_NONE,
	PG_MODE_CREATE,
	PG_MODE_UPDATE
};

float startTime;
int nProjectsUpdated;
int nProjectsCreated;

fs::path projectPath;
fs::path ofPath;
vector<string> addons;
vector<fs::path> srcPaths;
vector<string> targets;
string ofPathEnv;
string templateName;

bool busingEnvVar;
bool bVerbose;
bool bAddonsPassedIn;
bool bForce; // force even if things like ofRoot seem wrong of if update folder looks wonky
pgMode mode; // what mode are we in?
bool bRecursive; // do we recurse in update mode?
bool bHelpRequested; // did we request help?
bool bListTemplates; // did we request help?
bool bDryRun; // do dry run (useful for debugging recursive update)

void consoleSpace() {
	std::cout << std::endl;
}

void printVersion() {
	std::cout << OFPROJECTGENERATOR_MAJOR_VERSION << "." << OFPROJECTGENERATOR_MINOR_VERSION << "." << OFPROJECTGENERATOR_PATCH_VERSION << std::endl;
}

void printOFPath() {
    std::cout << ofPath.string() << endl;
}

bool printTemplates() {
	if (targets.size() > 1) {
		vector<vector<baseProject::Template>> allPlatformsTemplates;
		for (auto & target : targets) {
			auto templates = getTargetProject(target)->listAvailableTemplates(target);
			allPlatformsTemplates.emplace_back(templates);
		}
		std::set<baseProject::Template> commonTemplates;
		for (auto & templates : allPlatformsTemplates) {
			for (auto & t : templates) {
				bool foundInAll = true;
				for (auto & otherTemplates : allPlatformsTemplates) {
					auto found = false;
					for (auto & otherT : otherTemplates) {
						if (otherT.name == t.name) {
							found = true;
							continue;
						}
					}
					foundInAll &= found;
				}
				if (foundInAll) {
					commonTemplates.insert(t);
				}
			}
		}
		if (commonTemplates.empty()) {
			ofLogNotice() << "No templates available for all targets";
			return false;
		} else {
			ofLogNotice() << "Templates available for all targets";
			for (auto & t : commonTemplates) {
				ofLogNotice() << t.name << "\t\t" << t.description;
			}
			return true;
		}
	} else {
		bool templatesFound = false;
		for (auto & target : targets) {
			ofLogNotice() << "Templates for target " << target;
			auto templates = getTargetProject(target)->listAvailableTemplates(target);
			for (auto & templateConfig : templates) {
				ofLogNotice() << templateConfig.name << "\t\t" << templateConfig.description;
			}
			consoleSpace();
			templatesFound = !templates.empty();
		}
		return templatesFound;
	}
}

std::string normalizePath(const std::string& path) {
    try {
        auto value = std::filesystem::weakly_canonical(path).string();
        return value;
    } catch (const std::exception& ex) {
        std::cout << "Canonical path for [" << path << "] threw exception:\n"
                  << ex.what() << '\n';
        return "";
    }
}

std::filesystem::path normalizePath(const std::filesystem::path& path) {
    try {
        auto value = std::filesystem::weakly_canonical(path);
        return value;
    } catch (const std::exception& ex) {
        std::cout << "Canonical path for [" << path << "] threw exception:\n"
                  << ex.what() << '\n';
        return std::filesystem::path("");
    }
}

void handleCommand(const std::string& args) {
    if (args == "ping") {
        std::cout << "pong" << args << std::endl;
    } else {
        std::cout << "Unknown custom command: " << args << std::endl;
    }
}


void addPlatforms(const string & value) {
	targets.clear();
	vector<string> platforms = ofSplitString(value, ",", true, true);

	for (auto & p : platforms) {
		if (p == "allplatforms") {
			for (auto & option : platformsOptions) {
				targets.emplace_back(option);
			}
		} else {
			targets.emplace_back(p);
		}
	}
}

bool containsFolder(fs::path path, string folderName) {
	bool contains = false;
	for (const auto & entry : fs::directory_iterator(path)) {
		auto f = entry.path();
		if (f.filename() == folderName) {
			contains = true;
			break;
		}
	}
	return contains;
}

bool isGoodProjectPath(fs::path path) {
	// TODO: think of a way of detecting make obj folders which creates a structure similar to project
	// like this assimp3DModelLoaderExample/obj/osx/Release/src
	return fs::exists(path / "src");
}

bool isGoodOFPath(const fs::path & path) {
	if (!fs::is_directory(path)) {
	//		ofLogVerbose() << "ofPath: not a directory: " << path.string();
		return false;
	}
	bool bHasTemplates = containsFolder(path, "scripts");
	bHasTemplates = bHasTemplates && containsFolder(path, "addons");
	bHasTemplates = bHasTemplates && containsFolder(path, "libs");
	//	if (!bHasTemplates) ofLogVerbose() << "ofPath no addons / libs / scripts / templates directory " << path.string();
	return bHasTemplates;
}

fs::path findOFPathUpwards(const fs::path & startPath) {
	fs::path currentPath = startPath;
	//    ofLogNotice() << "startPath: " << currentPath.string();
	if (currentPath.empty() || currentPath == currentPath.root_path()) {
		ofLogError() << "Starting path is empty or root path, cannot find OF path.";
		return fs::path();
	}
	while (!currentPath.empty() && currentPath != currentPath.root_path()) {
	//        ofLogWarning() << "currentPath: " << currentPath.string();
		if (isGoodOFPath(currentPath)) {
	//            ofLogWarning() << "isGoodOFPath: " << currentPath.string();
			return currentPath;
		} else {
	//            ofLogWarning() << "not good OF Path: " << currentPath.string();
		}
		currentPath = currentPath.parent_path();
	}
	//    ofLogError() << "No valid OF path found... please use -o or --ofPath or set a PG_OF_PATH environment variable: " << startPath.string();
	return fs::path();
}

void updateProject(const fs::path & path, const string & target, bool bConsiderParameterAddons = true) {
	//    alert("updateProject path=" + path.string() , 34);
	// bConsiderParameterAddons = do we consider that the user could call update with a new set of addons
	// either we read the addons.make file, or we look at the parameter list.
	// if we are updating recursively, we *never* consider addons passed as parameters.
	cout << endl;
	ofLogNotice() << "updating project " << path;

	if (!bDryRun) {
		auto project = getTargetProject(target);
		project->create(path, templateName);

		if (bConsiderParameterAddons && bAddonsPassedIn) {
			for (auto & addon : addons) {
				project->addAddon(addon);
			}
		} else {
			ofLogNotice() << "parsing addons.make";
			project->parseAddons();
		}

		for (auto & srcPath : srcPaths) {
			project->addSrcRecursively(srcPath);
		}
		project->save();
	}
}

void recursiveUpdate(const fs::path & path, const string & target) {
	// FIXME: remove
	alert("recursiveUpdate:" + path.string() + "]");
	if (!fs::is_directory(path)) return;
	vector<fs::path> folders;

	// second check if this is a folder that has src in it
	if (isGoodProjectPath(path)) {
		folders.emplace_back(path);
	}

	// MARK: - Known issue. it can add undesired folders which can mirror directory of a valid project like
	// "./templates/allAddonsExample/obj/osx/Release"
	// "./templates/allAddonsExample/bin/build/build/arm64-apple-darwin_Release/obj.room/Volumes/tool/ofw/addons/ofxKinect"

	for (const auto & p : folderList(path)) {
		if (p.filename() == "src") {
			auto parent = p.parent_path();
			if (isGoodProjectPath(parent)) {
				folders.emplace_back(parent);
			}
		}
	}

	fs::path ofCalcPath = fs::weakly_canonical(fs::current_path() / ofPath);

	for (auto & path : folders) {
		//		cout << "------" << endl;
		//		cout << path << endl;
		//		cout << "------" << endl;
		nProjectsUpdated++;

		if (!ofPath.is_absolute()) {
			ofPath = ofCalcPath;
			if (ofIsPathInPath(path, ofPath)) {
				ofPath = fs::relative(ofCalcPath, path);
			}
		}
//		setOFRoot(ofPath);
		fs::current_path(path);
		//alert ("ofRoot " + ofPath.string());
		//alert ("cwd " + path.string());

		updateProject(path, target, false);
	}
}

int updateOFPath() {
    std::string ofPath;
	std::string ofPathEnv;
	#ifdef _WIN32
		char* envValue = nullptr;
		size_t len = 0;
		if (_dupenv_s(&envValue, &len, "PG_OF_PATH") == 0 && envValue != nullptr) {
			ofPathEnv = std::string(envValue);
			free(envValue);
		}
	#else
		const char* envValue = std::getenv("PG_OF_PATH");
		if (envValue) {
			ofPathEnv = std::string(envValue);
		}
	#endif

	if ((ofPath.empty() && !ofPathEnv.empty()) ||
		(!ofPath.empty() && !isGoodOFPath(ofPath) && !ofPathEnv.empty())) {
		ofPath = normalizePath(ofPathEnv);
		ofLogNotice() << "PG_OF_PATH set: ofPath [" << ofPath << "]";
	}
    
    fs::path startPath = normalizePath(ofFilePath::getCurrentExeDirFS());
    //ofFilePath::getAbsolutePathFS(fs::current_path(), false);
//    ofLogNotice() << "startPath: " << startPath.string();
    fs::path foundOFPath = findOFPathUpwards(startPath);
    if (foundOFPath.empty() && ofPath.empty()) {
        ofLogError() << "oF path not found: please use -o or --ofPath or set 'PG_OF_PATH' environment var. Auto up folders from :[" << startPath.string() << "]";
        return EXIT_FAILURE;
    } else {
        if (!ofPath.empty() && isGoodOFPath(ofPath)) {
            ofLogNotice() << "ofPath set and valid using [" << ofPath << "]";
        } else {
            if(isGoodOFPath(foundOFPath))
            ofPath = foundOFPath.string();
            ofLogNotice() << "ofPath auto-found and valid using [" << ofPath << "]";
        }
    }
    
    if (!ofPath.empty()) {
        if (!isGoodOFPath(ofPath)) {
            foundOFPath = findOFPathUpwards(ofPath);
            if (foundOFPath.empty()) {
                ofLogNotice() << "ofPath not valid. [" << ofPath << "] auto-find ofPath failed also...";
                return EXIT_USAGE;
            }
        }
//        if (ofIsPathInPath(projectPath, ofPath)) {
//            fs::path path = fs::relative(ofPath, projectPath);
//            ofPath = path.string();
//        }
        ofPath = normalizePath(ofPath);
        setOFRoot(ofPath);
    }
    
    return EXIT_OK;
}

void printHelp() {
	consoleSpace();

	string header = "";
	header += "\tprojectGenerator [options] pathName\n\n";
	header += "if pathName exists, project is updated\n";
	header += "if pathName doesn't exist, project is created\n";
	header += "(pathName must follow options, which can come in any order)";
	std::cout << header << std::endl;

	consoleSpace();

	option::printUsage(std::cout, usage);

	consoleSpace();

	string footer = "";
	footer += "examples:\n\n";
	footer += STRINGIFY(
		projectGenerator -o"../../../../" ../../../../apps/myApps/newExample
	);
	footer += "\n";
	footer += "(create a project called newExample using a relative path for the OF root and the project. note the relative path may be different depending on where this app is located)";
	footer += "\n\n";
	footer += STRINGIFY(
			  projectGenerator -r -o"../../../../" ../../../../examples
			  );
	footer += "\n";
	footer += "(recursively update the examples folder)";
	footer += "\n\n";
	footer += STRINGIFY(
			  projectGenerator -o"../../../../" -a"ofxXmlSettings, ofxOpenCv" ../../../../apps/myApps/newExample
			  );
	footer += "\n";
	footer += "(create / update an example with addons)";
	std::cout << footer << std::endl;

	consoleSpace();
}

int main(int argc, char ** argv) {
	ofLog() << "PG v." + getPGVersion();
	bAddonsPassedIn = false;
	bDryRun = false;
	busingEnvVar = false;
	bVerbose = true;
	mode = PG_MODE_NONE;
	bForce = false;
	bRecursive = false;
	bHelpRequested = false;
	bListTemplates = false;
	// FIXME! problem.
	targets.emplace_back(platformsToString[ofGetTargetPlatform()]);
	startTime = 0;
	nProjectsUpdated = 0;
	nProjectsCreated = 0;
	string projectName = "";
	//    projectPath = "";
	//	ofPath = "";
	templateName = "";

	// ------------------------------------------------------ parse args
	argc -= (argc > 0);
	argv += (argc > 0); // skip program name argv[0] if present
	option::Stats stats(usage, argc, argv);
	vector<option::Option> options(stats.options_max);
	vector<option::Option> buffer(stats.buffer_max);

	option::Parser parse(usage, argc, argv, &options[0], &buffer[0]);

	if (parse.error()) {
		return 1;
	}
    
    if (options[VERBOSE].count() > 0) {
        bVerbose = true;
    }

	// templates:
	if (options[LISTTEMPLATES].count() > 0) {
		bListTemplates = true;
	}

	if (options[RECURSIVE].count() > 0) {
		bRecursive = true;
	}

	if (options[DRYRUN].count() > 0) {
		bDryRun = true;
	}
    
    if (options[VERSION].count() > 0) {
        printVersion();
        return EXIT_OK;
    }
    
	int updated = updateOFPath();

	if (options[HELP] || argc == 0) {
		ofLogError() << "No arguments";
		printHelp();
		return EXIT_OK;
	}

	if (options[COMMAND].count() > 0) {
		if (options[COMMAND].arg != NULL) {
			string argument(options[COMMAND].arg);
			handleCommand(argument);
		} else {
			ofLogError() << "Custom command requires an argument";
			return EXIT_USAGE;
		}
		return EXIT_OK;
	}

	if (options[GET_OFPATH].count() > 0) {
		getOFRoot();
		return EXIT_OK;
	}

	if (options[GET_HOST_PLATFORM].count() > 0) {
		ofLogNotice() << ofGetTargetPlatform();
		return EXIT_OK;
	}
    
	if (options[TEMPLATE].count() > 0) {
		if (options[TEMPLATE].arg != NULL) {
			string templateString(options[TEMPLATE].arg);
			templateName = templateString;
		}
	}

	if (options[PLATFORMS].count() > 0) {
		if (options[PLATFORMS].arg != NULL) {
			string platformString(options[PLATFORMS].arg);
			addPlatforms(platformString);
		}
	}

	//fix as we want to treat vscode as a platform not a template
	if (templateName == "vscode") {
		templateName = "";
		addPlatforms("vscode");
	}

	if (options[ADDONS].count() > 0) {
		bAddonsPassedIn = true; // could be empty
		if (options[ADDONS].arg != NULL) {
			string addonsString(options[ADDONS].arg);
			addons = ofSplitString(addonsString, ",", true, true);
		}
	}

	// Additional Src Paths
	if (options[SRCEXTERNAL].count() > 0) {
		if (options[SRCEXTERNAL].arg != NULL) {
			string srcString(options[SRCEXTERNAL].arg);
			// TODO: TEST
			for (auto & s : ofSplitString(srcString, ",", true, true)) {
				s = ofFilePath::removeTrailingSlash(s);
				srcPaths.emplace_back(s);
//				alert ("external source folder : " + s, 31);
			}
		}
	}

	if (options[OFPATH].count() > 0) {
		if (options[OFPATH].arg != NULL) {
			ofPath = options[OFPATH].arg;
            ofLogNotice() << "ofPath arg [" << ofPath << "]";
            ofPath = normalizePath(ofPath);
            ofLogNotice() << "ofPath normalised arg [" << ofPath << "]";
		}
	}

	if (parse.nonOptionsCount() > 0) {
		projectName = parse.nonOption(0);
	}

	// ------------------------------------------------------ post parse

	nProjectsUpdated = 0;
	nProjectsCreated = 0;
	of::priv::initutils();
	startTime = ofGetElapsedTimef();
	consoleSpace();

	// try to get the OF_PATH as an environt variable
    

    fs::path projectPath = fs::weakly_canonical(fs::current_path() / projectName);

    // Verify that projectPath is not root and exists
    if (!projectPath.empty() && projectPath != projectPath.root_path() && fs::exists(projectPath)) {
        fs::create_directory(projectPath);
    } else {
        ofLogError() << "Invalid or non-existent project path: [" << projectPath << "]";
        return EXIT_FAILURE;
    }

    
	if (bListTemplates) {
		auto ret = printTemplates();
		consoleSpace();
		if (ret) {
			return EXIT_OK;
		} else {
			return EXIT_DATAERR;
		}
	}

	if (projectName == "") {
		ofLogError() << "Missing project path";
		printHelp();
		consoleSpace();
		return EXIT_USAGE;
	}

	if (bVerbose) {
		ofSetLogLevel(OF_LOG_VERBOSE);
	}

	if (bRecursive) {
		for (auto & t : targets) {
			ofLogNotice() << "-----------------------------------------------";
			ofLogNotice() << "updating an existing project";
			ofLogNotice() << "target platform is: " << t;

			// MARK: - RECURSIVE UPDATE
			recursiveUpdate(projectPath, t);

			ofLogNotice() << "project updated! ";
			ofLogNotice() << "-----------------------------------------------";
		}
	} else {
		if (mode == PG_MODE_UPDATE && !isGoodProjectPath(projectPath)) {
			ofLogError() << "there is no src folder in this project path to update, maybe use create instead? (or use force to force updating)";
		} else {
			nProjectsCreated += 1;

			for (auto & t : targets) {
				ofLogNotice() << "-----------------------------------------------";
				ofLogNotice() << "target platform is: [" << t << "]";
                ofLogNotice() << "setting OF path to: [" << ofPath << "]";
				if (busingEnvVar) {
					ofLogNotice() << "from PG_OF_PATH environment variable";
				} else {
					ofLogNotice() << "from -o option";
				}
				ofLogNotice() << "project path is: [" << projectPath << "]";
				if (templateName != "") {
					ofLogNotice() << "using additional template " << templateName;
				}
				ofLogNotice() << "setting up new project " << projectPath;

				if (mode == PG_MODE_UPDATE) {
					// MARK: - UPDATE
					updateProject(projectPath, t);
					ofLogNotice() << "project updated! ";
				} else {
					if (!bDryRun) {
						auto project = getTargetProject(t);
						project->create(projectPath, templateName);
						for (auto & addon : addons) {
							project->addAddon(addon);
						}
						for (auto & s : srcPaths) {
							project->addSrcRecursively(s);
						}
						project->save();
					}
					ofLogNotice() << "project created! ";
				}
				ofLogNotice() << "-----------------------------------------------";
				consoleSpace();
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
