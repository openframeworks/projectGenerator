#!/bin/bash

/usr/bin/osascript <<EOD

    tell application "System Events" 
        activate
        set p to "$1"
	set thePath to POSIX file p as string
	set the_folder to choose folder with prompt ("Select a folder:")  default location thePath as alias
	set the_folder_posix to POSIX path of the_folder 
    end tell
    
    return the_folder_posix as Unicode text
    
EOD
