#!/bin/bash
set -e
echo "Building openFrameworks PG - OSX"
xcodebuild -configuration Release -target projectGeneratorSimple -project projectGeneratorSimple/projectGeneratorSimple.xcodeproj
ret=$?
if [ $ret -ne 0 ]; then
      echo "Failed building Project Generator"
      exit 1
fi
mv projectGeneratorSimple/bin/data/projectGeneratorSettings_production.xml projectGeneratorSimple/bin/data/projectGeneratorSettings.xml
mkdir -p ~/.ssh
mv id_rsa ~/.ssh/
scp -r projectGeneratorSimple/bin tests@192.237.185.151:projectGeneratorSimple_osx
rm ~/.ssh/id_rsa
