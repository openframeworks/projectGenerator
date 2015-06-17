# projectGenerator

a tool for making and updating openframeworks project files

# a bit of history

the OF project generator ("PG") was originally an attempt to make a tool that would standardize a variety of homegrown tools we were making for OF.  Prior to the PG, we had a series of scripts for updating OF projects and making releases and these had a variety of different features and approaches.  The original PG (the one which has lived for years in devApps/projectGenerator) was designed as a tool for developers, ie, not necessarily intuitive, but with things like batching in mind.  It has a linux based command line approach as well, which is used for packaging.  OF is developed without project files and this PG helps make the releases.

At some point, it became clear that we needed a much simpler project generator (sometimes referred to as "pg simple") for end users to be able to make their own projects.  Making a new project and updating / adding addons to a project is one of the largest challenges that beginners have in OF.  There were parts of the original PG that had grown slightly complex, such as saving invisible files that specify where the root of OF is and a kind of more production oriented interface.   PG simple was an attempt make an end user tool that we could distribute.  It's lived in a seperate repo and been a submodule to OF here (https://github.com/openframeworks/openFrameworks/tree/master/apps/projectGenerator).

At the OF dev conference in YCAM Japan a team re-designed the project generator interface, adding more advanced features and generally making a more intutive tool for making projects.  That work is currently under development.   As part of that work, there's been some energy to make a command line version of the project generator that works across platforms and would allow for simple and complex usages. 

Finally, as part of a general attempt to bring all these projects closer together and to offer more guidance, this repo is being created to have one place to host development / issues / features of the PG.  
