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
if [ "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" = "openframeworks/projectGenerator/master" ]; then
    mv projectGeneratorSimple/bin/data/settings/projectGeneratorSettings_production.xml projectGeneratorSimple/bin/data/settings/projectGeneratorSettings.xml
    cp scripts/ssh_config ~/.ssh/config
    chmod 600 scripts/id_rsa
    ssh -i scripts/id_rsa tests@192.237.185.151 "rm -f projectGeneratorSimple_osx.zip"
    mv projectGeneratorSimple/bin projectGenerator_osx
    zip -r projectGenerator_osx.zip projectGenerator_osx
    scp -i scripts/id_rsa projectGenerator_osx.zip tests@192.237.185.151:projectGenerator_osx.zip
fi
rm scripts/id_rsa

