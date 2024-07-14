#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../" )" && pwd )"
FRONTEND_DIR="$( cd "$( dirname "${PG_DIR}/" )" && pwd )"

echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"
echo "FRONTEND_DIR:  ${FRONTEND_DIR}"
echo "======= to frontend dir ${FRONTEND_DIR}/frontend/dist/"

cd "${FRONTEND_DIR}/frontend/dist/"

if [ -d "win-unpacked" ]; then

    if [ -f "projectGenerator-vs-gui.zip" ]; then
        rm "projectGenerator-vs-gui.zip"
    fi

    cd "win-unpacked"

    _DIR=$(pwd)


    ${SCRIPT_DIR}/secure.sh ${_DIR}/projectGenerator.exe

    zip -r "../projectGenerator-vs-gui.zip" "."
    pwd
    ls
else
    echo "win-unpacked does not exist. please build first"
fi