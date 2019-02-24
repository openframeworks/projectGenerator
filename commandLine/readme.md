This is a command line tool for generating and updating OF projects. 

## usage

here's the extended options if you want to experiment.  please take are that options like recursive are very agressive, dryrun is suggested.

	      projectGenerator [options] pathName
	
	if pathName exists, project is updated
	if pathName doesn't exist, project is created
	
	OPTIONS:
	
	lists should be comma seperated and in quotes if there are spaces
	you can use : or = for parameter based options, such as -o=/usr/...
	
	-r, --recursive                                      update recursively
	                                                     (applies only to update)
	-h, --help
	-l, --listtemplates                                  list templates available
	                                                     for the specified or
	                                                     current platform(s)
	-p"platform list", --platforms="platform list"       platform list
	-a"addons list", --addons="addons list"              addons list
	-o"OF path", --ofPath="OF path"                      openframeworks path
	-v, --verbose                                        run verbose
	-t"project_template", --template="project_template"  project template
	-d, --dryrun                                         don't change files


**note that on windows, command line flags are written with "/" not "-" and don't have abbreviations, so -r would be /recursive**

	/recursive                    update recursively (applies only to update)
	/help
	/listtemplates                list templates available for the specified or
                              	      current platform(s)
	/platforms="platform list"    platform list
	/addons="addons list"         addons list
	/ofPath="OF path"             openframeworks path
	/verbose                      run verbose
	/template="project_template"  project template
	/dryrun                       don't change files


for example, to make a new project, you'd say: 

    ./projectGenerator -o"pathToOF" pathOfNewProject
    (on windows: ./projectGenerator /ofPath="pathToOF" pathOfNewProject)

to update an existing project: 

    ./projectGenerator -o"pathToOF" pathToExistingProject

to update a folder of projects

    ./projectGenerator -o"pathToOF" -r pathToFolderOfProjects

you can also specify platforms (default platform is inferred) such as: 

    ./projectGenerator -o"pathToOF" -p"ios" pathOfNewIOSProject





## Environment variable

If you want to skip using `-o` (for specifying the path to openframeworks) you can set an environment variable "PG_OF_PATH".  For example, on OSX, you can set this in your `~/.bash_profile`: 

     export PG_OF_PATH="MY_FULL_PATH_TO_OF"
     
where MY_FULL_PATH_TO_OF is the path, such as "/Users/zachlieberman/Documents/openFrameworks"


## osx notes

when you compile this on OSX, we remove the fmod dynamic lib dependency and strip the exectuable from the app package (on osx, OF compiles to an app but the actual exectuable is deeper in the package).  This is happening the project.xcconfig and in the build phases if you are curious.   (note that this is part of the nofmod template).
