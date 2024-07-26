#!/usr/bin/env bash

CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

winget install --id OpenJS.NodeJS.LTS --accept-package-agreements --accept-source-agreements

