#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_DIR="$( cd "$( dirname "${CURRENT_DIR}"/../../ )" && pwd )"
PG_DIR="$( cd "$( dirname "${SCRIPT_DIR}/../../" )" && pwd )"
FRONTEND_DIR="$( cd "$( dirname "${PG_DIR}" )" && pwd )"

echo "CURRENT_DIR:  ${CURRENT_DIR}"
echo "SCRIPT_DIR:  ${SCRIPT_DIR}"
echo "PG_DIR:  ${PG_DIR}"
echo "FRONTEND_DIR:  ${FRONTEND_DIR}"

echo "====== "
cd "${PG_DIR}/frontend"
pwd 
echo "Building openFrameworks Frontend"
npm install
echo "====== "
npm update
echo "====== "
npm run
echo "====== "
npm run build:macos-m1

check_bitcode() {
  local binary="$1"
  # Check if the binary contains the __LLVM segment, which indicates bitcode
   local escaped_binary=$(printf "%q" "$binary")
  # Use the escaped filename with otool
  local has_bitcode=$(otool -l $escaped_binary | grep __LLVM)
  
  if [ -n "$has_bitcode" ]; then
    echo "Bitcode found in $binary"
    xcrun bitcode_strip -r $binary -o $binary
  else
    echo "No bitcode in $binary"
  fi
}

# The root directory where the build is done (adjust this to your build directory)
ROOT_DIR="."

# Find all executable files and dynamic libraries in the directory
find "$ROOT_DIR/dist" -type f \( -name '*.app' -or -name '*.framework' -or -name '*.dylib' -or -name '*' \) -print0 | while IFS= read -r -d $'\0' binary; do
  # Call the check_bitcode function on each binary
  check_bitcode "$binary"
done
# mv commandLine.exe projectGenerator.exe
# zip -r projectGenerator-vs.zip projectGenerator.exe data


