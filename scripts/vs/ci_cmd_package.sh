#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"
CMDLINE_DIR="$( cd "$( dirname "${PG_DIR}/commandLine" )" && pwd )"

cd ${CMDLINE_DIR}/commandLine/bin/

echo "ci_cmd_package"
if [ -f "commandLine.exe" ]; then
    if [ -f "projectGenerator.exe" ]; then
        rm "projectGenerator.exe"
    fi

    if [ -f "commandLine.exe" ]; then
        echo "could not find commandLine.exe - build first via build_cmdline.sh"
        exit 1
    fi

    mv "commandLine.exe" "projectGenerator.exe"

    zip -r "projectGenerator-vs.zip" "projectGenerator.exe" "data"
else
    echo "commandLine.exe does not exist. please build first"
fi

