OF_ROOT=$PWD

SCRIPT_DIR="${BASH_SOURCE%/*}"

cd ..
of_root=${PWD}/openFrameworks
pg_root=${PWD}/openFrameworks/apps/projectGenerator
git clone https://github.com/openframeworks/openFrameworks --depth=1
cp -r projectGenerator openFrameworks/apps/
cd openFrameworks
scripts/vs/download_libs.sh -p vs --silent

