#!/bin/bash
set -e



echoDots(){
    sleep 0.1 # Waiting for a brief period first, allowing jobs returning immediatly to finish
    while isRunning $1; do
        for i in $(seq 1 10); do
            echo -ne .
            if ! isRunning $1; then
                printf "\r"
                return;
            fi
            sleep 1
        done
        printf "\r                    "
        printf "\r"
    done
}



cd ..
of_root=${PWD}/openFrameworks
pg_root=${PWD}/openFrameworks/apps/projectGenerator

#brew install npm

git clone --depth=1 https://github.com/openframeworks/openFrameworks
mv projectGenerator openFrameworks/apps/

# cd ${of_root}
# scripts/osx/download_libs.sh

# Compile commandline tool
# cd ${pg_root}
# echo "Building openFrameworks PG - OSX"
# xcodebuild -configuration Release -target commandLine -project commandLine/commandLine.xcodeproj
# ret=$?
# if [ $ret -ne 0 ]; then
#       echo "Failed building Project Generator"
#       exit 1
# fi


# Generate electron app
cd ${pg_root}/frontend
npm install > /dev/null
npm run build:osx > /dev/null
mv dist/projectGenerator-darwin-x64 ${pg_root}/projectGenerator-osx

# Copy commandLine into electron .app
cd ${pg_root}
# cp commandLine/bin/projectGenerator projectGenerator-osx/projectGenerator.app/Contents/Resources/app/app/projectGenerator 2> /dev/null
wget http://ci.openframeworks.cc/projectGenerator/projectGenerator_osx -O projectGenerator-osx/projectGenerator.app/Contents/Resources/app/app/projectGenerator 2> /dev/null
sed -i -e "s/osx/osx/g" projectGenerator-osx/projectGenerator.app/Contents/Resources/app/settings.json

# Sign app
echo "Creating signing certificates"
openssl aes-256-cbc -K $encrypted_489c559678c5_key -iv $encrypted_489c559678c5_iv -in scripts/developer_ID.p12.enc -out developer_ID.p12 -d
security create-keychain -p mysecretpassword build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p mysecretpassword build.keychain
security import developer_ID.p12 -k build.keychain -T /usr/bin/codesign
security find-identity -v
sudo npm install -g electron-osx-sign

echo "Signing electron .app"
electron-osx-sign projectGenerator-osx/projectGenerator.app
appsignPID=$!
echoDots $appsignPID
wait $appsignPID

echo "Signing command line binary"
codesign --deep --force --verbose --sign "Developer ID Application: Arturo Castro" projectGenerator-osx/projectGenerator.app/Contents/Resources/app/app/projectGenerator
clsignPID=$!
echoDots $clsignPID
wait $clsignPID

echo "Compressing PG app"
zip projectGenerator-osx.zip projectGenerator-osx

# Upload to OF CI server
echo "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}";
if [ "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" = "openframeworks/projectGenerator/master" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then
    echo "Uploading app to CI servers"
    openssl aes-256-cbc -K $encrypted_cd38768cbb9d_key -iv $encrypted_cd38768cbb9d_iv -in scripts/id_rsa.enc -out scripts/id_rsa -d
    cp scripts/ssh_config ~/.ssh/config
    chmod 600 scripts/id_rsa
    scp -i scripts/id_rsa commandLine/bin/projectGenerator tests@198.61.170.130:projectGenerator_builds/projectGenerator_osx_new
    ssh -i scripts/id_rsa tests@198.61.170.130 "mv projectGenerator_builds/projectGenerator_osx_new projectGenerator_builds/projectGenerator_osx"
fi
rm -rf scripts/id_rsa

