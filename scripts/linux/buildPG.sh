#!/bin/bash
set -e
cd ..
git clone --depth=1 https://github.com/openframeworks/openFrameworks
mv projectGenerator openFrameworks/apps/

cd openFrameworks

sudo ./scripts/linux/ubuntu/install_dependencies.sh -y;
scripts/linux/download_libs.sh

export LC_ALL=C
OF_ROOT=./

cd ${OF_ROOT}/apps/projectGenerator
make Release -C ./commandLine
ret=$?
if [ $ret -ne 0 ]; then
      echo "Failed building Project Generator"
      exit 1
fi

echo "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}";
if [ "${TRAVIS_REPO_SLUG}/${TRAVIS_BRANCH}" = "openframeworks/projectGenerator/master" ] && [ "${TRAVIS_PULL_REQUEST}" = "false" ]; then
    openssl aes-256-cbc -K $encrypted_cd38768cbb9d_key -iv $encrypted_cd38768cbb9d_iv -in scripts/id_rsa.enc -out scripts/id_rsa -d
    cp scripts/ssh_config ~/.ssh/config
    chmod 600 scripts/id_rsa
    scp -i scripts/id_rsa commandLine/bin/projectGenerator tests@198.61.170.130:projectGenerator_builds/projectGenerator_linux_new
    ssh -i scripts/id_rsa tests@198.61.170.130 "mv projectGenerator_builds/projectGenerator_linux_new projectGenerator_builds/projectGenerator_linux"
fi
rm -rf scripts/id_rsa
