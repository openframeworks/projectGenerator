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


sign_and_upload(){
    PLATFORM=$1
    # Copy commandLine into electron .app
    cd ${pg_root}
    cp commandLine/bin/projectGenerator projectGenerator-$PLATFORM/projectGenerator.app/Contents/Resources/app/app/projectGenerator 2> /dev/null
        
    sed -i -e "s/osx/$PLATFORM/g" projectGenerator-$PLATFORM/projectGenerator.app/Contents/Resources/app/settings.json
    
    if [[ -z "${GA_CI_SECRET}" ]] ; then
        echo " Not on main repo skipping sign and upload ";
    else
        if [[ "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/master" && "$TRAVIS_PULL_REQUEST" == "false" ]] || [[ "${GITHUB_REF##*/}" == "master" &&  -z "${GITHUB_HEAD_REF}" ]] ; then
            # Sign app
            echo "Signing electron .app"
            cd ${pg_root}
            xattr -cr projectGenerator-$PLATFORM/projectGenerator.app
            # codesign --deep --force --verbose --sign "Developer ID Application: Arturo Castro" "projectGenerator-$PLATFORM/projectGenerator.app"
            
            electron-osx-sign projectGenerator-$PLATFORM/projectGenerator.app --platform=darwin --type=distribution --no-gatekeeper-assess --hardened-runtime --entitlements=scripts/osx/PG.entitlements --entitlements-inherit=scripts/osx/PG.entitlements

            echo "Compressing PG app"
            zip --symlinks -r -q projectGenerator-$PLATFORM.zip projectGenerator-$PLATFORM
            
            # need to upload zip of just app to apple for notarizing
            zip --symlinks -r -q projectGenerator-$PLATFORM/projectGenerator.app.zip projectGenerator-$PLATFORM/projectGenerator.app
            xcrun altool --notarize-app --primary-bundle-id "com.electron.projectgenerator" --username "${GA_APPLE_USERNAME}" -p "${GA_APPLE_PASS}" --asc-provider "${GA_NOTARIZE_PROVIDER}" --file projectGenerator-$PLATFORM/projectGenerator.app.zip

            # Upload to OF CI server
            echo "Uploading $PLATFORM PG to CI servers"
            
            if [ "$GITHUB_ACTIONS" = true ]; then
                echo Unencrypting key for github actions
                openssl aes-256-cbc -salt -md md5 -a -d -in scripts/githubactions-id_rsa.enc -out scripts/id_rsa -pass env:GA_CI_SECRET
                mkdir -p ~/.ssh
            else
                echo Unencrypting key for travis
                openssl aes-256-cbc -K $encrypted_cd38768cbb9d_key -iv $encrypted_cd38768cbb9d_iv -in scripts/id_rsa.enc -out scripts/id_rsa -d
            fi
            
            cp scripts/ssh_config ~/.ssh/config
            chmod 600 scripts/id_rsa
                    
            scp -i scripts/id_rsa projectGenerator-$PLATFORM.zip tests@198.61.170.130:projectGenerator_builds/projectGenerator-$PLATFORM_new.zip
            ssh -i scripts/id_rsa tests@198.61.170.130 "mv projectGenerator_builds/projectGenerator-$PLATFORM_new.zip projectGenerator_builds/projectGenerator-$PLATFORM.zip"
        fi
    fi
}

import_certificate(){
    
    echo "import_certificate"

    if [[ "${GITHUB_REF##*/}" == "master" && -z "${GITHUB_HEAD_REF}" && -n "${CERTIFICATE_OSX_APPLICATION}" ]]; then
        echo "Decoding signing certificates"
        
        KEY_CHAIN=build.keychain
        CERTIFICATE_P12=certificate.p12
        
        # Recreate the certificate from the secure environment variable
        echo $CERTIFICATE_OSX_APPLICATION | base64 --decode > $CERTIFICATE_P12

        #create a keychain
        security create-keychain -p actions $KEY_CHAIN

        # Make the keychain the default so identities are found
        security default-keychain -s $KEY_CHAIN
        
        # Unlock the keychain
        security unlock-keychain -p actions $KEY_CHAIN
        
        echo "Importing signing certificates"
        sudo security import $CERTIFICATE_P12 -k $KEY_CHAIN -P $CERTIFICATE_PASSWORD -T /usr/bin/codesign;

        security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k actions $KEY_CHAIN
        
        # makes keychain not timeout
        security set-keychain-settings $KEY_CHAIN
        
        echo "import certificates done"
    fi
        
}

import_certificate_travis(){
    echo "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}";
    if [ "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" = "openframeworks/projectGenerator/master" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then
        echo "Decoding signing certificates"
        cd ${pg_root}/scripts
        openssl aes-256-cbc -K $encrypted_b485a78f2982_key -iv $encrypted_b485a78f2982_iv -in developer_ID.p12.enc -out developer_ID.p12 -d
        echo "Creating keychain"
        security create-keychain -p mysecretpassword build.keychain
        security -v list-keychains -s build.keychain "$HOME/Library/Keychains/login.keychain"
        echo "Setting keychain as default"
        security default-keychain -s build.keychain
        echo "Unlocking keychain"
        security unlock-keychain -p mysecretpassword build.keychain
        security set-keychain-settings -t 3600 -u build.keychain
        echo "Importing signing certificates"
        sudo security import developer_ID.p12 -k build.keychain -P $CERT_PWD -T /usr/bin/codesign
        # security set-key-partition-list -S apple-tool:,apple: -s -k mysecretpassword build.keychain
        # security find-identity -v
    fi
}

cd ..
of_root=${PWD}/openFrameworks
pg_root=${PWD}/openFrameworks/apps/projectGenerator

if [ -d "openframeworks/.git" ]; then
    echo 'OF already cloned, using it'
    # cd openframeworks 
    # git pull
  # Control will enter here if $DIRECTORY exists.
else  
    git clone --depth=1 https://github.com/openframeworks/openFrameworks
fi
#cp not move so github actions can do cleanup without error
cp -r projectGenerator openFrameworks/apps/

cd ${of_root}
if [ -d "libs/glfw" ]; then
    echo 'libs installed, using them'
else
    scripts/osx/download_libs.sh
fi

# Compile commandline tool
cd ${pg_root}
echo "Building openFrameworks PG - OSX"
xcodebuild -configuration Release -target commandLine CODE_SIGN_IDENTITY="" UseModernBuildSystem=NO -project commandLine/commandLine.xcodeproj
ret=$?
if [ $ret -ne 0 ]; then
      echo "Failed building Project Generator"
      exit 1
fi

# install electron sign globally
sudo npm install -g electron-osx-sign

if [ -d "/Users/runner/" ]; then
    sudo chown -R 501:20 "/Users/runner/.npm"
fi    

import_certificate

# Generate electron app
cd ${pg_root}/frontend
npm update
npm install > /dev/null
npm run build:osx > /dev/null
mv dist/projectGenerator-darwin-x64 ${pg_root}/projectGenerator-osx
sign_and_upload osx

cd ${pg_root}/frontend
npm run build:osx > /dev/null
mv dist/projectGenerator-darwin-x64 ${pg_root}/projectGenerator-ios
sign_and_upload ios

cd ${pg_root}/frontend
npm run build:osx > /dev/null
mv dist/projectGenerator-darwin-x64 ${pg_root}/projectGenerator-android
sign_and_upload android

rm -rf scripts/id_rsa 2> /dev/null
rm -rf scripts/*.p12 2> /dev/null

