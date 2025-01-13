#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"

OF_DIR="$( cd "$( dirname "${PG_DIR}/../../../" )" && pwd )"

echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"

echo "====== OF_DIR: ${OF_DIR}"
cd ${PG_DIR}
cd commandLine
echo "Building openFrameworks commandLine PG - VS"

if [ -z "${PLATFORM+x}" ]; then
    PLATFORM=x64
fi
if [ -z "${TOOLSET+x}" ]; then
    TOOLSET=v143
fi
if [ -z "${CONFIGURATION+x}" ]; then
    CONFIGURATION=${CONFIGURATION:-Release}
fi

CURRENT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if command -v msbuild &> /dev/null; then
    echo "msbuild is available, use it directly"
    msbuild "commandLine.vcxproj" //p:configuration=${CONFIGURATION} //p:platform=${PLATFORM} //p:PlatformToolset=${TOOLSET}
else
    echo "msbuild is not available, use the specified path"
    MSBUILD_PATH="C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe"
    MSSTRING="//p:configuration=${CONFIGURATION} //p:platform=${PLATFORM} //p:PlatformToolset=${TOOLSET}"
    "${MSBUILD_PATH}" commandLine.sln ${MSSTRING}
fi

ret=$?
if [ $ret -ne 0 ]; then
	  echo "Failed building Project Generator"
	  exit 1
fi

echo "Successfully built commandLine openFrameworks"

