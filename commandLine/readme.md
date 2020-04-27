This is a command line tool for generating and updating OF projects. 

## usage

here's the extended options if you want to experiment.  please take are that options like recursive are very agressive, dryrun is suggested.
<!-- (projectGenerator -h) -->
```
openFrameworks's project generator
Usage:
  projectGenerator [OPTION...]

  -p, --project arg    path to project (required)
  -o, --ofPath arg     path to openFrameworks directory (required, unless
                       PG_OF_PATH is set)
  -a, --addons arg     comma separated list of addons
  -s, --platforms arg  comma separated list of platforms
  -t, --template arg   template
  -h, --help           prints usage
  -v, --verbose        verbose output
  -d, --dryrun         do dry run (useful for debugging recursive update)
  -r, --recursive      recursively updates projects
  -l, --listtemplates  prints a list of available templates


examples:

update the project at the current working directory:
        projectGenerator -p . -o ../../..

create a new project:
        projectGenerator -p /path/to/oF/app/myApps/nonExistingDirectory -o /path/to/oF

recursively update the examples folder:
        projectGenerator -r -p /path/to/oF/examples -o /path/to/oF

create or update a project with addons:
        projectGenerator -a ofxOsc,ofxOpenCv -p /path/to/oF/apps/myApps/appWithAddons -o /path/to/oF
        projectGenerator -a "ofxOsc, ofxOpenCv" -p /path/to/oF/apps/myApps/appWithAddons -o /path/to/oF
```




## Environment variable

If you want to skip using `-o` (for specifying the path to openframeworks) you can set an environment variable "PG_OF_PATH".  For example, on OSX, you can set this in your `~/.bash_profile`: 

     export PG_OF_PATH="MY_FULL_PATH_TO_OF"
     
where MY_FULL_PATH_TO_OF is the path, such as "/Users/zachlieberman/Documents/openFrameworks"


## osx notes

when you compile this on OSX, we remove the fmod dynamic lib dependency and strip the exectuable from the app package (on osx, OF compiles to an app but the actual exectuable is deeper in the package).  This is happening the project.xcconfig and in the build phases if you are curious.   (note that this is part of the nofmod template).
