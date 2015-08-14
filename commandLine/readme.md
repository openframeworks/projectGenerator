when you compile this on OSX, we remove the fmod dynamic lib dependency and strip the exectuable from the app package (on osx, OF compiles to an app but the actual exectuable is deeper in the package).  This is happening the project.xcconfig and in the build phases if you are curious.

once compiled you should see commandLinePG next to the package -- give that a try.  that's what you want to use.

here's an example of creating a project, setting the OF path (../../../../) which is the path to OF from that point in the repo, using the create mode, and setting the path of the project you want to create (paths can be relative or absolute)

./commandLinePG -o "../../../../" -c -p "../../../../apps/myApps/test"

here's the extended options if you want to experiment.  please take are that options like recursive and update are very agressive, dryrun is suggested.


	OPTIONS:
	A command line project generator
	please use a command "create", "update"
	lists should be comma seperated
	and in quotes if there are spaces
	you can use : or = for parameter based options, such as -o=/usr/...

	-r, --recursive                                 update recursively (applies
	                                                only to update)
	-h, --help
	-c, --create                                    create a project file if it
	                                                doesn't exist
	-u, --update                                    update a project file if it
	                                                does exist
	-x"platform list", --platforms="platform list"  platform list
	-a"addons list", --addons="addons list"         addons list
	-o"OF path", --ofPath="OF path"                 openframeworks path
	-p"project path", --projectPath="project path"  project path
	-v, --verbose                                   run verbose
	-d, --dryrun                                    don't change files
