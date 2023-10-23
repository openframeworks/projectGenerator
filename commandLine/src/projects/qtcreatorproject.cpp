#include "qtcreatorproject.h"
#include "ofLog.h"
#include "ofFileUtils.h"
#include "Utils.h"
#include <regex>

std::string QtCreatorProject::LOG_NAME = "QtCreatorProject";

QtCreatorProject::QtCreatorProject(const std::string & target) : baseProject(target) {}

bool QtCreatorProject::createProjectFile(){
//	alert("QtCreatorProject::createProjectFile " + projectDir.string());
	if (!fs::exists(projectDir)) {
		fs::create_directory(projectDir);
	}

	fs::path qbsFile { fs::path { projectName + ".qbs" } };
	vector < std::pair <fs::path, fs::path > > fromTo {
		{ "qtcreator.qbs", qbsFile },
		{ "Makefile", "Makefile" },
		{ "config.make", "config.make" },
	};

	for (auto & p : fromTo) {
		// FIXME: Wrong paths here. there are some more folders like "wizard" / "openFrameworks"
		fs::path src { templatePath / p.first };
		fs::path dst { projectDir / p.second };
		try {
			fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
		} catch(fs::filesystem_error& e) {
			ofLogError(LOG_NAME) << "error copying template file " << src << " : " << dst << " : " << e.what();
			return false;
		}
	}

	// TODO: This opens twice the same file to find and replace. maybe we can make another function with std::pairs of find, replace strings
	findandreplaceInTexfile(qbsFile, "emptyExample", projectName);

	// Calculate OF Root in relation to each project (recursively);
	auto relRoot { fs::relative((fs::current_path() / getOFRoot()), projectDir) };
	if (!fs::equivalent(relRoot, "../../..")) {
		string root = relRoot.string();
		for (auto & p : fromTo) {
			findandreplaceInTexfile(p.second, "../../..", root);
		}
	}
	return true;
}


void QtCreatorProject::addSrc(const fs::path & srcFile, const fs::path & folder, baseProject::SrcType type){
	qbsProjectFiles.insert(srcFile.string());
}


bool QtCreatorProject::loadProjectFile(){
	fs::path file { projectDir / (projectName + ".qbs") };
	if (!fs::exists(file)) {
		ofLogError(LOG_NAME) << "error loading" << file << "doesn't exist";
		return false;
	}

	std::ifstream project(file);
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
				auto addonStr = a.isLocalAddon ? a.addonPath.string() : a.name;
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
		auto addonStr = a.isLocalAddon ? a.addonPath.string() : a.name;
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

	fs::path fileName = projectDir / (projectName + ".qbs");
	std::ofstream projectFile(fileName);
	try{
		projectFile << qbs;
	}catch(std::exception & e){
		ofLogError("") << "Error saving " << fileName << " : " << e.what();
		return false;
	}catch(...){
		ofLogError("") << "Error saving " << fileName;
		return false;
	}
//	ofFile project(projectDir / (projectName + ".qbs"),ofFile::WriteOnly,true);
//	project.writeFromBuffer(qbs);
	return true;
}


void QtCreatorProject::addAddon(ofAddon & addon){
	// FIXME: I think this is unneded since this function here is triggered by baseclass::addAddon(string) which already checked if exists.
//	for (auto & a : addons) {
//		if (a.name == addon.name) return;
//	}
	addons.emplace_back(addon);
}
