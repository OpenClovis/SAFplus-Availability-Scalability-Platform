#!/bin/bash

# command for manipulating lilo configuration.
# If invoked with "-l" parameter list lilo image labels found in the
# lilo.conf file.  Mark the active image with an asterisk on the list
# If invoked without any arguments, print the currently active label
# from the lilo.conf file
# If invoked with a single argument that isn't a flag (doesn't have a "-")
# then check that that argument is a valid boot image in the lilo.conf
# file.  If it is then edit the lilo.conf file and set the "default"
# value to the newly specified label.  Then run the lilo command.

#LILO_DIR=/boot/lilo
LILO_DIR=.
LILO_CONF=${LILO_DIR}/lilo.conf
LILO_PROG=${LILO_DIR}/lilo

OPTS="tlh"
LONGOPTS="test,list,help"

get_var(){
    echo $1 | sed -e 's/[ 	]*=.*//'
    return 0
}

get_val(){
    echo $1 | sed -e 's/^[^=]*=[ 	]*//'
    return 0
}
usage() {
    echo "Usage: ossswitch [--list | --test | --help | label]"
    if [ $# -gt 0 ]
    then
        echo "Switch the default boot label in LILO"
        echo ""
        echo "  -l, --list      List available labels in lilo.conf"
        echo "  -t, --test      Run a selftest function.  NOT FOR CASUAL USERS"
        echo "  -h, --help      Print this help message and exit"
        echo ""
        echo "With just the label argument, set the default to that label in lilo.conf"
        echo "and run lilo"
        echo ""
        echo "With no arugment, print the current default label from lilo.conf"
        echo ""
        echo ""
    fi
    return 0
}

get_images_and_labels()  {
    if [ $# -ne 1 ]
    then
        echo "missing parameter: lilo configuration file"
        exit 1
    fi
    egrep '(^image|^[ 	]*label=|^[ 	]*root=)' ${1} |
        sed -e 's/^[ 	]*//' -e 's/[ 	]*=[ 	]*/=/'
    return 0
}

run_lilo() {
    echo "RUNNING LILO PROGRAM NOW"
    # ${LILO_PROG} -C ${LILO_CONF}
    return 0
}

checkRootFileSystem() {
    if [ $# -ne 1 ]
    then
        echo "Usage: checkRootFileSystem block_device_name"
        return 1
    fi
    ROOT_FS=$1

    if [ ! -d ${LILO_DIR}/MOUNT_DIR ]
    then
        mkdir -p ${LILO_DIR}/MOUNT_DIR
    fi
    if [ ! -d ${LILO_DIR}/MOUNT_DIR ]
    then
        echo "No mount test dir (MOUNT_DIR) in ${LILO_DIR}"
        return 1
    fi

    mount ${ROOT_FS} ${LILO_DIR}/MOUNT_DIR
    STATUS=$?
    if [ $STATUS -ne 0 ]
    then
        echo "mount of ${ROOT_FS} to ${LILO_DIR}/MOUNT_DIR failed"
        return 1
    fi

    if [ ! -f ${LILO_DIR}/MOUNT_DIR/etc/passwd ]
    then
        echo "${ROOT_FS} doesn't have a '/etc/passwd' file"
        umount ${ROOT_FS}
        return 1
    fi

    umount ${ROOT_FS}

    return 0
}

#
# Slight bogosity here.  This function assumes that there is a trio
# of "global" arrays: images, roots, and labels.  It will update those
# arrays wth what it finds in  the configuration file.  If it finds
# a tag that it's not expecting it will return an error.  If it finds
# an image section without a label and root, or a label or root that
# isn't obviously in an image section then one or more entries in
# either the labels or the images array will be a null string.
parse_images() {
    if [ $# -ne 1 ]
    then
        echo "missing parameter: lilo configuration file"
        exit 1
    fi
    CONF=$1
    DEFAULT_ROOT=""
    LIST=`get_images_and_labels ${CONF}`
    for line in ${LIST}
    do
        tag=`get_var $line`
        case "$tag" in
        image)
            if [ "${#images[@]}" -gt "${#labels[@]}" ]
            then
                # We're missing a label for previous image
                # mark it with an empty label
                labels[${#labels[@]}]=""
            fi
            if [ "${#images[@]}" -gt "${#roots[@]}" ]
            then
                # We're missing a root for previous image.
                # Mark it with the default root
                roots[${#roots[@]}]=${DEFAULT_ROOT}
            fi
            images[${#images[@]}]=`get_val $line`
            ;;
        label)
            # check for a label tag without preceeding image tag
            if [ "${#images[@]}" -ne `expr "${#labels[@]}" + 1` ]
            then
                images[${#images[@]}]=""
            fi
            labels[${#labels[@]}]=`get_val $line`
            ;;
        root)
            # check whether this is a global root line.   That is a root
            # specifier outside of any image section
            if [ "${#images[@]}" -eq 0 ]
            then
                DEFAULT_ROOT=`get_val $line`
            # check for a root tag without preceeding image tag
            elif [ "${#images[@]}" -ne `expr "${#roots[@]}" + 1` ]
            then
                images[${#images[@]}]=""
                roots[${#roots[@]}]=`get_val $line`
            else
                roots[${#roots[@]}]=`get_val $line`
            fi
            ;;
        *)
            echo "Unrecognized tag ($tag) in ${CONF}"
            return 1
            ;;
        esac
    done 
    return 0;
}


list_images() {
    if [ $# -ne 1 ]
    then
        echo "missing parameter: lilo configuration file"
        exit 1
    fi
    CONF=$1
    declare -a images
    declare -a labels
    parse_images ${CONF}
    STATUS=$?
    if [ ${STATUS} != 0 ]
    then
        echo "error encountered while trying to parse config file($CONF)."
        return 1
    fi

    echo "Kernels to choose from:"
    defaultline=`grep ^default= ${CONF} | head -1`
    default=`get_val ${defaultline}`
    default_found=0
    declare -i i
    i=0
    while [ "${i}" -lt "${#labels[@]}" ]
    do
        if [ "${labels[$i]}" != "" -a "${images[$i]}" != "" ]
        then
            if [ "${default}" != "" -a "${default}" = "${labels[$i]}" ]
            then
                echo " * ${labels[$i]}: ${images[$i]}"
                default_found=1
            else
                echo "   ${labels[$i]}: ${images[$i]}"
            fi
        fi

        let i++
    done
    if [ "${default_found}" = 0 ]
    then
        echo "Default label (${default}) was not found in config file (${CONF})"
    fi

    return 0
}

make_image_active() {
    if [ $# -ne 2 ]
    then
        echo "missing parameter: label"
        exit 1
    fi
    CONF=$1
    DESIRED=$2
    defaultline=`grep ^default= ${CONF} | head -1`
    ORIGINAL=`get_val ${defaultline}`

    declare -a images
    declare -a labels
    parse_images ${CONF}
    STATUS=$?
    if [ ${STATUS} != 0 ]
    then
        echo "error encountered while trying to parse config file($CONF)."
        return 1
    fi

    declare -i i
    i=0
    while [ "${i}" -lt "${#labels[@]}" ]
    do
        if [ "${labels[$i]}" != "" -a "${images[$i]}" != "" ]
        then
            if [ "${DESIRED}" = "${labels[$i]}" ]
            then
                if [ ! -f "${images[$i]}" ]
                then
                    echo "There is no image file (${images[$i]}) for label ${DESIRED}"
                    return 1
                elif [ "${roots[$i]}" = "" -o ! -b "${roots[$i]}" ]
                then
                    echo "There is no root fs (${roots[$i]}) for label ${DESIRED}"
                    return 1
                elif ! checkRootFileSystem "${roots[$i]}"
                then
                    echo "${roots[$i]} is not a valid root file system"
                    return 1
                else
                    ed ${CONF} 2> /dev/null << EOF
/^default=/s/=.*\$/=${DESIRED}/
wq
EOF
                    STATUS=$?
                    if [ ${STATUS} -ne 0 ]
                    then
                        echo "Failed to change config file (${CONF})"
                        return 1
                    fi
                    run_lilo
                    STATUS=$?
                    if [ ${STATUS} -ne 0 ]
                    then
                        echo "Failure in lilo run.  Restoring config file."
                        ed ${CONF} 2> /dev/null << EOF
/^default=/s/=.*\$/=${ORIGINAL}/
wq
EOF
                        STATUS=$?
                        if [ ${STATUS} -ne 0 ]
                        then
                            echo "Failed to restore lilo config file(${CONF})"
                            return 1
                        fi
                        return 1
                    fi
                fi
                return 0
            fi
        fi

        let i++
    done

    echo "Couldn't find label (${DESIRED}) in config file (${CONF})"
    return 1
}

find_active_label() {
    if [ $# -ne 1 ]
    then
        echo "missing parameter: lilo configuration file"
        exit 1
    fi
    CONF=$1

    defaultline=`grep ^default= ${CONF} | head -1`
    default=`get_val ${defaultline}`
    if [ "${default}" = "" ]
    then
        echo "There is no default label defined" >2
        return 1
    fi

    declare -a images
    declare -a labels
    parse_images ${CONF}
    STATUS=$?
    if [ ${STATUS} != 0 ]
    then
        echo "error encountered while trying to parse config file($CONF)."
        return 1
    fi

    declare -i i
    i=0
    while [ "${i}" -lt "${#labels[@]}" ]
    do
        if [ "${labels[$i]}" != "" -a "${images[$i]}" != "" ]
        then
            if [ "${default}" = "${labels[$i]}" ]
            then
                echo "The current active label is ${labels[$i]}"
                if [ ! -f "${images[$i]}" ]
                then
                    return 1
                else
                    return 0
                fi
            fi
        fi

        let i++
    done
    echo "The current active label is ${default}"
    return 1
}


run_self_test() {
    declare -a images
    declare -a labels
    parse_images ${LILO_CONF}
    STATUS=$?
    if [ ${STATUS} -ne 0 ]
    then
        echo "FAILURE in parse_images"
        exit 1
    fi
    echo "images = ${images[@]}"
    echo "labels = ${labels[@]}"
    echo "roots = ${roots[@]}"

    echo ""
    echo "NEXT TEST"
    echo ""
    list_images ${LILO_CONF}
    echo ""
    echo "NEXT TEST"
    echo ""
# Check that make_image_active can activate known good labels
    make_image_active ${LILO_CONF} wrs-pnele-1.2
    status=$?
    if [ "${status}" != 0 ]
    then
        echo "Error in make_image_active wrs-pnele-1.2"
        exit 1
    fi
    echo ""
    echo "NEXT TEST"
    echo ""
    active=`find_active_label ${LILO_CONF} | sed -e 's/^The current active label is //'`
    STATUS=$?
    echo "active = $active"
    if [ ${STATUS} != 0 -o "${active}" != "wrs-pnele-1.2" ]
    then
        echo "The wrong label is active"
        exit 1
    else
        echo "Got the right active label"
    fi
    echo ""
    echo "NEXT TEST"
    echo ""
# another known good label
    make_image_active ${LILO_CONF} wrs-pnele-1.4
    status=$?
    if [ "${status}" != 0 ]
    then
        echo "Error in make_image_active wrs-pnele-1.4"
        exit 1
    fi
    echo ""
    echo "NEXT TEST"
    echo ""
# Test for a failure case.
    make_image_active ${LILO_CONF} nonesuch
    status=$?
    if [ "${status}" = 0 ]
    then
        echo "Error in make_image_active nonesuch"
        exit 1
    fi
    echo ""
    echo "NEXT TEST"
    echo ""

    active=`find_active_label ${LILO_CONF} | sed -e 's/^The current active label is //'`
    STATUS=$?
    echo "active = $active"
    if [ ${STATUS} != 0 -o "${active}" != "wrs-pnele-1.4" ]
    then
        echo "The wrong label is active"
        exit 1
    else
        echo "Got the right active label"
    fi
    return 0
}


TEST=""
LIST=""

eval set -- `getopt --options="${OPTS}" --longoptions="${LONGOPTS}" -- "$@"`
while [ $# -gt 0 ]
do
    case "$1" in
    -t|--test)
        TEST=true
        # echo "setting TEST to true"
        ;;
    -l|--list)
        LIST=true
        # echo "setting LIST to true"
        ;;
    -h|--help)
        usage --verbose
        exit 0
        ;;
    --)
        shift
        break
        ;;
    *) 
        usage
        exit 1
        ;;
    esac
    shift 
done

# If we have another argument left, it must be a label that was requested
if [ $# -gt 0 ]
then
    LABEL=$1
    # echo "setting LABEL to $1"
fi

# If none of the args were set then we must have been called to
# 1) check on the lilo.conf file
# 2) report the currently active label
if [ "${LIST}" = "" -a "${TEST}" = "" -a "${LABEL}" = "" ]
then
    find_active_label "${LILO_CONF}"
    STATUS=$?
    if [ "${STATUS}" -ne 0 ]
    then
        echo "The config file (${LILO_CONF}) has no image for current label"
        exit 1
    else
        exit 0
    fi
fi

# If the --list option was specified then generate a list of available labels
if [ "${LIST}" != "" ]
then
    list_images ${LILO_CONF}
    STATUS=$?
    if [ ${STATUS} -ne 0 ]
    then
        echo "Failure in listing labels"
        exit 1
    else
        exit 0
    fi
fi

# If a label was specified on the command line then make that the
# active label
if [ "${LABEL}" != "" ]
then
    make_image_active "${LILO_CONF}" "${LABEL}"
    STATUS=$?
    if [ ${STATUS} -ne 0 ]
    then
        echo "Failure in setting label"
        exit 1
    else
        exit 0
    fi

fi

# If --test was specified then run a self test
if [ "${TEST}" != "" ]
then
    run_self_test
    STATUS=$?
    if [ ${STATUS} -eq 0 ]
    then
        echo "self test passed"
        exit 0
    else
        echo "self test failed"
        exit 1
    fi
fi
