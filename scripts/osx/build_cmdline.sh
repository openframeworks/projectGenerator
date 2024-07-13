#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"
CMDLINE_DIR="$( cd "$( dirname "${PG_DIR}/commandLine" )" && pwd )"


if [ -z "${IDENTITY+x}" ]; then
   IDENTITY="-"
fi

echo "====== "
# Compile commandline tool
cd ${CMDLINE_DIR}
echo "Building openFrameworks PG - OSX"
xcodebuild -configuration Release -target commandLine CODE_SIGN_IDENTITY="$IDENTITY" UseModernBuildSystem=NO -project commandLine/commandLine.xcodeproj
ret=$?
if [ $ret -ne 0 ]; then
	  echo "Failed building Project Generator"
	  exit 1
fi

codesign --verify --deep --verbose=2 "commandLine/bin/projectGenerator"
SIGN_STATUS=$?
if [ $SIGN_STATUS -ne 0 ]; then
    echo "Code signing is required. Signing the application..."
    codesign --sign "$IDENTITY" --deep --force --verbose --entitlements $PG_DIR/scripts/osx/PG.entitlements "commandLine/bin/projectGenerator"
    echo "Verifying the new code signature..."
    codesign --verify --deep --verbose=2 "commandLine/bin/projectGenerator"
else
    echo "Application is already code-signed and valid"
fi

echo "Successfully built commandLine openFrameworks"

