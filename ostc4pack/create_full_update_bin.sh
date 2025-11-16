#!/bin/bash

#
# path and file name settings
#

# the build products are here
PROJECT_PATH=${BUILD_DIR:-$HOME/git/ostc4}

# Debug or Release build
BUILD_TYPE="Release"

# build project names
CPU1_DISCOVERY="Firmware"
CPU1_FONTPACK="FontPack"
CPU2_RTE="RTE"
CPU1_BOOTLOADER="BootLoader"

PROJECT_NAME_PREFIX="OSTC4_"
#
# End of path and file name settings
#

while test $# -gt 0; do
    case "$1" in
    --no-fonts)
        NO_FONTS=1
        shift

        ;;
    --no-rte)
        NO_RTE=1
        shift

        ;;
    --do-bootloader)
        DO_BOOTLOADER=1
        shift

        ;;
    --no-date)
        NO_DATE=1
        shift

        ;;
    --version)
        VERSION=1
        shift

        ;;
    --print-version-only)
        PRINT_VERSION_ONLY=1
        shift

        ;;
    *)
        echo "Invalid parameter. Usage: create_full_update_bin.sh [--no-fonts] [--no-rte] [--no-date]"
        exit 1

        ;;
  esac
done

BUILD_PATH=$PROJECT_PATH/RefPrj
PACKAGE_TOOL_DIR=$PROJECT_PATH/ostc4pack/src

CHECKSUM_COMMAND_PARAMETERS="--type"
if [ -z "${NO_DATE:+x}" ]; then
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} --date"
fi
if [ -n "${VERSION:+x}" ]; then
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} --version"
fi

if [ -n "${PRINT_VERSION_ONLY:+x}" ]; then
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} --print-version-only"

    $PACKAGE_TOOL_DIR/checksum_final_add_fletcher $CHECKSUM_COMMAND_PARAMETERS foo

    exit 0
fi

#
# Copy the bin files to pack and OSTC4pack_V4
#

mkdir -p ./$BUILD_TYPE
cd ./$BUILD_TYPE

if [ -z "${DO_BOOTLOADER:+x}" ]; then
    pushd $BUILD_PATH/$CPU1_DISCOVERY/$BUILD_TYPE/
    $PACKAGE_TOOL_DIR/OSTC4pack_V4 1 ${PROJECT_NAME_PREFIX}${CPU1_DISCOVERY}.bin
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} $(pwd)/${PROJECT_NAME_PREFIX}${CPU1_DISCOVERY}_upload.bin"
    popd
else
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} null"
fi

if [ -z "${NO_FONTS:+x}" ]; then
    pushd $BUILD_PATH/$CPU1_FONTPACK/$BUILD_TYPE/
    $PACKAGE_TOOL_DIR/OSTC4pack_V4 2 ${PROJECT_NAME_PREFIX}${CPU1_FONTPACK}.bin
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} $(pwd)/${PROJECT_NAME_PREFIX}${CPU1_FONTPACK}_upload.bin"
    popd
elif [ -n "${DO_BOOTLOADER:+x}" ]; then
    pushd $BUILD_PATH/$CPU1_BOOTLOADER/$BUILD_TYPE/
    $PACKAGE_TOOL_DIR/OSTC4pack_V4 2 ${PROJECT_NAME_PREFIX}${CPU1_BOOTLOADER}.bin
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} $(pwd)/${PROJECT_NAME_PREFIX}${CPU1_BOOTLOADER}_upload.bin"
    popd
else
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} null"
fi

if [ -z "${NO_RTE:+x}" ]; then
    pushd $BUILD_PATH/$CPU2_RTE/$BUILD_TYPE/
    $PACKAGE_TOOL_DIR/OSTC4pack_V4 0 ${PROJECT_NAME_PREFIX}${CPU2_RTE}.bin
    CHECKSUM_COMMAND_PARAMETERS="${CHECKSUM_COMMAND_PARAMETERS} $(pwd)/${PROJECT_NAME_PREFIX}${CPU2_RTE}_upload.bin"
    popd
fi


#
# Final pack
#
$PACKAGE_TOOL_DIR/checksum_final_add_fletcher $CHECKSUM_COMMAND_PARAMETERS
