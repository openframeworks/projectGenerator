#pragma once

#define PG_VERSION "57"

#include "ofAddon.h"
#include "pugixml.hpp"
#include <map>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = of::filesystem;

class baseProject {
public:
	enum LibType {
		DEBUG_LIB = 0,
		RELEASE_LIB
	};

	enum SrcType {
		DEFAULT,
		HEADER,
		CPP,
		C,
		OBJC
	};

	struct Template {
//		ofDirectory dir;
		fs::path dir;
		std::string name;
		std::vector<string> platforms;
		std::string description;
		std::map<fs::path, fs::path> renames;
		bool operator<(const Template & other) const{
			return dir<other.dir;
		}
	};

	baseProject(const std::string & _target);

	virtual ~baseProject(){}

	bool create(const fs::path & path, std::string templateName="");
	void parseAddons();
	void parseConfigMake();
	bool save();

	virtual void addSrc(const fs::path & srcFile, const fs::path & folder, SrcType type=DEFAULT) = 0;
	virtual void addInclude(std::string includeName) = 0;
	virtual void addLibrary(const LibraryBinary & lib) = 0;

	// FIXME: change some strings to const &
	virtual void addLDFLAG(std::string ldflag, LibType libType = RELEASE_LIB){}
	virtual void addCFLAG(std::string cflag, LibType libType = RELEASE_LIB){} // C_FLAGS
	virtual void addCPPFLAG(std::string cppflag, LibType libType = RELEASE_LIB){} // CXX_FLAGS
	virtual void addAfterRule(std::string script){}
	virtual void addDefine(std::string define, LibType libType = RELEASE_LIB) {}

	virtual void addAddon(std::string addon);
	virtual void addAddon(ofAddon & addon);
	virtual void addSrcRecursively(const fs::path & srcPath);

	bool isPlatformName(const string & platform);

	std::string getName() { return projectName; }
	fs::path getPath() { return projectDir; }

	std::vector<Template> listAvailableTemplates(std::string target);
	std::unique_ptr<baseProject::Template> parseTemplate(const fs::path & templateDir);
	virtual fs::path getPlatformTemplateDir();

	pugi::xml_document doc;
	bool bLoaded;

	fs::path projectDir;
	fs::path templatePath;
	std::string projectName;
	std::string target;

	bool bMakeRelative = false;





	// this shouldn't be called by anyone.  call "create(...), save" etc
private:

	virtual bool createProjectFile()=0;
	virtual bool loadProjectFile()=0;
	virtual bool saveProjectFile()=0;

	// virtual void renameProject();
	// this should get called at the end.

protected:
	void recursiveCopyContents(const fs::path & srcDir, const fs::path & destDir);
	void recursiveTemplateCopy(const fs::path & srcDir, const fs::path & destDir);
	bool recursiveCopy(const fs::path & srcDir, const fs::path & destDir);

	std::vector<ofAddon> addons;
	std::vector<fs::path> extSrcPaths;

	//cached addons - if an addon is requested more than once, avoid loading from disk as it's quite slow
	std::unordered_map<std::string,std::unordered_map<std::string, ofAddon>> addonsCache; //indexed by [platform][supplied path]
	bool isAddonInCache(const std::string & addonPath, const std::string platform); //is this addon in the mem cache?
	
	static void replaceAll(std::string& str, const std::string& from, const std::string& to) {
		if(from.empty())
			return;
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
	}

	struct copyTemplateFile {
	public:
		fs::path from;
		fs::path to;
		std::vector <std::pair <string, string> > findReplaces;
		
        bool run() {
               if (fs::exists(from)) {
                   if (fs::exists(to)) {
                       std::ifstream fileTo(to);
                       std::string existingContents((std::istreambuf_iterator<char>(fileTo)), std::istreambuf_iterator<char>());
                       fileTo.close();

                       std::ifstream fileFrom(from);
                       std::string templateContents((std::istreambuf_iterator<char>(fileFrom)), std::istreambuf_iterator<char>());
                       fileFrom.close();

                       std::string mergedContents = mergePlistFiles(existingContents, templateContents);

                       try {
                           std::ofstream fileOut(to);
                           fileOut << mergedContents;
                       } catch (std::exception& e) {
                           std::cout << "Error saving to " << to << " : " << e.what() << std::endl;
                           return false;
                       } catch (...) {
                           std::cout << "Error saving to " << to << std::endl;
                           return false;
                       }
                   } else {
                       try {
                           fs::copy(from, to, fs::copy_options::overwrite_existing);
                       } catch (fs::filesystem_error& e) {
                           std::cout << "error copying template file " << from << " to " << to << std::endl;
                           std::cout << e.what() << std::endl;
                           return false;
                       }
                   }
               } else {
                   return false;
               }

               return true;
           }
     

       std::string mergePlistFiles(const std::string& existingContents, const std::string& templateContents) {
           pugi::xml_document existingDoc;
               existingDoc.load_string(existingContents.c_str());

               pugi::xml_document templateDoc;
               templateDoc.load_string(templateContents.c_str());

               pugi::xml_node existingDict = existingDoc.child("plist").child("dict");
               pugi::xml_node templateDict = templateDoc.child("plist").child("dict");

               std::map<std::string, std::string> existingMap;
               std::map<std::string, std::string> templateMap;

               for (pugi::xml_node node = existingDict.first_child(); node; node = node.next_sibling("key")) {
                   std::string key = node.child_value();
                   std::string value;
                   pugi::xml_node valueNode = node.next_sibling();
                   if (std::string(valueNode.name()) == "string") {
                       value = valueNode.child_value();
                   } else {
                       value = valueNode.name();
                   }
                   existingMap[key] = value;
               }

               for (pugi::xml_node node = templateDict.first_child(); node; node = node.next_sibling("key")) {
                   std::string key = node.child_value();
                   std::string value;
                   pugi::xml_node valueNode = node.next_sibling();
                   if (std::string(valueNode.name()) == "string") {
                       value = valueNode.child_value();
                   } else {
                       value = valueNode.name();
                   }
                   templateMap[key] = value;
               }

               for (const auto& entry : templateMap) {
                   if (existingMap.find(entry.first) == existingMap.end()) {
                       existingMap[entry.first] = entry.second;
                   }
               }

            std::string mergedContents = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\">\n<dict>\n";
               for (const auto& entry : existingMap) {
                   mergedContents += "\t<key>" + entry.first + "</key>\n";
                   if (entry.second == "true" || entry.second == "false") {
                       mergedContents += "\t<" + entry.second + "/>\n";
                   } else {
                       mergedContents += "\t<string>" + entry.second + "</string>\n";
                   }
               }
               mergedContents += "</dict>\n</plist>\n";

               return mergedContents;
       }

        void replaceAll(std::string& str, const std::string& from, const std::string& to) {
            size_t startPos = 0;
            while ((startPos = str.find(from, startPos)) != std::string::npos) {
                str.replace(startPos, from.length(), to);
                startPos += to.length();
            }
        }
	};

	vector <copyTemplateFile> copyTemplateFiles;
};
