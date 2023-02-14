#!/bin/bash

#
# path and file name settings
#

# the build products are here
PROJECT_PATH="$HOME/git/ostc4"

# Debug or Release build
BUILD_TYPE="Debug"

# build project names
CPU1_DISCOVERY="Firmware"
CPU1_FONTPACK="FontPack"
CPU2_RTE="RTE"

PROJECT_NAME_PREFIX="OSTC4_"
#
# End of path and file name settings
#

#
# Copy the bin files to pack. Build them seperately
#

mkdir -p ./$BUILD_TYPE

BUILD_PATH=$PROJECT_PATH/RefPrj

cp $BUILD_PATH/$CPU1_DISCOVERY/$BUILD_TYPE/${PROJECT_NAME_PREFIX}$CPU1_DISCOVERY.bin ./$BUILD_TYPE
cp $BUILD_PATH/$CPU1_FONTPACK/$BUILD_TYPE/${PROJECT_NAME_PREFIX}$CPU1_FONTPACK.bin ./$BUILD_TYPE
cp $BUILD_PATH/$CPU2_RTE/$BUILD_TYPE/${PROJECT_NAME_PREFIX}$CPU2_RTE.bin ./$BUILD_TYPE

#
# OSTC4pack_V4 all
#

PACKAGE_TOOL_DIR=$PROJECT_PATH/ostc4pack/src

cd ./$BUILD_TYPE

$PACKAGE_TOOL_DIR/OSTC4pack_V4 1 ${PROJECT_NAME_PREFIX}$CPU1_DISCOVERY.bin
$PACKAGE_TOOL_DIR/OSTC4pack_V4 2 ${PROJECT_NAME_PREFIX}$CPU1_FONTPACK.bin
$PACKAGE_TOOL_DIR/OSTC4pack_V4 0 ${PROJECT_NAME_PREFIX}$CPU2_RTE.bin

#
# Final pack
#
$PACKAGE_TOOL_DIR/checksum_final_add_fletcher ${PROJECT_NAME_PREFIX}${CPU1_DISCOVERY}_upload.bin \
	${PROJECT_NAME_PREFIX}${CPU1_FONTPACK}_upload.bin \
	${PROJECT_NAME_PREFIX}${CPU2_RTE}_upload.bin
