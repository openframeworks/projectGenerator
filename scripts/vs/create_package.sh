OF_ROOT=$PWD
SCRIPT_DIR="${BASH_SOURCE%/*}"

cd ../openFrameworks/apps/projectGenerator/commandLine/bin/
mv commandLine.exe projectGenerator.exe
zip -r projectGenerator-vs.zip projectGenerator.exe data


