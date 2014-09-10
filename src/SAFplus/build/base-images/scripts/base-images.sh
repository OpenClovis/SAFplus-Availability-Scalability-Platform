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

##############################################################################
# Populate images with binaries, libraries, scripts, config files,
# etc. from the ASP build.
##############################################################################

# create and populate an image directory.
# Parameters:
#   source model dir    - base directory of source model dir
#   target model dir    - base directory of destination dir
#   architecture name   - e.g. i386, i686, ppc
#   system name         - linux version.  e.g. linux-2.6.14
#   stripped            - whether to strip images or not. default unstripped
strip_image="0"
if [ $# -eq 1 ]; then
    strip_image="$1"
fi
populate_image() {
    if [ $# -ne 4 ]
    then
        echo "Usage: populate_image source target arch system"
        return 1
    fi

    SOURCE_MODEL=$1
    TARGET_MODEL=$2
    ARCH=$3
    SYS=$4

    echo "Populating image directory ${TARGET_MODEL} with binaries from model ${SOURCE_MODEL}"
    echo "Project root is ${PROJECT_ROOT}, ASP installation: ${ASP_INSTALLDIR}, Not prebuilt? ${ASP_BUILD}"

    if [ "${SOLARIS_BUILD}" = "1" ]
    then
	INSTALL=/usr/ucb/install
    else
        INSTALL=install
    fi

    #
    # Do some consistency checking on the parameters:
    #
    if [ ! -d ${SOURCE_MODEL} ]
    then
        echo "source and target model dirs should already exist"
        echo "source model = ${SOURCE_MODEL}"
        echo "target model = ${TARGET_MODEL}"
        return 1
    fi
## AA: source model directory name isn't the same as model name anymore    
#    if [ `basename ${SOURCE_MODEL}` != `basename ${TARGET_MODEL}` ]
#    then
#        echo "model name should be the same for source and target directories"
#        echo "source model directory = ${SOURCE_MODEL}"
#        echo "target model directory = ${TARGET_MODEL}"
#        return 1
#    fi

    SOURCE_MODEL_ROOT=$(dirname $SOURCE_MODEL)

    if [ ! -d ${SOURCE_MODEL_ROOT}/target/${ARCH}/${SYS} ]
    then
        echo "system specific model directory doesn't exist"
        echo "source model directory = ${SOURCE_MODEL}"
        echo "arch = ${ARCH}"
        echo "system = ${SYS}"
        return 1
    fi
    if [ -z "${ARCH}" -o -z "${SYS}" ]
    then
        echo "neither architecture nor system should be empty"
        echo "architicture = ${ARCH}"
        echo "system = ${SYS}"
    fi

    #echo "source_model = ${SOURCE_MODEL}"
    #echo "TARGET_MODEL = ${TARGET_MODEL}"
    #echo "ARCH = ${ARCH}, SYS = ${SYS}"

    mkdir -p ${TARGET_MODEL}
    if [ ! -d "${TARGET_MODEL}" ]
    then
        echo "Directory creation failed for ${TARGET_MODEL}"
        return 1
    fi

    export imagedir=${TARGET_MODEL}/images/${ARCH}/${SYS}

    # Note that these are kindof weird in that they are overriding
    # values set in the makefile.  We do this because the makefile
    # is modified for single arch and system while we want this
    # function to be able to populate image directories for all
    # architectures and systems.
    # bleh.
    export ASP_KMOD=${SOURCE_MODEL_ROOT}/target/${ARCH}/${SYS}/kmod
    # where the built-with-the-model ASP libraries reside
    export ASP_LIB=${PROJECT_ROOT}/target/${ARCH}/${SYS}/lib
    # where the model specific libs reside
    export MODEL_LIB=${SOURCE_MODEL_ROOT}/target/${ARCH}/${SYS}/lib

    export MODEL_BIN=${SOURCE_MODEL_ROOT}/target/${ARCH}/${SYS}/bin

    echo "  Creating directory $targetmodeldir/images..."
    mkdir -p $imagedir
    mkdir $imagedir/bin
    mkdir $imagedir/lib
    mkdir $imagedir/etc
    mkdir $imagedir/etc/asp.d
    mkdir $imagedir/etc/init.d
    mkdir $imagedir/modules

    # fail quietly if file not found
    if [ -f $CLOVIS_ROOT/ASP/VERSION ]
    then
        cp $CLOVIS_ROOT/ASP/VERSION $imagedir/etc 2> /dev/null
    fi

    #
    # Invoke any model specific base-images.sh script
    #MODEL_SOURCE_DIR=${CLOVIS_ROOT}/ASP/models/${ASP_MODEL_NAME}
    SCRIPT=${SOURCE_MODEL}/build/scripts/base-images.sh
    if [ -f ${SCRIPT} ]
    then
        chmod a+x ${SCRIPT}
        ${SCRIPT} ${imagedir}
        if [ $? -ne 0 ]
        then
            echo "  Failure in model specific base-images script"
            return 1
        fi
    fi

    # Populate image directory

    echo "  Populating blade image at $imagedir ... "

    # Copying kernel modules if this isn't a TIPC build
    if [ $BUILD_TIPC = 0 ]; then
        echo "  Copying kernel modules..."
        ${INSTALL} $installflags $ASP_KMOD/*.ko $imagedir/modules
    fi

    # Copying libraries
    echo "  Copying shared libraries..."

   # Install ASP shared libraries
    if [ $ASP_BUILD = 0 ]; then
        echo "    Prebuilt ASP libraries from $ASP_INSTALLDIR/target/$ARCH/$SYS/lib"
        ${INSTALL} $installflags $ASP_INSTALLDIR/target/$ARCH/$SYS/lib/*.so $imagedir/lib
    fi


    export tmp=`/bin/ls  $MODEL_LIB/*.so 2> /dev/null`
    if [ ! -z "$tmp" ]; then  # Install MODEL specific libraries
        echo "    Model specific libraries from $MODEL_LIB"
        echo "    ${INSTALL} $installflags $MODEL_LIB/*.so $imagedir/lib"
        ${INSTALL} $installflags $MODEL_LIB/*.so $imagedir/lib
    fi

        echo "    ASP libraries built with the model located at $ASP_LIB"

    export tmp=`/bin/ls  $ASP_LIB/*.so 2> /dev/null`    
    if [ ! -z "$tmp" ]; then  # Install MODEL specific libraries
        ${INSTALL} $installflags $ASP_LIB/*.so $imagedir/lib
    fi

    # GAS, what is this???
    if [ -d $ASP_LIB/pym ]; then
        cp -r $ASP_LIB/pym $imagedir/lib
    fi

    # Copying binaries and scripts
    echo "  Copying binaries and scripts..."
    for each_exe in $MODEL_BIN/*
    do
        ${INSTALL} $exe_flags $each_exe $imagedir/bin
    done

    if [ -d ${SOURCE_MODEL}/scripts ]; then
        ${INSTALL} $exe_flags ${SOURCE_MODEL}/scripts/* $imagedir/bin
    fi

    ## check if we are supposed to strip 
    if [ "$strip_image" -ne "0" ]; then
        echo "  Stripping the libraries and binaries..."
        strip ${ASP_STRIP_ARGS} $imagedir/lib/* 2>/dev/null
        strip ${ASP_STRIP_ARGS} $imagedir/bin/* 2>/dev/null
    fi

    ${INSTALL} $exe_flags $CLOVIS_ROOT/ASP/build/base-images/scripts/safplus_run $imagedir/bin
    ${INSTALL} $exe_flags $CLOVIS_ROOT/ASP/build/base-images/scripts/safplus_console $imagedir/bin
    ${INSTALL} $exe_flags $CLOVIS_ROOT/ASP/build/base-images/scripts/safplus $imagedir/etc/init.d
    ${INSTALL} $exe_flags $CLOVIS_ROOT/ASP/build/base-images/scripts/asp.py $imagedir/etc
    ${INSTALL} $exe_flags $CLOVIS_ROOT/ASP/build/base-images/scripts/asp_*.py $imagedir/etc
    ${INSTALL} $exe_flags $CLOVIS_ROOT/ASP/build/base-images/scripts/safplus_watchdog.py $imagedir/etc
    ${INSTALL} $exe_flags $CLOVIS_ROOT/ASP/tools/logTools/clLogViewer/safplus_binlogviewer $imagedir/bin
    ${INSTALL} $exe_flags $CLOVIS_ROOT/ASP/build/base-images/scripts/virtualIp $imagedir/bin
    (cd $imagedir/bin; ln -s ./safplus_run asp_run)
    (cd $imagedir/bin; ln -s ./safplus_console asp_console)
    (cd $imagedir/bin; ln -s ./safplus_binlogviewer asp_binlogviewer)
    (cd $imagedir/bin; ln -s ./safplus_info aspinfo)
    (cd $imagedir/etc/init.d; ln -s ./safplus asp)
 
    echo cd $imagedir ln -s etc/init.d/safplus ${ASP_MODEL_NAME}
    (cd $imagedir; ln -s etc/init.d/safplus ${ASP_MODEL_NAME})


    # Copying config files
    echo "  Copying config files... All .xml, .txt, and .conf files in $MODEL_CONFIG will be put into <ASP_install_dir>/etc"
    ${INSTALL} $installflags $MODEL_CONFIG/*.xml $imagedir/etc
    if [ $(ls -1 $MODEL_CONFIG/*.txt 2>/dev/null| wc -l) -gt 0 ]; then
        ${INSTALL} $installflags $MODEL_CONFIG/*.txt $imagedir/etc
    fi
    if [ $(ls -1 $MODEL_CONFIG/*.conf 2>/dev/null| wc -l) -gt 0 ]; then
        ${INSTALL} $installflags $MODEL_CONFIG/*.conf $imagedir/etc
    fi

    # Copy customer specific scripts, config files...anything ;-)
    # The `/' after the extras is needed.
    # For the meaning of the options please see rsync(1)
    echo "  Copying extras... Place any files you want to be part of the final model into directories or subdirectories of $MODEL_PATH/extras."
    if [ -d $MODEL_PATH/extras ]; then
        rsync -avpDHL --ignore-existing --exclude='.svn' $MODEL_PATH/extras/ $imagedir/
    else
        echo "    No extras directory exists."
    fi

    # Touching up snmpd.conf if necessary, and copying over
    if [ $TRAP_IP ]; then
        sed -e "s/trap2sink 127.0.0.1/trap2sink $TRAP_IP/g" $MODEL_CONFIG/snmpd.conf > $imagedir/etc/snmpd.conf
    else
        ${INSTALL} $installflags $MODEL_CONFIG/snmpd.conf $imagedir/etc/snmpd.conf
    fi

    # Touching up openhpi.conf if necessary, and copying over

    if [ $CMM_IP ]; then
        echo "  Requested shelf manager auth_type: $CMM_AUTH_TYPE"
        echo "  Requested shelf manager username:  $CMM_USERNAME"
        echo "  Requested shelf manager password:  $CMM_PASSWORD"
        if [ ! "${CMM_USERNAME-z}" == z ] && \
           [ ! "${CMM_PASSWORD-z}" == z ] && \
           [ ! "${CMM_AUTH_TYPE-z}" == z ]; then
            echo "  Updating addr/auth_type/username/password fields in openhpi.conf..."
            sed -e "s/^[[:blank:]]*addr = \"192.168.30.81\"/        addr = \"$CMM_IP\"/" \
                -e "s/^[[:blank:]]*username = \"root\"/        username = \"$CMM_USERNAME\"/g" \
                -e "s/^[[:blank:]]*password = \"cmmrootpass\"/        password = \"$CMM_PASSWORD\"/g" \
                -e "s/^[[:blank:]]*auth_type = \"none\"/        auth_type = \"$CMM_AUTH_TYPE\"/g" \
			$MODEL_CONFIG/openhpi.conf > $imagedir/etc/openhpi.conf
        else
          echo "  Updating addr field in openhpi.conf..."
          sed -e "s/^[[:blank:]]*addr = \"192.168.30.81\"/\taddr = \"$CMM_IP\"/" \
                $MODEL_CONFIG/openhpi.conf > $imagedir/etc/openhpi.conf
        fi
    else
        ${INSTALL} $installflags $MODEL_CONFIG/openhpi.conf $imagedir/etc/openhpi.conf
    fi

    #
    # Now we need to edit the amfConfig files in order to
    # enable or disable the chassis manager depending on whether
    # we've been built with or without chassis manager executable.
    # We edit amfDefinitions file in order to enable or disable the
    # csaSnmp
    #
    # First changes for chassis manager
    if grep '<aspServiceUnit *name=\"cmSU\"' ${imagedir}/etc/clAmfConfig.xml > /dev/null 2>&1
    then
        if [ "${CM_BUILD}" = "" -o "${CM_BUILD}" = 0 ]
        then
            CONFIG_EDIT_SCRIPT="/<aspServiceUnit *name=\"cmSU\"/c
        <!-- <aspServiceUnit name=\"cmSU\"/> -->
.
1
"
        else
            CONFIG_EDIT_SCRIPT="/<aspServiceUnit *name=\"cmSU\"/c
        <aspServiceUnit name=\"cmSU\"/>
.
1
"
        fi
    fi

    # Next, changes for SNMP
    if grep '<aspServiceUnit *name=\"snmpSU\"' ${imagedir}/etc/clAmfConfig.xml > /dev/null 2>&1
    then
        if [ "${SNMP_BUILD}" = "" -o "${SNMP_BUILD}" = 0 ]
        then
            CONFIG_EDIT_SCRIPT="${CONFIG_EDIT_SCRIPT}
/<aspServiceUnit *name=\"snmpSU\"/c
            <!-- <aspServiceUnit name=\"snmpSU\"/> -->
.
1
"
        else
            CONFIG_EDIT_SCRIPT="${CONFIG_EDIT_SCRIPT}
/<aspServiceUnit *name=\"snmpSU\"/c
            <aspServiceUnit name=\"snmpSU\"/>
.
1
"
        fi
    fi

    #
    # And now run the ed scripts
    chmod u+w ${imagedir}/etc/clAmfConfig.xml
    ed ${imagedir}/etc/clAmfConfig.xml >& tmp.out << EOF
${CONFIG_EDIT_SCRIPT}
w
q
EOF

    echo -n "Cleaning up (removing *.db *.cor AMF_CHECK* AMF_CKPT* AMS_CKPT*) ... "
    cd ${imagedir}
    find . \( -name "*.db" -o -name "*.cor*" -o -name "AMF_CHECK*" -o -name "AMF_CKPT*" -o -name "AMS_CKPT*" \) -exec rm {} \;
    echo "done"
}

# Look to the MODEL_PATH.  There should be such a directory.  Look in
# the ${MODEL_TARGET} directory (which should also be present)

export MODEL_TARGET_ROOT=$(dirname $(dirname $MODEL_TARGET))

if [ ! -d ${MODEL_TARGET_ROOT} ]
then
    echo "There is no directory: ${MODEL_TARGET_ROOT}"
    exit 1
fi

#
# Edit the target.conf to replace any ARCH= lines with a line that
# describes the last architecture specified for configure.
NEW_ARCH="${CL_TARGET_PLATFORM}/${CL_TARGET_OS}"
ed ${MODEL_PATH}/target.conf >& tmp.out << EOF
g/^ARCH=/d
a
ARCH=${NEW_ARCH}
.
w
q
EOF
if [ $? -ne 0 ]
then
    echo "Failed to update target.conf (in ${MODEL_PATH}) with"
    echo "new ARCH information: ${NEW_ARCH}"
    exit 1
fi

# Source our project area config file and target.conf

if [ -f  $PROJECT_ROOT/.openclovis.conf ]; then
  echo "Sourcing $PROJECT_ROOT/.openclovis.conf"
  source $PROJECT_ROOT/.openclovis.conf
fi
source $MODEL_PATH/target.conf

# Setting up and deriving required variables 

targetmodeldir=$PROJECT_ROOT/target/$ASP_MODEL_NAME
export installflags='-m 644'
export exe_flags='-m 755'

# env | sort

echo ""

# verify and create target images directory

if [ -d $targetmodeldir/images ]; then
    rm -rf ${targetmodeldir}/images_backup > /dev/null 2>&1
    echo "Old images saved as ${targetmodeldir}/images_backup"
    mv -f ${targetmodeldir}/images ${targetmodeldir}/images_backup
fi

# In the MODEL_PATH directory there should be a number of architecture 
# directories  .For each architecture directory look for subdirectories.
# Those subdirectories are the "system" directories.  Invoke the
# populate_image function for each architecture/system directory
echo ""

for arch in ${MODEL_TARGET_ROOT}/*
do
    if [ -d "${arch}" ]
    then
        ARCH=`basename "${arch}"`
        for sys in ${arch}/*
        do
            if [ -d "${sys}" ]
            then
                SYS=`basename "${sys}"`

                # Now, it would be really nice if we could do some
                # extra validation on the SYS and ARCH names.  We'll
                # settle for looking for directories in the ${sys}
                # directory
                if [ -d ${sys}/bin -a -d ${sys}/lib ]
                then
                    populate_image ${MODEL_PATH} ${PROJECT_ROOT}/target/${ASP_MODEL_NAME} ${ARCH} ${SYS}
                    if [ $? -ne 0 ]
                    then
                        echo "Failure populating image for arch: ${arch} sys: ${SYS}"
                        exit 1
                    fi
                fi
            fi
        done
    fi
done
echo ""
