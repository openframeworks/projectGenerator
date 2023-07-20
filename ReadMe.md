# Download instructions 
You can download packaged macOS and Windows builds here ( note Linux needs to be built per platform ):
- https://github.com/openframeworks/projectGenerator/releases/download/nightly/projectGenerator-osx.zip
- https://github.com/openframeworks/projectGenerator/releases/download/nightly/projectGenerator-vs.zip 

# Build instructions 

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
msbuild commandLine.vcxproj /p:configuration=release /p:platform=${{ matrix.platform }} /p:PlatformToolset=v142
cp commandLine.exe ../frontend/app/projectGenerator.exe

# for macos 
xcodebuild -configuration Release -target commandLine CODE_SIGN_IDENTITY="" -project commandLine.xcodeproj
cp bin/projectGenerator ../frontend/app/
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
