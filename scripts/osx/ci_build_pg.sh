#!/bin/bash
# set -e

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}" )" && pwd )"
export PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}" )" && pwd )"
export OF_DIR="$( cd "$( dirname "${PG_DIR}/../../../" )" && pwd )"
export FRONTEND_DIR="${PG_DIR}/frontend"
export CMDLINE_DIR="${PG_DIR}/commandLine"

echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"
echo "FRONTEND_DIR:  ${FRONTEND_DIR}"
echo "CMDLINE_DIR:  ${CMDLINE_DIR}"
echo "====== OF_DIR: ${OF_DIR}"

echoDots(){
	sleep 0.1 # Waiting for a brief period first, allowing jobs returning immediately to finish
	while isRunning $1; do
		for i in $(seq 1 10); do
			echo -ne .
			if ! isRunning $1; then
				printf "\r"
				return
			fi
			sleep 1
		done
		printf "\r                    "
		printf "\r"
	done
}

package_app(){
	PLATFORM=$1
	echo "package_app -- [${PLATFORM}${POSTFIX}]"
	
	cd "${PG_DIR}"
	echo "Current directory: (should be projectGenerator) [${PG_DIR}]"
	pwd
	echo "Directory contents:"
	ls
	echo "-------------"
	echo "copy cmdline PG from app contents (signed)"
	
	if command -v rsync &> /dev/null; then
		rsync -avzp "${CMDLINE_DIR}/bin/commandLine.app/Contents/MacOS/commandLine" "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator.app/Contents/Resources/app/app/projectGenerator" 2> /dev/null
	else
		cp -aX "${CMDLINE_DIR}/bin/commandLine.app/Contents/MacOS/commandLine" "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator.app/Contents/Resources/app/app/projectGenerator" 2> /dev/null
	fi

	cd "${PG_DIR}"
	pwd
	echo "Directory contents:"
	ls
	echo "-------------"

	echo "[Signing electron] .app"
	echo "cd to [${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}]"
	cd "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}"
	pwd
	echo "Directory contents:"
	ls
	echo "-------------"
	xattr -cr projectGenerator.app || echo "Warning: xattr failed"

	echo "-------------"
	echo "cd to [${PG_DIR}]"
	cd "${PG_DIR}"
	pwd

	echo "[Secure] creating PKL files"
	cd "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator.app/Contents/MacOS/projectGenerator" || echo "Warning: Directory not found"

	. "${SCRIPT_DIR}/secure.sh"
	secure "projectGenerator" projectGenerator.pkl

	cd "${PG_DIR}"
	cd "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator.app/Contents/Resources/app/app/projectGenerator" || echo "Warning: Directory not found"
	pwd
	. "${SCRIPT_DIR}/secure.sh"
	secure "projectGenerator" projectGenerator.pkl
	cd "${PG_DIR}"

	TEAM_ID="HC25N2E7UT"
	APPLE_ID="theo@theowatson.com"

	if [[ ("${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/master" || "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/bleeding") && "$TRAVIS_PULL_REQUEST" == "false" ]] || 
		[[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" ]] ; then
		electron-osx-sign "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator.app" --platform=darwin --type=distribution --no-gatekeeper-assess --hardened-runtime --entitlements=${SCRIPT_DIR}/osx/PG.entitlements --entitlements-inherit=${SCRIPT_DIR}/osx/PG.entitlements
	fi
	
	echo "[Compressing] projectGenerator gui"
	zip --symlinks -r -q "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator-${PLATFORM}${POSTFIX}.zip" "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator.app"
	
	if [[ ("${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/master" || "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/bleeding") && "$TRAVIS_PULL_REQUEST" == "false" ]] || 
		[[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" ]] ; then
		echo "[Notarizing] projectGenerator gui [projectGenerator-${PLATFORM}${POSTFIX}]"
		xcrun notarytool submit "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator-${PLATFORM}${POSTFIX}.zip" --apple-id "${APPLE_ID}" --team-id "${TEAM_ID}" --password "${GA_APPLE_PASS}"
	fi
	
	cd "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}"
	pwd
	ls

	cd "${PG_DIR}"
	if [[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" ]] ; then
		mv -v "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}/projectGenerator-${PLATFORM}${POSTFIX}.zip" "${PG_DIR}/../../../projectGenerator/projectGenerator-${PLATFORM}${POSTFIX}.zip"
		cd ${PG_DIR}/../../../projectGenerator
		echo "Final Directory contents: [${PG_DIR}/../../../projectGenerator]"
		pwd
		ls
		echo "-------------"
	fi
}

sign_and_upload(){
	echo "  sign_and_upload --"
	PLATFORM=$1
	
	cd ${PG_DIR}
	cp commandLine/bin/projectGenerator projectGenerator-$PLATFORM/projectGenerator.app/Contents/Resources/app/app/projectGenerator 2> /dev/null

	if [[ -z "${GA_CI_SECRET}" ]] ; then
		echo " Not on main repo skipping sign and upload ";
	else
		if [[ ("${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/master" || "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" == "openframeworks/projectGenerator/bleeding") && "$TRAVIS_PULL_REQUEST" == "false" ]] || 
			[[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" ]] ; then
			echo "Signing electron .app"
			cd ${PG_DIR}
			xattr -cr projectGenerator-$PLATFORM/projectGenerator.app || echo "Warning: xattr failed"

			electron-osx-sign projectGenerator-$PLATFORM/projectGenerator.app --platform=darwin --type=distribution --no-gatekeeper-assess --hardened-runtime --entitlements=scripts/osx/PG.entitlements --entitlements-inherit=scripts/osx/PG.entitlements

			echo "Compressing PG app"
			zip --symlinks -r -q projectGenerator-$PLATFORM.zip projectGenerator-$PLATFORM

			zip --symlinks -r -q projectGenerator-$PLATFORM/projectGenerator.app.zip projectGenerator-$PLATFORM/projectGenerator.app
			TEAM_ID="HC25N2E7UT"
			xcrun notarytool submit "projectGenerator-${PLATFORM}/projectGenerator-${PLATFORM}.app.zip" --apple-id "${GA_APPLE_USERNAME}" --team-id "${TEAM_ID}" --password "${GA_APPLE_PASS}"
			
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
	echo "  [import_certificate]"

	if [[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" && -n "${CERTIFICATE_OSX_APPLICATION}" ]]; then
		echo "Decoding signing certificates"

		KEY_CHAIN=build.keychain
		CERTIFICATE_P12=certificate.p12

		echo $CERTIFICATE_OSX_APPLICATION | base64 --decode > $CERTIFICATE_P12

		security create-keychain -p actions $KEY_CHAIN

		security default-keychain -s $KEY_CHAIN

		security unlock-keychain -p actions $KEY_CHAIN

		echo "Importing signing certificates"
		sudo security import $CERTIFICATE_P12 -k $KEY_CHAIN -P $CERTIFICATE_PASSWORD -T /usr/bin/codesign;

		security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k actions $KEY_CHAIN

		security set-keychain-settings $KEY_CHAIN

		echo "[import certificates done]"
	fi
}

echo "##[group]build_cmdline.sh"
${CURRENT_DIR}/build_cmdline.sh
echo "##[endgroup]"

echo "##[group]import_certificate"
import_certificate
echo "##[endgroup]"

echo "##[group]build_frontend mac-universal"
${CURRENT_DIR}/build_frontend.sh
echo "##[endgroup]"

if [ -d "${PG_DIR}/projectGenerator-osx" ]; then
	rm -rf ${PG_DIR}/projectGenerator-osx
fi

if [ -d "${PG_DIR}/projectGenerator-osx_arm64" ]; then
	rm -rf ${PG_DIR}/projectGenerator-osx_arm64
fi

if [ -d "${PG_DIR}/projectGenerator-osx_64" ]; then
	rm -rf ${PG_DIR}/projectGenerator-osx_64
fi

cd "${PG_DIR}"
echo "Current directory: (should be apps/projectGenerator) after builds [ ${PG_DIR} ]"
pwd
echo "Directory contents:"
ls
echo "-------------"

export PLATFORM=osx

export POSTFIX=""
if [ -z "${PG_DIR}/projectGenerator-osx${POSTFIX}" ]; then
	mkdir -v "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}"
	mv -v "${FRONTEND_DIR}/dist/mac-universal" "${PG_DIR}/projectGenerator-${PLATFORM}${POSTFIX}"
fi
echo "-------------"
echo "  package_app osx universal"
package_app ${PLATFORM}
echo "-------------"

if [[ ("${GITHUB_REF##*/}" == "master" || "${GITHUB_REF##*/}" == "bleeding") && -z "${GITHUB_HEAD_REF}" && -n "${CERTIFICATE_OSX_APPLICATION}" ]]; then
	echo "  [security] remove imported keys."
	rm -rf scripts/id_rsa 2> /dev/null
	rm -rf scripts/*.p12 2> /dev/null
fi

echo "  [Completed]"
