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

# load values from our project area config file
# NOTE that we need to have CLOVIS_ROOT and ASP_MODEL_NAME in our
# environment
if [ -f  ${PROJECT_ROOT}/.config ]; then
  echo "Sourcing ${PROJECT_ROOT}/.config"
  source ${PROJECT_ROOT}/.config
fi
TARGET_DIR=${PROJECT_ROOT}/target/${ASP_MODEL_NAME}
NODE_CONF=${TARGET_DIR}/nodes.conf
TARGET_CONF=${MODEL_PATH}/target.conf

# global variables for TIPC warning
TIPC_ABSENT=0
WARN_TIPC=0

#Use different commands for Solaris
HOST_OS=`uname -s`
if [ $HOST_OS == "SunOS" ]; then
    SED_OPTION=""
else
    SED_OPTION="--in-place"
fi

#
# Create node.conf by extracting info from clAmfConfig.xml
python ${CLOVIS_ROOT}/SAFplus/build/instantiate/scripts/extract_nodeinfo.py \
    ${MODEL_PATH}/config/clAmfConfig.xml > ${NODE_CONF}

#
# source the node conf and target conf files to load requisite data
source ${NODE_CONF}
source ${TARGET_CONF}


# Create targetconf.xml 
TARGET_CONF_XML_FILE=${TARGET_DIR}/images/${ARCH}/etc/targetconf.xml
python ${CLOVIS_ROOT}/SAFplus/build/instantiate/scripts/target_conf_to_xml.py \
    ${TARGET_CONF} > ${TARGET_CONF_XML_FILE}


#
# Function definitions.
#
# target_conf_error dumps a generic error message informing user
# that setting information in target.conf will be necessary if they
# are to make any progress
target_conf_error(){
    echo "*****************************************************************"
    echo "Please edit the target.conf file in the directory:" 
    echo "${MODEL_PATH}"
    echo "before proceeding."
    echo "*****************************************************************"
}

#
# find_in_array takes an array value and searches to find the first
# entry equal to the search value
# ARGS:
#   1: array
#   2: search
# PRINTS: index in the array of the first matching value, nothing otherwise
# RETURNS: 0 if a match is found, 1 if no match
find_in_array() {
    declare -a ARR="$1"
    SEARCH=$2

    declare -i idx=0
    for i in ${ARR[@]}
    do
        if [ ${i} = "${SEARCH}" ]
        then
            echo ${idx}
            return 0
        fi
        let idx++
    done
    return 1
}
#
# A function to instantiate a blade template.
# The template is specified on the command line, as is the name of the
# desired instantiation.
instantiate() {
    UNDEFINED_MSG="is not defined.  Usage: instantiate template inst_name"
    TEMPLATE=${1:?"template ${UNDEFINED_MSG}"}
    INSTANCE=${2:?"instance ${UNDEFINED_MSG}"}
    MULTIPLICITY=${3:-"UNDEFINED"}

    if [ ! -d ${TEMPLATE} ]
    then
        echo "${TEMPLATE} is not a legitimate template directory"
        exit 1
    fi
    if [ ${TEMPLATE} = ${INSTANCE} ]
    then
        echo "template name is the same as the instance name"
        exit 1
    fi

    # Create the directory structure of the template in the instance
    mkdir -p ${INSTANCE}
    ( cd ${TEMPLATE} ; find . -type d -print ) |
    while read LINE
    do
        mkdir -p ${INSTANCE}/${LINE}
    done

    # Create links to the templates files in the instance
    ( cd ${TEMPLATE} ; find . \( -type f -o -type l \) -print ) |
    while read LINE
    do
        ln ${TEMPLATE}/${LINE} ${INSTANCE}/`dirname ${LINE}` 2>/dev/null
    done

    if [ ${MULTIPLICITY} == "yes" ]
    then
        use distributed ${INSTANCE}/etc
    elif [ ${MULTIPLICITY} == "no" ]
    then
        use single ${INSTANCE}/etc
    fi
}

get_val() {
    VAR=${1:?Variable is not specified in get_val}
    NODE=${2:?Node is not specified in get_val}
    PROG=${CLOVIS_ROOT}/SAFplus/build/common/scripts/extract_targetinfo.py

    python ${PROG} --target-conf "${TARGET_CONF}" --variable ${VAR} ${NODE}
    return $?
}

