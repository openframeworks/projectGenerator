#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"
FRONTEND_DIR="$( cd "$( dirname "${PG_DIR}" )" && pwd )"

echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"
echo "FRONTEND_DIR:  ${FRONTEND_DIR}"

if [ -z "${FRONTEND_TARGET+x}" ]; then
    FRONTEND_TARGET=mac:universal
fi

cd "${PG_DIR}/frontend"

if [ -z "${BUILD_TEST+x}" ]; then
   BUILD_TEST=0
fi


if [ -z "${BUILD_TEST+x}" ]; then
   BUILD_TEST=0
fi

# Assuming CMDLINE_DIR is already set to the correct directory path
SOURCE_FILE="${PG_DIR}/commandLine/bin/projectGenerator"

# Replace [destination_path] with the actual path where you want to copy the file
DESTINATION_PATH="app/"
echo "SOURCE_FILE:$SOURCE_FILE";
# Check if the source file exists
if [ -f "$SOURCE_FILE" ]; then
    # File exists, proceed with copying
    mkdir -p "$DESTINATION_PATH" # Create destination directory if it doesn't exist
    cp "$SOURCE_FILE" "$DESTINATION_PATH"
    echo "File copied successfully."
else
    # File does not exist
    echo "Error: Source file does not exist."
fi


echo "====== "
cd "${PG_DIR}/frontend"
pwd 
echo "Building openFrameworks Frontend ${FRONTEND_TARGET}"
npm install
echo "====== install"
npm update 
echo "====== update"
npm run
echo "====== run"

if [ "${BUILD_TEST}" == 1 ]; then
   npm run start:prod
   echo "====== start:prod"
fi

npm run dist:${FRONTEND_TARGET}
echo "====== dist:${FRONTEND_TARGET}"

