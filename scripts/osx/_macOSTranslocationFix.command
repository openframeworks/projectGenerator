#!/bin/sh
cd "$( dirname "${BASH_SOURCE[0]}" )"
xattr -r -d com.apple.quarantine projectGenerator.app