#
# A function to localize asp.conf file.  Not really localizing it
# (as in i18n/l10n) but rather setting local values for the particular
# node to which we are currently installing in the asp.conf file
# The "asp" script is now just one script instead of a asp and a
# asp.payload script in source.  Instead, we have the asp script
# read the asp.conf file and behave in accordance with the settings
# found in the asp.conf file.
# Usage: lcliz_conf conf_file node_name cluster_number logical_slot 
#        boot_config link [hpi_addr] [tipc_netid]
lcliz_conf() {
    UNDEFINED_MSG="is not defined.  Usage: lcliz_conf conf_file node_name cluster
    _number logical_slot boot_config link [hpi_addr] [snmp] [tipc_netid]"
    FILE=${1:?"conf_file ${UNDEFINED_MSG}"}
    NODE=${2:?"node_name ${UNDEFINED_MSG}"}
    CLUSTER=${3:?"cluster_number ${UNDEFINED_MSG}"}
    SLOT=${4:?"logical_slot ${UNDEFINED_MSG}"}
    BOOT=${5:?"boot_config ${UNDEFINED_MSG}"}
    LINK=${6:?"link ${UNDEFINED_MSG}"}
    HPI=${7:-"UNDEFINED"}
    SNMP=${8:-"UNDEFINED"}
    NETID=${9:-"UNDEFINED"}
    TIPC_BUILD=0

    # tell asp.conf if this is a TIPC build
    if [ "${BUILD_TIPC+set}" = set ]
    then
        TIPC_BUILD=1
    fi

    # do a little consistency checking on the "optional" parameters
    if [ "${HPI}" = "" ]
    then
        HPI="UNDEFINED"
    fi
    if [ "${SNMP}" = "" ]
    then
        SNMP="UNDEFINED"
    fi
    if [ "${NETID}" = "UNDEFINED" ]
    then
        NETID=
    fi

    if idx=`find_in_array "(${SYSTEM_CONTROLLERS[*]})" ${NODE}`
    then
        echo "  This is a system controller"
        SYSTEM_CONTROLLER=1
    else
        echo "  This is a payload blade"
        SYSTEM_CONTROLLER=0
    fi

    #
    # This is weird/stupid.  We are getting a bunch of things that
    # come from the target.conf file passed in to us as parameters.
    # But, the list is already long and unwieldy.  So, rather than
    # adding another parameter (or two, actually) I am going to go
    # to the source and get the values I want directly from the
    # target.conf file.
    # Now the only question is whether I should get rid of most of
    # the other parameters from this function.
    if [ -f ${TARGET_CONF} ]
    then
        eval `get_val PERSIST ${NODE}`
        RES=$?
        eval `get_val VOLATILE ${NODE}`
        if [ $? -ne 0 -o $RES -ne 0 ]
        then
            echo "Error encountered in searching target config file"
            return 1
        fi
    else
        echo "No target.conf file: ${TARGET_CONF}"
        return 1
    fi

    # Build up the edit string that will make necessary transformation
    # to the asp.conf file
    # Please note that ASP_SIMULATION is set in make-cross.mk itself.
    EDIT_STRING="/^export NODENAME=/s/=[^=]*\$/=${NODE}/
1
/^export DEFAULT_NODEADDR=/s/=[^=]*\$/=${SLOT}/
1
/^export AUTO_ASSIGN_NODEADDR=/s/=[^=]*\$/=${AUTO_ASSIGN_NODEADDR}/
1
/^export SAHPI_UNSPECIFIED_DOMAIN_ID=/s/=[^=]*\$/=${HPI}/
1
/^export SNMP_TRAP_ADDR=/s/=[^=]*\$/=${SNMP}/
1
/^export BUILD_TIPC=/s/=[^=]*\$/=${BUILD_TIPC}/
1
/^export TIPC_NETID=/s/=[^=]*\$/=${NETID}/
1
/^export LINK_NAME=/s/=[^=]*\$/=${LINK}/
1
/^export ASP_SIMULATION=/s/=[^=]*\$/=${ASP_SIMULATION}/
1
/^export SYSTEM_CONTROLLER=/s/=[^=]*\$/=${SYSTEM_CONTROLLER}/
1
/^export ASP_VALGRIND_CMD=/s/=[^=]*\$/=${ASP_VALGRIND_CMD}/
a
.
"

    #
    # NOTE that we do this this way because it allows us the option of
    # building up the edit string differently.  We can apply some logic
    # to the creation of the edit string.  e.g. if the value of one of
    # the parameters is X say, then we could put in one edit command vs
    # some other edit command.
    # I know, I know, we're not using that ability now, but I was and I
    # thought I'd leave the "infrastructure" the same to allow that
    # flexibility in the future.
    # tmp.out just shoves the output somewhere where it is not visible 
    # but can be used for debug
    chmod u+w ${FILE}
    echo "  Creating file ${FILE}"
    ed ${FILE} >& tmp.out << EOF
${EDIT_STRING}
w
q
EOF

    if [ $? -ne 0 ]
    then
        echo "Failure editing asp.conf config file: ${FILE}"
        return 1
    fi
}

