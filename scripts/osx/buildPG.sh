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

git clone --depth=1 https://github.com/openframeworks/openFrameworks
mv projectGenerator openFrameworks/apps/

cd ${of_root}
scripts/osx/download_libs.sh

# Compile commandline tool
cd ${pg_root}
echo "Building openFrameworks PG - OSX"
xcodebuild -configuration Release -target commandLine -project commandLine/commandLine.xcodeproj
ret=$?
if [ $ret -ne 0 ]; then
      echo "Failed building Project Generator"
      exit 1
fi


# Generate electron app
cd ${pg_root}/frontend
npm install > /dev/null
npm run build:osx > /dev/null
mv dist/projectGenerator-darwin-x64 ${pg_root}/projectGenerator-osx

# Copy commandLine into electron .app
cd ${pg_root}
cp commandLine/bin/projectGenerator projectGenerator-osx/projectGenerator.app/Contents/Resources/app/app/projectGenerator 2> /dev/null
# wget http://ci.openframeworks.cc/projectGenerator/projectGenerator_osx -O projectGenerator-osx/projectGenerator.app/Contents/Resources/app/app/projectGenerator 2> /dev/null
sed -i -e "s/osx/osx/g" projectGenerator-osx/projectGenerator.app/Contents/Resources/app/settings.json

echo "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}";
if [ "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" = "openframeworks/projectGenerator/master" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then
# Sign app
    echo "Decoding signing certificates"
    cd ${pg_root}/scripts
    openssl aes-256-cbc -K $encrypted_b485a78f2982_key -iv $encrypted_b485a78f2982_iv -in developer_ID.p12.enc -out developer_ID.p12 -d
    echo "Creating keychain"
    security create-keychain -p mysecretpassword build.keychain
    echo "Setting keychain as default"
    security default-keychain -s build.keychain
    echo "Unlocking keychain"
    security unlock-keychain -p mysecretpassword build.keychain
    security set-keychain-settings -t 3600 -u build.keychain
    echo "Importing signing certificates"
    sudo security import developer_ID.p12 -k build.keychain -P $CERT_PWD -T /usr/bin/codesign
    # security set-key-partition-list -S apple-tool:,apple: -s -k mysecretpassword build.keychain
    # security find-identity -v

    echo "Signing electron .app"
    cd ${pg_root}
    sudo npm install -g electron-osx-sign
    xattr -cr projectGenerator-osx/projectGenerator.app
    electron-osx-sign projectGenerator-osx/projectGenerator.app --platform=darwin --type=distribution
    # codesign --deep --force --verbose --sign "Developer ID Application: Arturo Castro" projectGenerator-osx/projectGenerator.app


    echo "Compressing PG app"
    zip --symlinks -r -q projectGenerator-osx.zip projectGenerator-osx

# Upload to OF CI server
    echo "Uploading app to CI servers"
    openssl aes-256-cbc -K $encrypted_cd38768cbb9d_key -iv $encrypted_cd38768cbb9d_iv -in scripts/id_rsa.enc -out scripts/id_rsa -d
    cp scripts/ssh_config ~/.ssh/config
    chmod 600 scripts/id_rsa
    scp -i scripts/id_rsa projectGenerator-osx.zip tests@198.61.170.130:projectGenerator_builds/projectGenerator-osx_new.zip
    ssh -i scripts/id_rsa tests@198.61.170.130 "mv projectGenerator_builds/projectGenerator-osx_new.zip projectGenerator_builds/projectGenerator-osx.zip"
fi
rm -rf scripts/id_rsa
rm -rf scripts/developer_ID.p12

