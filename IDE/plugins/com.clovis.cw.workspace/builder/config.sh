################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
################################################################################
#!/bin/bash

#This file will set env and configure.

ARGUMENT=""

if [ "$3" != "NO" ]; then
	ARGUMENT=${ARGUMENT}$3
fi

if [ "$4" != "NO" ]; then
	ARGUMENT=${ARGUMENT}" "
	ARGUMENT=${ARGUMENT}$4
fi

if [ "$5" != "NO" ]; then
	ARGUMENT=${ARGUMENT}" "
	ARGUMENT=${ARGUMENT}$5
fi

if [ "$6" != "NO" ]; then
	ARGUMENT=${ARGUMENT}" "
	ARGUMENT=${ARGUMENT}$6
fi

if [ "$7" != "NO" ]; then
	ARGUMENT=${ARGUMENT}" "
	ARGUMENT=${ARGUMENT}$7
fi

if [ "$8" != "NO" ]; then
	ARGUMENT=${ARGUMENT}" "
	ARGUMENT=${ARGUMENT}$8
fi

if [ "$9" != "NO" ]; then
	ARGUMENT=${ARGUMENT}" "
	ARGUMENT=${ARGUMENT}$9
fi

if [ "${10}" != "NO" ]; then
	ARGUMENT=${ARGUMENT}" "
	ARGUMENT=${ARGUMENT}${10}
fi

export ASP_MODEL_NAME=$1
$2/configure --with-model-name=$ASP_MODEL_NAME $ARGUMENT

sourceFile="$2/build/common/conf/target.conf"
dest="${11}/$1/src/"
destFile="/${11}/$1/src/target.conf"

if [ -f "$sourceFile" ]
	then
		if [ ! -f "$destFile" ]
  			then
    			cp $sourceFile $dest
    	fi		
fi


