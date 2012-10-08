# project generator

please note that the project generator is a work in progress / alpha.  We'll be retooling it based on feedback we get. 

## about

this is the simple version of the openframeworks project generator.  This tool has been exiting inside of the develop banch of openframeworks (from github) but not in releases, and with 0072, we are starting to move this tool into releases.   It's very much a work in progress, so here are some points and caveats: 

* the tool can be used to generate project files for the platform you've downloaded (osx xcode, windows codeblocks etc)
* the project generator can add addons to your project, but not all addons (especially non core addons) are packaged in the right way for this
* it assumes you will want to put your project in "root/apps/myApps", but you can put your projects anywhere relative to OF, and it should work. Putting your apps in non standard paths (ie, not at a height of ../../../) is  

## how to use the project generator

It's pretty simple to make an new project, there are 3 things you can adjust: 

**name of the project**

this is where you set the name of the project.  This creates both a folder with this name and a project file, so for example, if your name is "myCoolExample" it will generate: 

myCoolExample/myCoolExample.xcodeproj

where the myCoolExample will have all the src / settings / etc of an exmpty example. 

**path of the project**

this defaults to apps/myApps, but it should allow you to put projects anywhere.  We strongly recommend you to keep them inside this release of OF, so that if the OF release or your project get moved, or if some lower level folder gets renamed, the generated paths don't break.  it's much safer to have paths that are "../../../" vs paths that have a name in them.  please note that because we also create a folder with this tool with the name of your project, the actuall full path to your project will look like: 

chosenPath/projectName/projectName.project

(where chosenPath and projectName are based on your settings, and .project is the xcode, code blocks, visual studio file that's generated)

**addons**

to use, choose addons in the gui then hit the back button. 

here you can select the addons you'd like included.  Not all addons work, but all of the core addons should and if the addon doesn't work, you can tell the addon developer to take a look at it.  The project generator is basically searching the file system recursively through the addons, and it's looking for things like folders that say "osx" on them, etc, so it can make decisions about what to include / exclude, etc.  For most simple addons, this should work fairly well, but we haven't tested with many addons yet.   

After this release is out, we'll spend some time to document exactly how our system parses, and some of the more intricate nuances that will help addon developers make PG friendly releases. 

**generate**

when you hit this button, hopefully magic will happen.  Some addons take a very long time to parse (such as opencv) so please be patient.  If you add alot of addons, it can sometimes take several seconds for a project to generate. 

## Differences with the version in the openframeworks develop branch

* the version in github can generate projects for mutiple platforms (since you will have all the necessary templates, etc if you've got the whole OF repo cloned)
* the version in github can generate recursively.  It does this for generating the OF releases project files.
* the version on github can update a project that alraedy exists, useful if you are trying to port existing work on one platform or release of OF to another. 

## where to find the project generator

the project generator simple version lives here: 

https://github.com/ofZach/projectGeneratorSimple

the more complete one, with the git repo, is here: 

https://github.com/openframeworks/openFrameworks/tree/master/apps/devApps/projectGenerator

since there's common code, we'll be moving these two back into one spot, but right now they're at two locations. 

If you have issues / questions / suggestions on this, please use projectGeneratorSimple repo. 

## thanks

thanks to Navit Keren and Patricio Gonzalez Vivo for their help on the UX design, and the OF dev team / mailing list for testing. 