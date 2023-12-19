#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

#install nodejs
brew install node
if [ -d "/Users/runner/" ]; then
	# install electron sign globally
	sudo npm install -g electron-osx-sign
	sudo chown -R 501:20 "/Users/runner/.npm"
fi
