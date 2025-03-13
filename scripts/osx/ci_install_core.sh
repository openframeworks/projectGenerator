#!/bin/bash
set -e


# Script for CI / BOTS

ORIGINAL_DIR=$(pwd)
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"

OF_DIR="$( cd "$( dirname "${PG_DIR}/../../" )" && pwd )"

FRONTEND_DIR="$( cd "$( dirname "${PG_DIR}/frontend" )" && pwd )"

CMDLINE_DIR="$( cd "$( dirname "${PG_DIR}/commandLine" )" && pwd )"

cd "$CURRENT_DIR"

ls -ld $(pwd)

echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"
echo "FRONTEND_DIR:  ${FRONTEND_DIR}"
echo "CMD_DIR:  ${CMDLINE_DIR}"


BASE_DIR="${OF_DIR}"
echo "BASE_DIR: ${BASE_DIR}"

# Define OF_ROOT relative to BASE_DIR.
OF_ROOT="${BASE_DIR}/openFrameworks"
echo "====== OF_DIR: ${OF_ROOT}"

# List current directory to verify permissions.
echo "Current directory: $(pwd)"
ls -ld "$(pwd)"

# Now, clone if not already present.
if [ -d "${OF_ROOT}/.git" ]; then
    echo 'OF already cloned, using it'
    cd "${OF_ROOT}"
    git pull
    git submodule init
    git submodule update --recursive
    cd "${BASE_DIR}"
else
    echo "cloning openFrameworks"
    git clone --depth=1 https://github.com/openframeworks/openFrameworks "${OF_ROOT}"
fi
	
cd ${OF_ROOT}

echo "Current directory:"
pwd
echo "Directory ../ contents:"
ls

if [ -d "libs/glfw" ]; then
	echo 'libs installed, using them'
else
	echo 'downloading libs'
	scripts/osx/download_libs.sh
fi


echo "ci install complete ---"

echo "Current directory:  $(pwd) ---"
echo "Directory ../ contents:  $(pwd) ---"
echo "------------------"

cd ${OF_ROOT}
echo "Current directory: $(pwd)"
echo "Directory contents:"
ls

echo "------------------"
echo "Intended destination for projectGenerator files:"
echo "${OF_ROOT}/apps/projectGenerator"

if command -v rsync &> /dev/null; then
    rsync -avzp --exclude='.git/' --exclude='.ccache/' ${PG_DIR}/ ${OF_ROOT}/apps/projectGenerator/
else
    cp -X ${PG_DIR}/ ${OF_ROOT}/apps/projectGenerator/ 2> /dev/null
fi

ls apps/projectGenerator



cd "$ORIGINAL_DIR"

