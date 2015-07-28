when you compile this on OSX, we remove fmod and strip the exectuable from the package.  This is happening the project.xcconfig and in the build phases.
you should see commandLinePG next to the package -- give that a try.

here's an example of creating a project, setting the OF path (../../../../), using the create mode, and setting the path of the project you want.

./commandLinePG -o "../../../../" -c -p "../../../../apps/myApps/test"
