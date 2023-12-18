#!/bin/bash
set -e

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"

OF_DIR="$( cd "$( dirname "${PG_DIR}/../../../" )" && pwd )"

FRONTEND_DIR="$( cd "$( dirname "${PG_DIR}/frontend" )" && pwd )"

CMDLINE_DIR="$( cd "$( dirname "${PG_DIR}/commandLine" )" && pwd )"


echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"
echo "FRONTEND_DIR:  ${FRONTEND_DIR}"
echo "CMD_DIR:  ${CMD_DIR}"
echo "====== OF_DIR: ${OF_DIR}"

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

package_app(){
	if [[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" ]] ; then

		PLATFORM=$1
		# Copy commandLine into electron .app
		cd ${PG_DIR}
		echo "Current directory: (should be projectGenerator)"
		pwd
		echo "Directory contents:"
		ls
		echo "-------------"
		#echo "copy cmdline PG to "
		cp commandLine/bin/projectGenerator projectGenerator-$PLATFORM/projectGenerator.app/Contents/Resources/app/app/projectGenerator 2> /dev/null
		cd ${PG_DIR}
		pwd
		echo "Directory contents:"
		ls
		echo "-------------"
		sed -i -e "s/osx/$PLATFORM/g" projectGenerator-$PLATFORM/projectGenerator.app/Contents/Resources/app/settings.json

		# Sign app
		echo "Signing electron .app"
		echo "cd to ${PG_DIR}/projectGenerator-$PLATFORM"
		cd ${PG_DIR}/projectGenerator-$PLATFORM
		pwd
		echo "Directory contents:"
		ls
		echo "-------------"
		xattr -cr projectGenerator.app
		echo "-------------"
		echo "cd to ${PG_DIR}"
		cd ${PG_DIR}
		electron-osx-sign projectGenerator-$PLATFORM/projectGenerator.app --platform=darwin --type=distribution --no-gatekeeper-assess --hardened-runtime --entitlements=scripts/osx/PG.entitlements --entitlements-inherit=scripts/osx/PG.entitlements

		${SCRIPT_DIR}/secure.sh projectGenerator-$PLATFORM/projectGenerator.app/Contents/MacOS/projectGenerator projectGenerator-$PLATFORM
		echo "Compressing PG app"
		# need to upload zip of just app to apple for notarizing
		zip --symlinks -r -q projectGenerator-$PLATFORM/projectGenerator-$PLATFORM.zip projectGenerator-$PLATFORM/projectGenerator.app
		xcrun altool --notarize-app --primary-bundle-id "com.electron.projectgenerator" --username "${GA_APPLE_USERNAME}" -p "${GA_APPLE_PASS}" --asc-provider "${GA_NOTARIZE_PROVIDER}" --file projectGenerator-$PLATFORM/projectGenerator-$PLATFORM.zip
		mv projectGenerator-$PLATFORM/projectGenerator-$PLATFORM.zip ${PG_DIR}/../../../projectGenerator/projectGenerator-$PLATFORM.zip
		cd ${PG_DIR}/../../../projectGenerator
		echo "Final Directory contents: ${PG_DIR}/../../../projectGenerator"
		pwd
		ls
		echo "-------------"
	else
		echo "package_app not on master/bleeding so will not package app"	
	fi
}


sign_and_upload(){
	PLATFORM=$1
	# Copy commandLine into electron .app
	cd ${PG_DIR}
	cp commandLine/bin/projectGenerator projectGenerator-$PLATFORM/projectGenerator.app/Contents/Resources/app/app/projectGenerator 2> /dev/null

	sed -i -e "s/osx/$PLATFORM/g" projectGenerator-$PLATFORM/projectGenerator.app/Contents/Resources/app/settings.json

	if [[ -z "${GA_CI_SECRET}" ]] ; then
		echo " Not on main repo skipping sign and upload ";
	else
		if [[ ("${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/master" || "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/bleeding") && "$TRAVIS_PULL_REQUEST" == "false" ]] || 
   			[[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" ]] ; then
    		# Sign app
			echo "Signing electron .app"
			cd ${PG_DIR}
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

	if [[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" && -n "${CERTIFICATE_OSX_APPLICATION}" ]]; then
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

echo "build_cmdline.sh"
# Compile commandline tool
${CURRENT_DIR}/build_cmdline.sh

echo "test_cmdline.sh"
# Test commandline tool
${CURRENT_DIR}/test_cmdline.sh


echo "import_certificate"
import_certificate
# Generate electron app

echo "build_frontend"
${CURRENT_DIR}/build_frontend.sh

echo "Current directory: (should be projectGenerator)"
pwd
echo "Directory contents:"
ls

if [ -d "${PG_DIR}/projectGenerator-osx" ]; then
	rm -rf ${PG_DIR}/projectGenerator-osx
fi

cd ${PG_DIR}
echo "Current directory: (should be apps/projectGenerator)"
pwd
echo "Directory contents:"
ls
echo "-------------"
mv frontend/dist/mac-universal ${PG_DIR}/projectGenerator-osx

echo "-------------"
echo "package_app osx"
package_app osx

# echo "package_app ios"
# npm run build:macos > /dev/null

# if [ -d "${PG_DIR}/projectGenerator-ios" ]; then
# 	rm -rf ${PG_DIR}/projectGenerator-ios
# fi
# mv dist/mac ${PG_DIR}/projectGenerator-ios
# echo "package_app ios"
# package_app ios

echo "for security remove keys maybe imported"
rm -rf scripts/id_rsa 2> /dev/null
rm -rf scripts/*.p12 2> /dev/null

