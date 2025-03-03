#define TARGET_NO_SOUND
#define TARGET_NODISPLAY

#include "Utils.h"
#include "defines.h"
#include "ofUtils.h"
#include "ofFileUtils.h"
#include "ofSystemUtils.h"
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
	COMMAND,
	BACKUP_PROJECT_FILES,
	FRAMEWORKS,
	CLEANNAME_DISABLE
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
	{ GET_HOST_PLATFORM, 0, "i", "platform", option::Arg::None, "  --getplatform, -i  \treturn the current host platform" },
	{ COMMAND, 0, "c", "command", option::Arg::None, "  --command, -c \truns command" },
	{ BACKUP_PROJECT_FILES, 0, "b", "backup", option::Arg::None, "  --backup, -b  \tbackup project files when replacing with template" },
	
	{ FRAMEWORKS, 0, "f", "frameworks", option::Arg::Optional, "  --frameworks, -f  \tframeworks list (such as Vision,ARKit)" },
	
	{ CLEANNAME_DISABLE, 0, "n", "cleanname", option::Arg::Optional, "  --cleanname, -f  \tcleanname" },

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
fs::path defaultAppPath = { "apps/myApps" };
fs::path generatorPath;
fs::path ofPath;
vector<string> addons;
vector<fs::path> srcPaths;
vector<string> targets;
vector<string> frameworks;
string ofPathEnv;
string templateName;

bool busingEnvVar;
bool bVerbose;
bool bAddonsPassedIn;
bool bForce; // force even if things like ofRoot seem wrong of if update folder looks wonky
pgMode mode; // what mode are we in? //mode is never set to anything else. this is unnecesary.
bool bRecursive; // do we recurse in update mode?
bool bHelpRequested; // did we request help?
bool bListTemplates; // did we request help?
bool bDryRun; // do dry run (useful for debugging recursive update)
bool bBackup;
bool bCleanName = true;

void consoleSpace() {
	std::cout << std::endl;
}

void printVersion() {
	messageReturn("version", PG_VERSION);
}

void printOFPath() {
	std::cout << ofPath.string() << endl;
}

void setofPath(const fs::path& path) {
	ofLogVerbose() << " setofPath: [" << path << "] ";
	ofPath = path;
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

void handleCommand(const std::string & args) {
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
		ofLogVerbose() << "ofPath: not a directory: " << path;
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
		ofLogError() << "Starting path is empty or root path, cannot find OF path." << "\" }";
		return {};
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
	return {};
}

void updateProject(const fs::path & path, const string & target, bool bConsiderParameterAddons = true) {
	// alert("updateProject path=" + path.string() , 34);
	// bConsiderParameterAddons = do we consider that the user could call update with a new set of addons
	// either we read the addons.make file, or we look at the parameter list.
	// if we are updating recursively, we *never* consider addons passed as parameters.
	consoleSpace();
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
		for (auto & f : frameworks) {
			project->addFramework(f, "Frameworks", true);
		}

		for (auto & srcPath : srcPaths) {
			project->addSrcRecursively(srcPath);
		}
		project->save();
	}
}

void recursiveUpdate(const fs::path & path, const string & target) {
	// FIXME: remove
//	alert("recursiveUpdate :[" + path.string() + "]");
	ofLogNotice() << "recursiveUpdate " << path;
	
	if (!fs::is_directory(path)) return;
	vector<fs::path> folders;

	// second check if this is a folder that has src in it
	if (isGoodProjectPath(path)) {
		folders.emplace_back(path);
	}

	// MARK: Known issue. it can add undesired folders which can mirror directory of a valid project like
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
		nProjectsUpdated++;

		if (!ofPath.is_absolute()) {
			setofPath(ofCalcPath);
			if (ofIsPathInPath(path, ofPath)) {
				setofPath(fs::relative(ofCalcPath, path));
			}
		}

		updateProject(path, target, false);
	}
}

