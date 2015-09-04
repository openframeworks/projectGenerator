when you compile this on OSX, we remove the fmod dynamic lib dependency and strip the exectuable from the app package (on osx, OF compiles to an app but the actual exectuable is deeper in the package).  This is happening the project.xcconfig and in the build phases if you are curious.   (note that this is part of the nofmod template).

here's the extended options if you want to experiment.  please take are that options like recursive and update are very agressive, dryrun is suggested.


	usage:


	      projectGenerator [options] pathName

	if pathName exists, project is updated
	if pathName doesn't exist, project is created

	OPTIONS:

	lists should be comma seperated and in quotes if there are spaces
	you can use : or = for parameter based options, such as -o=/usr/...

	-r, --recursive                                 update recursively (applies
	                                                only to update)
	-h, --help
	-x"platform list", --platforms="platform list"  platform list
	-a"addons list", --addons="addons list"         addons list
	-o"OF path", --ofPath="OF path"                 openframeworks path
	-v, --verbose                                   run verbose
	-d, --dryrun                                    don't change files