# Update the MulticastAddress and/or MulticastPort fields in gmsconfig
# if new values are specfied in target.conf
if [ "$GMS_MCAST_IP" ]; then
    sed ${SED_OPTION} -e "s/<multicastAddress>[^<]*</<multicastAddress>$GMS_MCAST_IP</g" \
        ${TARGET_DIR}/images/${ARCH}/etc/clGmsConfig.xml
fi
if [ "$GMS_MCAST_PORT" ]; then
    sed ${SED_OPTION} -e "s/<multicastPort>[^<]*</<multicastPort>$GMS_MCAST_PORT</g" \
        ${TARGET_DIR}/images/${ARCH}/etc/clGmsConfig.xml
fi

# Do some minimal consistency checking.
# 1: For each entry in the NODE_INSTANCES array, there should be a
#    matching NODETYPE value.
# 2: For each NODETYPE value, there should be a matching value in the
#    NODE_TYPES array
# 3: For each NODETYPE value, there should be a matching BOOTCONFIG
#    value
# 4: For each entry in the NODE_INSTANCES array that has a slot defined,
#    there should be a matching ARCH value.
# 5: For each matching ARCH value there should be a directory in the
#    ${TARGET_DIR}/images.  That directory should have bin, etc, lib,
#    modules, and share directories.

for i in ${NODE_INSTANCES[@]}
do
    eval TYPE='$'NODETYPE_${i}
    if [ "${TYPE}" = "" ]
    then
        echo "No NODETYPE entry for node ${i}"
        exit 1
    fi
    if ! idx=`find_in_array "(${NODE_TYPES[*]})" ${TYPE}`
    then
        echo "Type: ${TYPE} for node ${i} not found in NODE_TYPES array"
        exit 1
    fi
    eval BOOTCONFIG='$'BOOTCONFIG_${TYPE}
    if [ "${BOOTCONFIG}" == "" ]
    then
        echo "There is no boot config in nodes.conf for node type: ${TYPE}"
        exit 1
    fi
    SLOT=""
    eval `get_val SLOT ${i}`
    if [ ! -z "${SLOT}" ]
    then
        ARCH=""
        eval `get_val ARCH ${i}`
        if [ "${ARCH}" = "" ]
        then
            echo "No ARCH entry for node ${i}"
            exit 1
        fi
        if [ ! -d "${TARGET_DIR}/images/${ARCH}" ]
        then
            echo "No architecture directory (${ARCH}) for node ${i}"
            exit 1
        fi
        D="${TARGET_DIR}/images/${ARCH}"
        if [ ! \( -d ${D}/bin -a -d ${D}/etc -a -d ${D}/lib -a \
                -d ${D}/modules \) ]
        then
            echo "Incomplete source directory ${D}"
            echo "The source directory should have bin, etc, lib, modules,"
            echo "and share subdirectories."
            exit 1
        fi
    fi
done

#
# Invoke any model specific instantiate.sh script
MODEL_SOURCE_DIR=${MODEL_PATH}
SCRIPT=${MODEL_SOURCE_DIR}/build/scripts/instantiate.sh
if [ -f ${SCRIPT} ]
then
    chmod a+x ${SCRIPT}
    ${SCRIPT} ${TARGET_DIR}/images/${ARCH}
