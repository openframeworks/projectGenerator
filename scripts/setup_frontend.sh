#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}" )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../" )" && pwd )"

FRONTEND_DIR="${PG_DIR}/frontend"

echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"
echo "FRONTEND_DIR:  ${FRONTEND_DIR}"

echo "====== "
cd "${FRONTEND_DIR}"
pwd 
echo "Setup PG openFrameworks Frontend Systems"
npm install
echo "====== install"
npm update
echo "====== update"
npm run
echo "PG openFrameworks Frontend Systems setup"
