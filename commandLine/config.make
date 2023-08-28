################################################################################
# CONFIGURE PROJECT MAKEFILE (optional)
#   This file is where we make project specific configurations.
################################################################################

################################################################################
# OF ROOT
#   The location of your root openFrameworks installation
#       (default) OF_ROOT = ../../.. 
################################################################################
OF_ROOT = ../../..

################################################################################
# PROJECT ROOT
#   The location of the project - a starting place for searching for files
#       (default) PROJECT_ROOT = . (this directory)
#    
################################################################################
# PROJECT_ROOT = .

################################################################################
# PROJECT SPECIFIC CHECKS
#   This is a project defined section to create internal makefile flags to 
#   conditionally enable or disable the addition of various features within 
#   this makefile.  For instance, if you want to make changes based on whether
#   GTK is installed, one might test that here and create a variable to check. 
################################################################################
# None
APPNAME = projectGenerator
#PROJECT_AFTER_OSX = cp "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME" "$TARGET_BUILD_DIR/commandLinePG"; rm -rf "$TARGET_BUILD_DIR/$PRODUCT_NAME.app";	mv "$TARGET_BUILD_DIR/commandLinePG" "$TARGET_BUILD_DIR/projectGenerator"
PROJECT_AFTER_OSX = if test -f "$TARGET_BUILD_DIR/projectGenerator"; then rm "$TARGET_BUILD_DIR/projectGenerator"; fi;  cp "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME" "$TARGET_BUILD_DIR/projectGenerator"

################################################################################
# PROJECT EXTERNAL SOURCE PATHS
#   These are fully qualified paths that are not within the PROJECT_ROOT folder.
#   Like source folders in the PROJECT_ROOT, these paths are subject to 
#   exlclusion via the PROJECT_EXLCUSIONS list.
#
#     (default) PROJECT_EXTERNAL_SOURCE_PATHS = (blank) 
#
#   Note: Leave a leading space when adding list items with the += operator
################################################################################
# PROJECT_EXTERNAL_SOURCE_PATHS = 

################################################################################
# PROJECT EXCLUSIONS
#   These makefiles assume that all folders in your current project directory 
#   and any listed in the PROJECT_EXTERNAL_SOURCH_PATHS are are valid locations
#   to look for source code. The any folders or files that match any of the 
#   items in the PROJECT_EXCLUSIONS list below will be ignored.
#
#   Each item in the PROJECT_EXCLUSIONS list will be treated as a complete 
#   string unless teh user adds a wildcard (%) operator to match subdirectories.
#   GNU make only allows one wildcard for matching.  The second wildcard (%) is
#   treated literally.
#
#      (default) PROJECT_EXCLUSIONS = (blank)
#
#		Will automatically exclude the following:
#
#			$(PROJECT_ROOT)/bin%
#			$(PROJECT_ROOT)/obj%
#			$(PROJECT_ROOT)/%.xcodeproj
#
#   Note: Leave a leading space when adding list items with the += operator
################################################################################
# PROJECT_EXCLUSIONS =

################################################################################
# PROJECT LINKER FLAGS
#	These flags will be sent to the linker when compiling the executable.
#
#		(default) PROJECT_LDFLAGS = -Wl,-rpath=./libs
#
#   Note: Leave a leading space when adding list items with the += operator
################################################################################

# Currently, shared libraries that are needed are copied to the 
# $(PROJECT_ROOT)/bin/libs directory.  The following LDFLAGS tell the linker to
# add a runtime path to search for those shared libraries, since they aren't 
# incorporated directly into the final executable application binary.
# TODO: should this be a default setting?
# PROJECT_LDFLAGS=-Wl,-rpath=./libs

################################################################################
# PROJECT DEFINES
#   Create a space-delimited list of DEFINES. The list will be converted into 
#   CFLAGS with the "-D" flag later in the makefile.
#
#		(default) PROJECT_DEFINES = (blank)
#
#   Note: Leave a leading space when adding list items with the += operator
################################################################################
# PROJECT_DEFINES = 

################################################################################
# PROJECT CFLAGS
#   This is a list of fully qualified CFLAGS required when compiling for this 
#   project.  These CFLAGS will be used IN ADDITION TO the PLATFORM_CFLAGS 
#   defined in your platform specific core configuration files. These flags are
#   presented to the compiler BEFORE the PROJECT_OPTIMIZATION_CFLAGS below. 
#
#		(default) PROJECT_CFLAGS = (blank)
#
#   Note: Before adding PROJECT_CFLAGS, note that the PLATFORM_CFLAGS defined in 
#   your platform specific configuration file will be applied by default and 
#   further flags here may not be needed.
#
#   Note: Leave a leading space when adding list items with the += operator
################################################################################
# PROJECT_CFLAGS = 

################################################################################
# PROJECT OPTIMIZATION CFLAGS
#   These are lists of CFLAGS that are target-specific.  While any flags could 
#   be conditionally added, they are usually limited to optimization flags. 
#   These flags are added BEFORE the PROJECT_CFLAGS.
#
#   PROJECT_OPTIMIZATION_CFLAGS_RELEASE flags are only applied to RELEASE targets.
#
#		(default) PROJECT_OPTIMIZATION_CFLAGS_RELEASE = (blank)
#
#   PROJECT_OPTIMIZATION_CFLAGS_DEBUG flags are only applied to DEBUG targets.
#
#		(default) PROJECT_OPTIMIZATION_CFLAGS_DEBUG = (blank)
#
#   Note: Before adding PROJECT_OPTIMIZATION_CFLAGS, please note that the 
#   PLATFORM_OPTIMIZATION_CFLAGS defined in your platform specific configuration 
#   file will be applied by default and further optimization flags here may not 
#   be needed.
#
#   Note: Leave a leading space when adding list items with the += operator
################################################################################
PROJECT_OPTIMIZATION_CFLAGS_RELEASE = -O3
# PROJECT_OPTIMIZATION_CFLAGS_DEBUG = 

################################################################################
# PROJECT COMPILERS
#   Custom compilers can be set for CC and CXX
#		(default) PROJECT_CXX = (blank)
#		(default) PROJECT_CC = (blank)
#   Note: Leave a leading space when adding list items with the += operator
################################################################################
# PROJECT_CXX = 
# PROJECT_CC = 


#PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/utils
PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/sound
PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/3d
PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/communication
PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/events
PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/gl
PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/graphics
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/math
PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/types
PLATFORM_CORE_EXCLUSIONS += ../../../libs/openFrameworks/video

PLATFORM_CORE_EXCLUSIONS += ../../../libs/boost%
PLATFORM_CORE_EXCLUSIONS += ../../../libs/cairo%
PLATFORM_CORE_EXCLUSIONS += ../../../libs/rtAudio%
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/glm%

# unable to remove because ofURLFileLoaderImpl is being used by ofImage, which is being used by ofUtils
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/curl%

#PLATFORM_CORE_EXCLUSIONS += ../../../libs/fmod%
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/freetype%
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/FreeImage%

# ../../../libs/openFrameworks/utils/ofConstants.h:192:11: fatal error: 'GL/glew.h' file not found
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/glew%

# not possible to remove yet
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/glfw%

# ../../../libs/openFrameworks/utils/ofConstants.h:287:10: fatal error: 'tesselator.h' file not found
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/tess2%

# inside ofImage inside ofUtils
#PLATFORM_CORE_EXCLUSIONS += ../../../libs/uriparser%

