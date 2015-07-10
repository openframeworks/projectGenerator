#!/bin/bash
set -e
cd ..
git clone --depth=1 https://github.com/openframeworks/openFrameworks
mv projectGenerator openFrameworks/apps/
cd openFrameworks/apps/projectGenerator
echo "Building openFrameworks PG - OSX"
xcodebuild -configuration Release -target projectGeneratorSimple -project projectGeneratorSimple/projectGeneratorSimple.xcodeproj
ret=$?
if [ $ret -ne 0 ]; then
      echo "Failed building Project Generator"
      exit 1
fi
mv projectGeneratorSimple/bin/data/settings/projectGeneratorSettings_production.xml projectGeneratorSimple/bin/data/settings/projectGeneratorSettings.xml
cp scripts/ssh_config ~/.ssh/config
chmod 600 scripts/id_rsa
scp -i scripts/id_rsa -r projectGeneratorSimple/bin tests@192.237.185.151:projectGeneratorSimple_osx
rm scripts/id_rsa
