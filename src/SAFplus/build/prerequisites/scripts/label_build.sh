#!/bin/bash
# Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
# This file is available  under  a  commercial  license  from  the
# copyright  holder or the GNU General Public License Version 2.0.
#
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# For more  information,  see the  file COPYING provided with this
# material.

#
# if CROSS_BUILD is empty, set it to 'local'
if [ ! $CROSS_BUILD ]; then
    CROSS_BUILD=local
fi

echo "MODEL_PATH = ${MODEL_PATH}"
echo "TARGET_OS = ${CL_TARGET_OS}"
echo "TARGET_PLATFORM = ${CL_TARGET_PLATFORM}"
echo "CROSS_BUILD = ${CROSS_BUILD}"
echo "ASP_BUILD=${ASP_BUILD}"
echo "MODEL_BUILD=${MODEL_BUILD}"

if [ $ASP_BUILD -eq 1 ] && [ $MODEL_BUILD -eq 0 ]; then
    LABEL_FILE="${ASP_TARGET}/build_label"
else
    LABEL_FILE="${PROJECT_ROOT}/${ASP_MODEL_NAME}/target/${CL_TARGET_PLATFORM}/${CL_TARGET_OS}/build_label"
fi

echo "label file = ${LABEL_FILE}"
#
# If we've been called with the "CLEAN" argument then don't label
# the build, but rather wipe the label off of the build.
if [ $# -gt 0 ]
then
    if [ "$1" = "CLEAN" ]
    then
        echo -n "removing ${LABEL_FILE} ..."
        rm -f "${LABEL_FILE}"
        echo "done."
        exit
    fi
fi

# Check the cross build tool described in the target directory.
# If there is no such description then write the current cross
# build tool chain to the label.  If there is a description,
# make sure that it matches the current value.  If it doesn't
# match then complain and exit with an error code.
if [ -f ${LABEL_FILE} ]
then
    OLD_LABEL=`cat ${LABEL_FILE}`
    if [ ! -z "${OLD_LABEL}" -a "${OLD_LABEL}" != "${CROSS_BUILD}" ]
    then
        echo "old label does not match new cross build label"
        echo "old label file = ${LABEL_FILE}"
        echo "old label = ${OLD_LABEL}"
        echo "new label = ${CROSS_BUILD}"
        exit 1
    fi

    # Just in case the build has the correct cross build tools.
    if [ ! -z "${OLD_LABEL}" -a "${OLD_LABEL}" = "${CROSS_BUILD}" ]
    then
        exit 0
    fi
fi

# Before we write the cross build tools name to the build label file
# check and make sure that the directory where the label file is
# supposed to reside actually exists.  Yes, this is probably excessively
# anal retentive.
DIR=`dirname ${LABEL_FILE}`
if [ ! -d "${DIR}" ]
then
    echo "directory for label file doesn't exist"
    echo "directory name = ${DIR}"
    exit 1
fi

touch "${LABEL_FILE}" && echo "${CROSS_BUILD}" > "${LABEL_FILE}"
if [ $? -ne 0 ]
then
    echo "Error encountered in updating label file"
    echo "Label file = ${LABEL_FILE}"
    exit 1
fi
exit 0
