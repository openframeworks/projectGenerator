# openFrameworks projectGenerator 

[![build-linux](https://github.com/openframeworks/projectGenerator/actions/workflows/build-linux.yml/badge.svg)](https://github.com/openframeworks/projectGenerator/actions/workflows/build-linux.yml) [![build-vs](https://github.com/openframeworks/projectGenerator/actions/workflows/build-vs.yml/badge.svg)](https://github.com/openframeworks/projectGenerator/actions/workflows/build-vs.yml) [![build-macos](https://github.com/openframeworks/projectGenerator/actions/workflows/build-macos.yml/badge.svg)](https://github.com/openframeworks/projectGenerator/actions/workflows/build-macos.yml)


# Project Generator
## Builds the cmdline and GUI project generators for openFrameworks

The project Generator, also known as _PG_,
is a utility tool to facilitate the creation of new openFrameworks projects,
in multiple platforms.

- the commandLine projectGenerator (pure openFrameworks)
- the projectGenerator GUI app (electron) distributed in releases

The content in this page is primarily target at developers willing to contribute to the code base.
If you are working with openFrameworks and would like to use the Project Generator, check the
instructions in [Create in a new project](https://openframeworks.cc/learning/01_basics/create_a_new_project/).

---------------------------------------


# Download instructions 
You can download packaged macOS and Windows builds here ( note Linux needs to be built per platform ):
- https://github.com/openframeworks/projectGenerator/releases/download/nightly/projectGenerator-osx.zip
- https://github.com/openframeworks/projectGenerator/releases/download/nightly/projectGenerator-vs-gui.zip 
- https://github.com/openframeworks/projectGenerator/releases/download/nightly/projectGenerator-vs.zip (commmandLine)

---------------------------------------

# Build instructions with Bash Scripts 

In the projectGenerator scripts folder run the scripts in order to setup environment and build


## Visual Studio 2022 

projectGenerator\scripts\vs\setup_environment.sh (first time)

```projectGenerator\scripts\vs\build_cmdline.sh```
- (build projectGenerator commandLine requires openFrameworks in directories above / or run from correct sub-module location)

```projectGenerator\scripts\vs\build_frontend.sh``` (Builds Electron App GUI Project Generator) 
- requires commandLine.exe in folder. 
- If cannot build commandLine.exe download from releases and run just this

projectGenerator\scripts\vs\build_dev_frontend.sh 
 - (Builds Electron App GUI Project Generator in DEBUG mode / Verbose and will autorun the test build) 

## macOS / osx

projectGenerator\scripts\osx\setup_environment.sh (first time)

```projectGenerator\scripts\vs\build_cmdline.sh```
- (build projectGenerator commandLine requires openFrameworks in directories above / or run from correct sub-module location)

```projectGenerator\scripts\vs\build_frontend.sh``` (Builds Electron App GUI Project Generator) 
- requires commandLine.exe in folder. 
- If cannot build commandLine.exe download from releases and run just this

## Linux

```projectGenerator\scripts\linux\setup_environment.sh``` (first time)
```projectGenerator\scripts\linux\build_cmdline.sh```
```projectGenerator\scripts\linux\build_frontend.sh```


## Contributing

The project generator is available in two flavors:

- A command line utility tool. This option is the most versatile and provides all features.
  See more information [here](/commandLine/README.md).

- A user-interface, which works for the most typical scenarios. See more information in
the [front-end](/frontend/README.md) folder.  
 ![project generator front-end](/frontend/screenshot.png)


# LEGAGY BUILD INSTRUCTIONS ----- (more detail)
---------------------------------------

## Clone OF ( skip if already cloned ) 
```
git clone git@github.com:openframeworks/openFrameworks.git --depth=1
```

## Init / update submodules 
```
cd openFrameworks
git submodule init
git submodule update
```

## Install libs ( replace with platform )
`./scripts/osx/download_libs.sh`

## Build the projectGenerator ( can also use project file ) 
```
cd apps/projectGenerator/commandLine/

# for linux do:
make Release
cp bin/projectGenerator ../frontend/app/

# for windows do:
msbuild commandLine.vcxproj /p:configuration=release /p:platform=${{ matrix.platform }} /p:PlatformToolset=v143
cp commandLine.exe ../frontend/app/projectGenerator.exe

# for macos 
xcodebuild -configuration Release -target commandLine CODE_SIGN_IDENTITY="" -project commandLine.xcodeproj
cp bin/projectGenerator ../frontend/app/
```
On macOS, if you get this error: 
> xcode-select: error: tool 'xcodebuild' requires Xcode, but active developer directory '/Library/Developer/CommandLineTools' is a command line tools instance

It's probably because your Command Line Tools were installed via Home Brew or some other mechanism. You need to re-select the whole Xcode install by executing:
```
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
```
## Build the frontend electron app 
Note: see the more detailed guide include node install in the frontend [ReadMe](frontend/ReadMe.md)
```
cd ../frontend
npm install 
npm update

# to run the app without packaging it 
npm start

# to package the pg app as a standalone
npm run build:macos 
```
