#!/bin/bash

echo "copying the executable out of the app"

cp bin/commandLine.app/Contents/MacOS/commandLine bin/commandLineExe

echo "deleting the app"

rm -rf bin/commandLine.app

echo "renaming the executable"

mv bin/commandLineExe bin/commandLine

