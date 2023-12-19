#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"
CMDLINE_DIR="$( cd "$( dirname "${PG_DIR}/commandLine" )" && pwd )"

echo "====== "
# Compile commandline tool
cd ${CMDLINE_DIR}
echo "Building openFrameworks PG - OSX"
xcodebuild -configuration Release -target commandLine CODE_SIGN_IDENTITY="" UseModernBuildSystem=NO -project commandLine/commandLine.xcodeproj
ret=$?
if [ $ret -ne 0 ]; then
	  echo "Failed building Project Generator"
	  exit 1
fi

echo "Successfully built commandLine openFrameworks"

