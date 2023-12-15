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

if [ -z "${BUILD_TEST+x}" ]; then
   BUILD_TEST=0
fi

echo "====== "
cd "${PG_DIR}/frontend"
pwd 
echo "Building openFrameworks Frontend ${FRONTEND_TARGET}"
npm install
echo "====== install"
npm update > /dev/null
echo "====== update"
npm run > /dev/null
echo "====== run"

if [ "${BUILD_TEST}" == 1 ]; then
   npm run start:prod
   echo "====== start:prod"
fi

npm run dist:${FRONTEND_TARGET}
echo "====== dist:${FRONTEND_TARGET}"