int updateOFPath(fs::path path) {

	std::string ofPathEnv;
	auto envValue = ofGetEnv("PG_OF_PATH");
	if(!envValue.empty()) {
		ofPathEnv = std::string(envValue);
		ofPathEnv = normalizePath(ofPathEnv);
	}

	if ((ofPath.empty() && !ofPathEnv.empty()) ||
		((!ofPath.empty() && !isGoodOFPath(ofPath)) &&
		 (!ofPathEnv.empty() && isGoodOFPath(ofPathEnv)))) {
		setofPath(ofPathEnv);
		ofLogNotice() << "PG_OF_PATH set: ofPath [" << ofPath << "]";
	}
	of::filesystem::path exePath = ofFilePath::getCurrentExeDir();

	fs::path startPath = normalizePath(exePath);
	generatorPath = startPath;
	ofLogVerbose() << "projectGenerator cmd path: {" << startPath << "] }";
	//ofFilePath::getAbsolutePathFS(fs::current_path(), false);
//    ofLogNotice() << "startPath: " << startPath.string();
	fs::path foundOFPath = findOFPathUpwards(startPath);
	if (foundOFPath.empty() && ofPath.empty()) {
		ofLogError() << "{ \"errorMessage: \"" << "oF path not found: please use -o or --ofPath or set 'PG_OF_PATH' environment var. Auto up folders from :[" << startPath.string() << "]" << "\" }";
		return EXIT_FAILURE;
	} else {
		if (!ofPath.empty() && isGoodOFPath(ofPath)) {
			ofLogNotice() << "ofPath set and valid using [" << ofPath << "]";
		} else {
			if(isGoodOFPath(foundOFPath))
			setofPath(foundOFPath);
			setOFRoot(foundOFPath);
			ofLogVerbose() << "ofPath auto-found and valid using [" << ofPath << "]";
		}
	}

	if (!ofPath.empty()) {
		if (!isGoodOFPath(ofPath)) {
			foundOFPath = findOFPathUpwards(ofPath);
			if (foundOFPath.empty()) {
				ofLogNotice() << "{ \"errorMessage: \"" << "ofPath not valid. [" << ofPath << "] auto-find ofPath failed also..." << "\" }";
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
	messageReturn("openFrameworks projectGenerator", getPGVersion());
	bAddonsPassedIn = false;
	bDryRun = false;
	bBackup = false;
	busingEnvVar = false;
	bVerbose = false;
	mode = PG_MODE_NONE;
	bForce = false;
	bRecursive = false;
	bHelpRequested = false;
	bListTemplates = false;
	targets.emplace_back(getPlatformString());

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
		messageError("Parse error for arguments");
		return 1;
	}

	if (options[VERBOSE].count() > 0) {
		bVerbose = true;
	}

	if (options[BACKUP_PROJECT_FILES].count() > 0) {
		bBackup = true;
		backupProjectFiles = bBackup;
		ofLogVerbose() << "Backup project files: true";
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
	
	if (options[CLEANNAME_DISABLE].count() > 0) {
		bCleanName = false;
	}
	parseCleanName = bCleanName;

	if (options[VERSION].count() > 0) {
		printVersion();
		return EXIT_OK;
	}
	

	if (options[OFPATH].count() > 0) {
		if (options[OFPATH].arg != NULL) {
			setofPath(options[OFPATH].arg);
			ofLogVerbose() << "ofPath arg: [" << ofPath << "]";
			setofPath(normalizePath(ofPath));
			ofLogVerbose() << "ofPath normalised arg: [" << ofPath << "]";
		}
	}
	int updated = updateOFPath(ofPath);

	if (options[COMMAND].count() > 0) {
		if (options[COMMAND].arg != NULL) {
			string argument(options[COMMAND].arg);
			handleCommand(argument);
		} else {
			messageError("Custom command requires an argument");
			return EXIT_USAGE;
		}
		return EXIT_OK;
	}

	if (options[GET_OFPATH].count() > 0) {
		ofLogNotice() << "{ \"ofPath\": " << getOFRoot() << " }";
		return EXIT_OK;
	}

	if (options[GET_HOST_PLATFORM].count() > 0) {
		ofLogNotice() <<  "{ \"ofHostPlatform\": \"" << platformsToString[ofGetTargetPlatform()] << "\" }";
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

#ifndef DEBUG_NO_OPTIONS
	if (options[HELP] || argc == 0) {
		printHelp();
		messageError("No arguments");
		return EXIT_OK;
	}
#endif

	if (options[FRAMEWORKS].count() > 0) {
		bAddonsPassedIn = true; // could be empty
		if (options[FRAMEWORKS].arg != NULL) {
			frameworks = ofSplitString(options[FRAMEWORKS].arg, ",", true, true);
//			cout << "frameworks " << options[FRAMEWORKS].arg << endl;
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

	if (bVerbose) {
		ofSetLogLevel(OF_LOG_VERBOSE);
	}

	if (projectName == "") {
		printHelp();
		consoleSpace();
		messageError("Missing project path");
		return EXIT_USAGE;
	}

	fs::path projectPath = normalizePath(fs::weakly_canonical(fs::current_path() / projectName));
	fs::path projectNamePath = projectPath.filename();
	projectName = projectNamePath.string();


	ofLogVerbose() << " projectPath path: [" << projectPath << "] root_path: [" << projectPath.root_path() << "]";
	ofLogVerbose() << " ofPath path: [" << ofPath << "]";
	ofLogVerbose() << " ofRoot path: [" << getOFRoot()  << "]";

	if(projectPath == projectPath.root_path()) {
		ofLogVerbose() << " !! projectPath == projectPath.root_path() ";
	} else if(normalizePath(fs::weakly_canonical( projectPath.root_path() / projectName )) == projectPath) {
		ofLogVerbose() << " !! normalizePath(fs::weakly_canonical( projectPath.root_path() / projectName )) == projectPath ";
		ofLogVerbose() << " !! fs::weakly_canonical( projectPath.root_path() / projectName )):=" << fs::weakly_canonical( projectPath.root_path() / projectName );
		ofLogVerbose() << " !! normalizePath(fs::weakly_canonical( projectPath.root_path() / projectName )):=" << normalizePath(fs::weakly_canonical( projectPath.root_path() / projectName ));
		ofLogVerbose() << " !! projectPath:=" << projectPath;
	} else if(normalizePath(fs::weakly_canonical( generatorPath / projectName )) == projectPath) {
		ofLogVerbose() << " !! normalizePath(fs::weakly_canonical( generatorPath / projectName )) == projectPath ";
	}
	if(projectPath.empty() ) {
		projectPath = normalizePath(fs::weakly_canonical( getOFRoot() / defaultAppPath / projectName));
		ofLogNotice() << " projectPath.empty() path now: [" << projectPath << "]";
	} else if(projectPath == projectPath.root_path() || // if projectPath == "/"
			  normalizePath(fs::weakly_canonical( projectPath.root_path() / projectName )) == projectPath || // or /projectName
			  normalizePath(fs::weakly_canonical( generatorPath / projectName )) == projectPath // or generatorPath/projectName
			  ){
		ofLogVerbose() << " fs::weakly_canonical( [" << fs::weakly_canonical( projectPath.root_path() / projectName ) << "]";
		ofLogVerbose() << " projectPath.root_path(): [" << projectPath.root_path() << "]";
		projectPath =  normalizePath(fs::weakly_canonical( getOFRoot() / defaultAppPath / projectName));
		ofLogNotice() << " projectPath issue managed, path now: [" << projectPath << "]";
	} else {
		ofLogVerbose() << " projectPath path: [" << projectPath << "]";
	}
	if(projectPath.empty()) {
		messageError( "Invalid project path: {" + projectPath.string() + "}");
		return EXIT_FAILURE;
	}
	// make folder
	if (!fs::exists(projectPath)) {
		try {
			ofLogVerbose() << " creating projectPath directory.";
			fs::create_directories(projectPath);
		} catch (const std::exception& ex) {
			messageError( "fs::create_directory failed, \"projectPath\": { \""+ projectPath.string() + "\" }, \"exception\": \""
			+ ex.what() + "\"");
			return EXIT_FAILURE;
		}
	} else {
		if (fs::exists(projectPath)) {
			ofLogVerbose() << " The project path exists.";
			if (fs::is_directory(projectPath)) {
				ofLogVerbose() << "  and it is a directory.";
			} else {
				ofLogVerbose() << "  and It is a file...";
			}
		}
	}

	if (bListTemplates) {
		auto ret = printTemplates();
		consoleSpace();
		if (ret) {
			messageReturn("status", "EXIT_OK");
			return EXIT_OK;
		} else {
			messageError("printTemplates data error");
			return EXIT_DATAERR;
		}
	}

	if (bRecursive) {
		ofLogVerbose() << "project path is: [" << projectPath << "]";
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

			ofLogNotice() << "setting OF path to: [" << ofPath << "]";
			if (busingEnvVar) {
				ofLogNotice() << "from PG_OF_PATH environment variable";
			} else {
				ofLogNotice() << "from -o option";
			}

			
			for (auto & t : targets) {
				consoleSpace();
				ofLogNotice() << "-----------------------------------------------";
				ofLogNotice() << "target platform is: [" << t << "]";
//				ofLogNotice() << "project path is: [" << projectPath << "]";
				if (templateName != "") {
					ofLogNotice() << "using additional template " << templateName;
				}
				ofLogVerbose() << "setting up new project " << projectPath;

				if (mode == PG_MODE_UPDATE) {
					// MARK: - UPDATE
					updateProject(projectPath, t);
					ofLogNotice() << "project updated! ";
				} else {
					if (!bDryRun) {
//						ofLogNotice() << "project path is: [" << projectPath << "]";
						auto project = getTargetProject(t);
						project->create(projectPath, templateName);
						if(bAddonsPassedIn){
							for (auto & addon : addons) {
								project->addAddon(addon);
							}
						}else{
							project->parseAddons();
						}
						for (auto & f : frameworks) {
							project->addFramework(f, "Frameworks", true);
						}

						for (auto & s : srcPaths) {
							project->addSrcRecursively(s);
						}
						project->save();
					}
					ofLogNotice() << "project created! ";
				}
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

	messageReturn("status", "EXIT_OK");
	return EXIT_OK;
}
