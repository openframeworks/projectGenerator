#!/bin/bash
set -e


# Script for CI / BOTS 

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
echo "CMD_DIR:  ${CMDLINE_DIR}"

echo "SCRIPT dir:"
pwd

cd ../

OF_ROOT=${PWD}/openFrameworks

echo "====== OF_DIR: ${OF_ROOT}"

echo "Current directory:"
pwd
echo "Directory contents:"
ls


pg_root=${PWD}/openFrameworks/apps/projectGenerator
pwd
echo "ci setup - ${PWD}"
ls
if [ -d "${OF_ROOT}/.git" ]; then
	echo 'OF already cloned, using it'
	cd ${OF_ROOT}
	git pull
	git submodule init
	git submodule update --recursive
	cd ..
else
	echo "cloning of"
	git clone --depth=1 https://github.com/openframeworks/openFrameworks
	pwd 
	ls
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

echo "Current directory:  ---"
pwd

echo "Directory ../ contents:  ---"
cd ../
pwd
ls
echo "------------------"

echo "copying pg to oF dir"
pwd
ls
echo "------------------"
mkdir -p openFrameworks/apps/projectGenerator

rsync -av --exclude='.git/' projectGenerator/ openFrameworks/apps/projectGenerator/

echo "------------------"




