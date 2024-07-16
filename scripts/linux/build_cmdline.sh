#!/bin/bash
set -e

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
echo "Building Project Generator"

export LC_ALL=C
cd ${OF_DIR}/apps/projectGenerator

# current makefile
make -j2 Release -C ./commandLine

ret=$?
if [ $ret -ne 0 ]; then
      echo "Failed building Project Generator"
      exit 1
fi
echo "Successfly built projectGenerator for [linux64]";
