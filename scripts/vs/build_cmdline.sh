#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"

OF_DIR="$( cd "$( dirname "${PG_DIR}/../../../" )" && pwd )"


echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"

echo "====== OF_DIR: ${OF_DIR}"

echo "====== "
# Compile commandline tool
cd ${PG_DIR}
cd commandLine

echo "Building openFrameworks commandLine PG - VS"

if [ -z "${PLATFORM+x}" ]; then
    PLATFORM=x64
fi

if [ -z "${TOOLSET+x}" ]; then
    TOOLSET=v142
fi
pwd
ls

msbuild "commandLine.vcxproj" /p:configuration=release /p:platform=${PLATFORM} /p:PlatformToolset=${TOOLSET}
ret=$?
if [ $ret -ne 0 ]; then
	  echo "Failed building Project Generator"
	  exit 1
fi

echo "Successfully built commandLine openFrameworks"

