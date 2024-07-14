#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}"/../../ )" && pwd )"

CMD_DIR="${PG_DIR}/commandLine"

echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"
echo "CMD_DIR:  ${CMD_DIR}"
echo "====== ${CMD_DIR}"
# Compile commandline tool
cd "${CMD_DIR}/bin"
echo "Testing projectGenerator: [vs]";
chmod +x projectGenerator
./projectGenerator --recursive -pvs -o../../../../ ../../../../examples/
errorcode=$?
if [[ $errorcode -ne 0 ]]; then
		exit $errorcode
fi

echo "test out of folder -o [vs]";
if ! command -v rsync &> /dev/null
then      
    cp -a ./projectGenerator ../../../../../  
else
    rsync -azp ./projectGenerator ../../../../../  
fi
cd ../../../../../ 
pwd
./projectGenerator --recursive -pvs -o"./openFrameworks" ./openFrameworks/examples/
errorcode=$?
if [[ $errorcode -ne 0 ]]; then
		exit $errorcode
fi
echo "Successful projectGenerator tests for [vs]";