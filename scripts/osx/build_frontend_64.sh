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
   export FRONTEND_TARGET=mac:x64
fi

echo "Building openFrameworks Frontend [${FRONTEND_TARGET}]"

${CURRENT_DIR}/build_frontend.sh