fi
#
# Now, for each entry in the NODE_INSTANCES array instantiate that
# node's image from the appropriate architecture specific image
for i in ${NODE_INSTANCES[@]}
do
    SLOT=""
    eval `get_val SLOT ${i}`
    if [ "${SLOT}" = "" ]
    then
        echo "No slot defined for node ${i} in target.conf file"
        echo "Skipping node ${i}"
        continue
    fi
    #
    # Get Link information from target.conf
    LINK=""
    eval `get_val LINK ${i}`
    if [ "${LINK}" = "" ]
    then
        echo "No link interface defined for node ${i} in target.conf file,"
        echo "assuming eth0."
        LINK=eth0
    fi
    #
    # Get TIPC netid information from target.conf if we've been built with
    # TIPC
    if [ "${BUILD_TIPC+set}" = set -a "${TIPC_NETID}" = "" ]
    then
        echo "No value specified for TIPC_NETID in target.conf file, assuming"
        echo "that TIPC will be properly configured on the node before running"
        echo "SAFplus"
    fi
    #
    # If we've been built with the chassis manager then we should specify
    # the CMM_IP address
    if [ "${CM_BUILD}" = 1 -a "${CMM_IP}" = "" ]
    then
        echo "No value specified for CMM_IP in target.conf file.  You will need to configure OpenHPI yourself"
    fi
    
    # If we've been built with support for SNMP traps then we should specify
    # the SNMP trap address
    if [ "${SNMP_BUILD}" = 1 -a "${TRAP_IP}" = "" ]
    then
        echo "No value specified for TRAP_IP in target.conf"
        target_conf_error
        exit 1
    fi
    eval TYPE='$'NODETYPE_${i}
    eval BOOTCONFIG='$'BOOTCONFIG_${TYPE}
    rm -rf ${TARGET_DIR}/${i}
    ARCH=""
    eval `get_val ARCH ${i}`

    asp_conf_file=${TARGET_DIR}/images/asp_${i}.conf
    if [ "$INSTANTIATE_IMAGES" == "YES" ]; then
        instantiate ${TARGET_DIR}/images/${ARCH} ${TARGET_DIR}/images/${i}
        asp_conf_file=${TARGET_DIR}/images/${i}/etc/asp.conf
   
        # copy the targetconf.xml file but only if it isn't already hardlinked to the same place
        if [ ${TARGET_CONF_XML_FILE} -ef ${TARGET_DIR}/images/${i}/etc/targetconf.xml ]; then
          echo ""
        else
          cp ${TARGET_CONF_XML_FILE} ${TARGET_DIR}/images/${i}/etc/
        fi
 
        # check if this is a local build and if we are not copying tipc files
        if [ -f ${TARGET_DIR}/images/${ARCH}/local_build ]; then
            if [ ! -f ${TARGET_DIR}/images/${i}/bin/tipc-config ]; then
                if [ $TIPC_ABSENT -eq 1 ]; then
                    WARN_TIPC=1
                else
                    TIPC_ABSENT=1
                fi
            fi
            rm -f ${TARGET_DIR}/images/${i}/local_build
        fi
    fi
    echo ""
    echo "Building ${i}.  Default Slot: ${SLOT}. Architecture: ${ARCH}.  Intracluster network: ${LINK}."
    
    cp ${CLOVIS_ROOT}/SAFplus/build/instantiate/templates/asp.conf ${asp_conf_file}

    lcliz_conf ${asp_conf_file} ${i} 0 ${SLOT} ${BOOTCONFIG} ${LINK} "${CMM_IP}" "${TRAP_IP}" "${TIPC_NETID}"

    if [ $? -ne 0 ]
    then
        echo "localization failed"
        echo "Node ${i}, slot ${SLOT}, BOOT ${BOOTCONFIG}, CMM ${CMM_IP},"
        echo "TRAP ${TRAP_IP} LINK ${LINK} TIPC_NETID ${TIPC_NETID}"
        exit 1
    fi

    # Append any runtime environment variables to our localized asp.conf
    # First, the global target environment
    if [ -f ${MODEL_PATH}/target.env ]; then
        cat ${MODEL_PATH}/target.env >> ${asp_conf_file}
    fi
    # Then, the node-specific target environment
    if [ -f ${MODEL_PATH}/target_${i}.env ]; then
        cat ${MODEL_PATH}/target_${i}.env >> ${asp_conf_file}
    fi

    #The node-specific transport configuration 
    if [ -f ${MODEL_PATH}/config/clTransport_${i}.xml ]; then
        cp ${MODEL_PATH}/config/clTransport_${i}.xml ${TARGET_DIR}/images/${i}/etc/clTransport.xml
        #Cleanup other node-specific
        rm -rf ${TARGET_DIR}/images/${i}/etc/clTransport_*.xml
    fi

    if [ "${CREATE_TARBALLS}" = "YES" ]; then
        if [ "$INSTANTIATE_IMAGES" == "YES" ]; then
            echo "Creating tarball: ${TARGET_DIR}/images/${i}.tgz"
            pushd ${TARGET_DIR}/images/${i}
            tar cf - . | gzip > ../${i}.tgz
            popd
        else
            echo "  Configured to skip per-blade tarball; a common tarball will be created instead."
        fi
    fi
done


if [ "${CREATE_TARBALLS}" = "YES" ]; then
        if [ "$INSTANTIATE_IMAGES" == "YES" ]; then
            echo "Blade specific tarballs created."
        else
            generic_node=$(echo ${ARCH} | sed -e 's;/;-;g')
            echo ""
            echo "Creating tarball: ${TARGET_DIR}/images/${generic_node}.tgz"
            pushd ${TARGET_DIR}/images/${ARCH}
            tar cf - . | gzip > ../../${generic_node}.tgz
            popd
        fi
fi

if [ $WARN_TIPC -eq 1 ]; then
    echo ""
    echo "---------------------------------------------------------------------------"
    echo "WARNING: You are deploying multiple nodes built locally on this development"
    echo "host.  These target images do not contain tipc, as it is already present on"
    echo "this development host.  Please ensure that the rest of the nodes designated"
    echo "to run these images have the tipc kernel module and tipc-config application"
    echo "installed before deploying and running SAFplus.                               "
    echo "---------------------------------------------------------------------------"
fi
