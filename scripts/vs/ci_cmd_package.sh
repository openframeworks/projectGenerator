#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"
CMDLINE_DIR="$( cd "$( dirname "${PG_DIR}" )" && pwd )"

cd ${PG_DIR}/commandLine/bin/

echo "ci_cmd_package"
pwd
ls
if [ -f "commandLine.exe" ]; then
    if [ -f "projectGenerator.exe" ]; then
        rm "projectGenerator.exe"
    fi

    mv "commandLine.exe" "projectGenerator.exe"

    zip -r "projectGenerator-vs.zip" "projectGenerator.exe" "data"
else
    echo "commandLine.exe does not exist. please build first"
fi

cd ${PG_DIR}

SOURCE_FILE="${PG_DIR}/commandLine/bin/projectGenerator.exe"
FRONTEND_DIR="$( cd "$( dirname "${PG_DIR}/frontend" )" && pwd )"
cd "${PG_DIR}/frontend"

DESTINATION_PATH="app/"
echo "SOURCE_FILE:$SOURCE_FILE";
# Check if the source file exists
if [ -f "$SOURCE_FILE" ]; then
    # File exists, proceed with copying
    mkdir -p "$DESTINATION_PATH" # Create destination directory if it doesn't exist
    cp "$SOURCE_FILE" "$DESTINATION_PATH/projectGenerator.exe"
    echo "projectGenerator.exe File copied successfully."
else
    # File does not exist
    echo " projectGenerator.exe Error: Source file does not exist."
fi