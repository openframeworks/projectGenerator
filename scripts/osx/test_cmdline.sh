#!/usr/bin/env bash

# test_cmdline.sh
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
pwd
echo "Testing projectGenerator [osx]";
chmod +x projectGenerator

codesign --verify --deep --verbose=2 "./projectGenerator"
SIGN_STATUS=$?
if [ $SIGN_STATUS -ne 0 ]; then
    echo "Code signing is required. Signing the application..."
    codesign --sign "-" --deep --force --verbose --entitlements $PG_DIR/scripts/osx/PG.entitlements "./projectGenerator"
    echo "Verifying the new code signature..."
    codesign --verify --deep --verbose=2 "./projectGenerator"
else
    echo "Application is already code-signed and valid"
fi

echo "Test: projectGenerator building"
echo "Test 1: test examples templates - auto path to openFrameworks core"
./projectGenerator --recursive -posx ../../examples/templates
echo "Test 1: test examples templates - success"

echo "Test: projectGenerator building"
echo "Test 2: all examples - defined core openFrameworks path ../../"
./projectGenerator --recursive -posx -o../../ ../../examples/ ./projectGenerator
echo "Test 2: all examples - success"

# echo "test out of folder -o [vs]";
# rm -rf ../../../../../pg2
# mkdir -p  ../../../../../pg2
# if ! command -v rsync &> /dev/null
# then
#     cp -a ./projectGenerator ../../../../../pg2
# else
#     rsync -azp ./projectGenerator ../../../../../pg2
# fi
# cd ../../../../../pg2
# ls -a
# pwd
# ./projectGenerator --recursive -posx -o"./../openFrameworks" ./../openFrameworks/examples/
# errorcode=$?
# if [[ $errorcode -ne 0 ]]; then
#         exit $errorcode
# fi

# ./projectGenerator --recursive -posx -o"./../openFrameworks" ./../openFrameworks/examples/
# errorcode=$?
# if [[ $errorcode -ne 0 ]]; then
#         exit $errorcode
# fi

# echo "Test generate new just name"
# ./projectGenerator -o"../openFrameworks" -p"osx" "testingGenerate"
# errorcode=$?
# if [[ $errorcode -ne 0 ]]; then
#       exit $errorcode
# fi
# echo "Test generate new / update full path"
# ./projectGenerator -o"../openFrameworks" -p"osx" "../openFrameworks/apps/myApps/testingGenerate"
# errorcode=$?
# if [[ $errorcode -ne 0 ]]; then
#         exit $errorcode
# fi

# echo "Test generate full path"
# ./projectGenerator -o"../openFrameworks" -p"osx" "openFrameworks/apps/myApps/testingGenerate2"
# errorcode=$?
# if [[ $errorcode -ne 0 ]]; then
#         exit $errorcode
# fi


echo "Successful projectGenerator tests for [osx]";
