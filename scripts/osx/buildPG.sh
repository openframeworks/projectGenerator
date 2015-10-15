#!/bin/bash
set -e
cd ..
git clone --depth=1 https://github.com/openframeworks/openFrameworks
mv projectGenerator openFrameworks/apps/
cd openFrameworks/apps/projectGenerator
echo "Building openFrameworks PG - OSX"
xcodebuild -configuration Release -target commandLine -project commandLine/commandLine.xcodeproj
ret=$?
if [ $ret -ne 0 ]; then
      echo "Failed building Project Generator"
      exit 1
fi
if [ "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" = "openframeworks/projectGenerator/master" ]; then
    cp scripts/ssh_config ~/.ssh/config
    chmod 600 scripts/id_rsa
    scp -i scripts/id_rsa commandLine/bin/projectGenerator tests@198.61.170.130:projectGenerator_builds/projectGenerator_osx_new
    ssh -i scripts/id_rsa tests@198.61.170.130 "mv projectGenerator_builds/projectGenerator_osx_new projectGenerator_builds/projectGenerator_osx"
fi
rm scripts/id_rsa

