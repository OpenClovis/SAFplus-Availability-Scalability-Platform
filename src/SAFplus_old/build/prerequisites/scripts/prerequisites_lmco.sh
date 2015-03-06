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
# Populate generic image with third party prerequisite binaries, libraries,
# config files, etc. from the local system or toolchain as necessary.
##############################################################################

# source sdk information

source $CLOVIS_ROOT/.config

# Setting up and deriving required variables 

targetmodeldir=$CLOVIS_ROOT/target/$ASP_MODEL_NAME
imagedir=$targetmodeldir/images/generic
installflags='-m 644'
exe_flags='-m 755'

# env | sort

# verify and create target images directory
# ideally, at this point, the directory structure under $imagedir should
# already exist.  however, if it doesn't (in case this is invoked out of
# order manually), create the tree automatically.

if [ ! \( -d $imagedir/bin -a -d $imagedir/lib -a -d $imagedir/etc -a -d $imagedir/modules \)  ]; then
	echo "Creating directory $imagedir..."
	mkdir -p $imagedir/bin
	mkdir $imagedir/lib
	mkdir $imagedir/etc
	mkdir $imagedir/modules

	# If the directories were not all successfully created then it's a bad
	# failure.  Exit with error
	if [ ! \( -d $imagedir/bin -a -d $imagedir/lib -a -d $imagedir/etc -a $imagedir/modules \) ]; then
		echo "Image directory at $imagedir is incomplete.  Could not create."
		exit 1
	fi
fi

if [ ! -d $targetmodeldir ]; then
	echo "Target model directory at $targetmodeldir does not exist"
	exit 1
fi

# Populate image directory

echo "Populating generic blade image at $imagedir with third party"
echo "prerequisites... "

if [ $CROSS_BUILD ]; then
	# If this is a cross build, use prerequisite files from the toolchain
	echo "Cross build detected, using prerequisites from $CROSS_BUILD toolchain"

	toolchaindir=$CL_BUILDTOOLS/$CROSS_BUILD
	if [ ! \( "$toolchaindir" != "/" -a -d $toolchaindir \) ]; then
		echo "Tool chain directory: $toolchaindir is not a proper directory"
		exit 1
	fi
	
	cd $toolchaindir

	# There may be multiple problems with prerequisites.  Keep track of the
	# problems and tell about them at the end.  If a problem was observed
	# then exit at the end.  I think that this is slightly less yucky than
	# spreading lots of if tests through the code.
	declare -a res_array
	declare -a op_array
	set -o pipefail
	# db
	echo -n "db "
	tar cfh - lib/libdb* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in db"

	# gdbm
	echo -n "gdbm "
	tar cfh - lib/libgdbm* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gdbm"

	# net-snmp
	echo -n "net-snmp "
	install $exe_flags $toolchaindir/sbin/snmp* $imagedir/bin
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in install snmp binaries"
	tar cfh - lib/libnetsnmp* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in libnetsnmp"
	tar cfh - share/snmp | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in db"
	if [ $(ls -1 lib/libstdc++.so.5.0.5 2>/dev/null | wc -l) -gt 0 ]; then
		tar cfh - lib/libstdc++.so.5* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in libstdc++"
	fi
	if [ $(ls -1 lib/libcrypto*.* 2>/dev/null | wc -l) -gt 0 ]; then
		tar cfh - lib/libcrypto*.* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in libcrypto"
	fi
	if [ $(ls -1 lib/libssl*.* 2>/dev/null | wc -l) -gt 0 ]; then
		tar cfh - lib/libssl* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in libssl"
	fi

	# openhpi
	echo "openhpi "
	install $exe_flags $toolchaindir/sbin/openhpid $imagedir/bin
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="install openhpid"
	install $exe_flags $toolchaindir/bin/*hpi* $imagedir/bin
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="install hpi binaries"
	tar cfh - lib/*hpi* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in hpi libraries"
	tar cfh - lib/*oht* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in hpi libraries"
	tar cfh - lib/*ohu* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in hpi libraries"
	tar cfh - lib/*glib* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in glib libraries"
	tar cfh - lib/*gmodule* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gmodule library files"
	tar cfh - lib/*gobject* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gobject library files"
	tar cfh - lib/*gthread* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gthread library files"

	# libhcl
#!/bin/bash
################################################################################
# 
#   Copyright (C) 2002-2009 by OpenClovis Inc. All Rights  Reserved.
# 
#   The source code for this program is not published or otherwise divested
#   of its trade secrets, irrespective of what has been deposited with  the
#   U.S. Copyright office.
# 
#   No part of the source code  for this  program may  be use,  reproduced,
#   modified, transmitted, transcribed, stored  in a retrieval  system,  or
#   translated, in any form or by  any  means,  without  the prior  written
#   permission of OpenClovis Inc
################################################################################
#
# Build: 4.2.0
#

##############################################################################
# Populate generic image with third party prerequisite binaries, libraries,
# config files, etc. from the local system or toolchain as necessary.
##############################################################################

# source sdk information

source $CLOVIS_ROOT/.config

# Setting up and deriving required variables 

targetmodeldir=$CLOVIS_ROOT/target/$ASP_MODEL_NAME
imagedir=$targetmodeldir/images/generic
installflags='-m 644'
exe_flags='-m 755'

# env | sort

# verify and create target images directory
# ideally, at this point, the directory structure under $imagedir should
# already exist.  however, if it doesn't (in case this is invoked out of
# order manually), create the tree automatically.

if [ ! \( -d $imagedir/bin -a -d $imagedir/lib -a -d $imagedir/etc -a -d $imagedir/modules \)  ]; then
	echo "Creating directory $imagedir..."
	mkdir -p $imagedir/bin
	mkdir $imagedir/lib
	mkdir $imagedir/etc
	mkdir $imagedir/modules

	# If the directories were not all successfully created then it's a bad
	# failure.  Exit with error
	if [ ! \( -d $imagedir/bin -a -d $imagedir/lib -a -d $imagedir/etc -a $imagedir/modules \) ]; then
		echo "Image directory at $imagedir is incomplete.  Could not create."
		exit 1
	fi
fi

if [ ! -d $targetmodeldir ]; then
	echo "Target model directory at $targetmodeldir does not exist"
	exit 1
fi

# Populate image directory

echo "Populating generic blade image at $imagedir with third party"
echo "prerequisites... "

if [ $CROSS_BUILD ]; then
	# If this is a cross build, use prerequisite files from the toolchain
	echo "Cross build detected, using prerequisites from $CROSS_BUILD toolchain"

	toolchaindir=$CL_BUILDTOOLS/$CROSS_BUILD
	if [ ! \( "$toolchaindir" != "/" -a -d $toolchaindir \) ]; then
		echo "Tool chain directory: $toolchaindir is not a proper directory"
		exit 1
	fi
	
	cd $toolchaindir

	# There may be multiple problems with prerequisites.  Keep track of the
	# problems and tell about them at the end.  If a problem was observed
	# then exit at the end.  I think that this is slightly less yucky than
	# spreading lots of if tests through the code.
	declare -a res_array
	declare -a op_array
	set -o pipefail
	# db
	echo -n "db "
	tar cfh - lib/libdb* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in db"

	# gdbm
	echo -n "gdbm "
	tar cfh - lib/libgdbm* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gdbm"

	# net-snmp
	echo -n "net-snmp "
	install $exe_flags $toolchaindir/sbin/snmp* $imagedir/bin
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in install snmp binaries"
	tar cfh - lib/libnetsnmp* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in libnetsnmp"
	tar cfh - share/snmp | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in db"
	if [ $(ls -1 lib/libstdc++.so.5.0.5 2>/dev/null | wc -l) -gt 0 ]; then
		tar cfh - lib/libstdc++.so.5* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in libstdc++"
	fi
	if [ $(ls -1 lib/libcrypto*.* 2>/dev/null | wc -l) -gt 0 ]; then
		tar cfh - lib/libcrypto*.* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in libcrypto"
	fi
	if [ $(ls -1 lib/libssl*.* 2>/dev/null | wc -l) -gt 0 ]; then
		tar cfh - lib/libssl* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in libssl"
	fi

	# openhpi
	echo "openhpi "
	install $exe_flags $toolchaindir/sbin/openhpid $imagedir/bin
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="install openhpid"
	install $exe_flags $toolchaindir/bin/*hpi* $imagedir/bin
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="install hpi binaries"
	tar cfh - lib/*hpi* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in hpi libraries"
	tar cfh - lib/*oht* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in hpi libraries"
	tar cfh - lib/*ohu* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in hpi libraries"
	tar cfh - lib/*glib* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in glib libraries"
	tar cfh - lib/*gmodule* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gmodule library files"
	tar cfh - lib/*gobject* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gobject library files"
	tar cfh - lib/*gthread* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gthread library files"

	# libhcl
	echo -n "hcl "
	if [ -f $toolchaindir/lib/libhcl.so ]; then
		cd $toolchaindir
		tar chf - lib/*hcl* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in hcl libraries"
		cd - >/dev/null 2>&1
	fi
	if [ -f $toolchaindir/bin/hpiapp ]; then
		install $exe_flags $toolchaindir/bin/hpiapp $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install hpiapp"
	fi
	if [ -f $toolchaindir/bin/saHpiAlarmAdd ] ;then
		install $exe_flags $toolchaindir/bin/saHpiAlarmAdd $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install saHpiAlarmAdd"
	fi


	#libw3c
	if [ $(ls -1 $toolchaindir/lib/libwww*.* 2>/dev/null| wc -l) -gt 0 ]; then
		echo -n "w3c-libwww "
		install $exe_flags $toolchaindir/bin/libwww-config $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install w3c-libwww binaries"
		install $exe_flags $toolchaindir/bin/w3c $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install w3c-libwww binaries"
		install $exe_flags $toolchaindir/bin/webbot $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww binaries"
		install $exe_flags $toolchaindir/bin/www $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww binaries"
		tar cfh - lib/libwww*.* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		tar cfh - lib/libmd5*.* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		tar cfh - lib/libpics*.* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		tar cfh - lib/libxmlparse*.* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		tar cfh - lib/libxmltok*.* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
	fi
	#xmlrpc
	if [ $(ls -1 $toolchaindir/lib/libxmlrpc*.* 2>/dev/null| wc -l) -gt 0 ]; then
		echo -n "xmlrpc "
		install $exe_flags $toolchaindir/bin/xmlrpc $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		install $exe_flags $toolchaindir/bin/xmlrpc-c-config $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		install $exe_flags $toolchaindir/bin/xmlrpc_transport $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		tar cfh - lib/libxmlrpc*.* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc libraries"
	fi

	cd - >/dev/null 2>&1
	i=0
	res=0
	while [ $i -lt ${#res_array[@]} ]; do
		if [ ${res_array[$i]} -ne 0 ]; then
			echo "Failure detected while trying to ${op_array[$i]}."
			res=1
		fi
		i=$(expr $i + 1)
	done
	if [ $res != 0 ]; then
		exit 1
	fi

else
	echo "Installing prerequisites from local system"

	toolchaindir=$CL_BUILDTOOLS/local

	# There may be multiple problems with prerequisites.  Keep track of the
	# problems and tell about them at the end.  If a problem was observed
	# then exit at the end.  I think that this is slightly less yucky than
	# spreading lots of if tests through the code.
	declare -a res_array
	declare -a op_array
	set -o pipefail

	# db
	echo -n "db "
	if [ -f $toolchaindir/lib/libdb.so ]; then
		cd $toolchaindir
		tar cfh - lib/libdb* | tar xf - -C $imagedir
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in db"
		cd - >/dev/null 2>&1
	else
		cd /usr/lib
		tar cfh - libdb[.-]* | tar xf - -C $imagedir/lib
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in db"
		cd - >/dev/null 2>&1
	fi

	# gdbm
	echo -n "gdbm "
	if [ -f $toolchaindir/lib/libgdbm.so ]; then
		cd $toolchaindir
		tar cfh - lib/libgdbm* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in gdbm"
		cd - >/dev/null 2>&1
	else
		cd /usr/lib
		tar cfh - libgdbm.* | tar xf - -C $imagedir/lib
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in gdbm"
		cd - >/dev/null 2>&1
	fi

	# net-snmp
	echo -n "net-snmp "
	if [ -f $toolchaindir/lib/libnetsnmp.so ]; then
		cd $toolchaindir
		install $exe_flags $toolchaindir/sbin/snmp* $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="instll net-snmp"
		tar cfh - lib/libnetsnmp* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in libnetsnmp"
		tar cfh - share/snmp | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in snmp"
		if [ $(ls -1 lib/libstdc++.so.5.0.5 2>/dev/null | wc -l) -gt 0 ]; then
			tar cfh - lib/libstdc++.so.5* | tar xf - -C $imagedir
			res_array[${#res_array[@]}]=$?
			op_array[${#op_array[@]}]="copy in libstdc++"
		fi
		if [ $(ls -1 lib/libcrypto*.* 2>/dev/null | wc -l) -gt 0 ]; then
			tar cfh - lib/libcrypto*.* | tar xf - -C $imagedir
			res_array[${#res_array[@]}]=$?
			op_array[${#op_array[@]}]="copy in libcrypto"
		fi
		if [ $(ls -1 lib/libssl*.* 2>/dev/null | wc -l) -gt 0 ]; then
			tar cfh - lib/libssl*.* | tar xf - -C $imagedir
			res_array[${#res_array[@]}]=$?
			op_array[${#op_array[@]}]="copy in libsl"
		fi
		cd - >/dev/null 2>&1
	else
		cd $(net-snmp-config --prefix)
		install $exe_flags sbin/snmp* $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install net-snmp"
		tar cfh - lib/libnetsnmp* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in libnetsnmp"
		tar cfh - share/snmp | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in snmp"
		if [ $(ls -1 lib/libstdc++.so.5.0.5 2>/dev/null | wc -l) -gt 0 ]; then
			tar cfh - lib/libstdc++.so.5* | tar xf - -C $imagedir
			res_array[${#res_array[@]}]=$?
			op_array[${#op_array[@]}]="copy in libstdc++"
		fi
		if [ $(ls -1 lib/libcrypto*.* 2>/dev/null | wc -l) -gt 0 ]; then
			tar cfh - lib/libcrypto*.* | tar xf - -C $imagedir
			res_array[${#res_array[@]}]=$?
			op_array[${#op_array[@]}]="copy in libcrypto"
		fi
		if [ $(ls -1 lib/libssl*.* 2>/dev/null | wc -l) -gt 0 ]; then
			tar chf - lib/libssl* | tar xf - -C $imagedir
			res_array[${#res_array[@]}]=$?
			op_array[${#op_array[@]}]="copy in libssl"
		fi
		cd - >/dev/null 2>&1
	fi

	# openhpi
	echo -n "openhpi "
	if [ -f $toolchaindir/lib/libopenhpi.a ]; then
		cd $toolchaindir
		if [ -f $toolchaindir/sbin/openhpid ]; then
			install $exe_flags $toolchaindir/sbin/openhpid $imagedir/bin
			res_array[${#res_array[@]}]=$?
			op_array[${#op_array[@]}]="install openhpid"
		fi
		install $exe_flags $toolchaindir/bin/*hpi* $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install openhpi"
		tar chf - lib/*hpi* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in openhpi libraries"
		tar cfh - lib/*oht* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in hpi libraries"
		tar cfh - lib/*ohu* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in hpi libraries"
		cd - >/dev/null 2>&1
	else
		if [ -f /usr/local/sbin/openhpid ]; then
			cd /usr/local
		else
			cd /usr
		fi
		if [ -f sbin/openhpid ]; then
			install $exe_flags sbin/openhpid $imagedir/bin
			res_array[${#res_array[@]}]=$?
			op_array[${#op_array[@]}]="install openhpid"
		fi
		install $exe_flags bin/*hpi* $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install openhpi binaries"
		tar chf - lib/*hpi* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in openhpi libraries"
		tar cfh - lib/*oht* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in hpi libraries"
		tar cfh - lib/*ohu* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in hpi libraries"
		cd - >/dev/null 2>&1
	fi
	# libhcl
	echo -n "hcl "
	if [ -f $toolchaindir/lib/libhcl.so ]; then
		cd $toolchaindir
		tar chf - lib/*hcl* | tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="copy in hcl libraries"
		cd - >/dev/null 2>&1
	fi
	if [ -f $toolchaindir/bin/hpiapp ]; then
		install $exe_flags $toolchaindir/bin/hpiapp $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install hpiapp"
	fi
	if [ -f $toolchaindir/bin/saHpiAlarmAdd ] ;then
		install $exe_flags $toolchaindir/bin/saHpiAlarmAdd $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install saHpiAlarmAdd"
	fi

	echo -n "glib-2.0 "
	export PKG_CONFIG_PATH=${toolchaindir}/lib/pkgconfig:$PKG_CONFIG_PATH
	GLIB_LIB_DIR=$(pkg-config --libs-only-L glib-2.0 | sed -e 's/^.*-L//g')
	if [ -z ${GLIB_LIB_DIR} ]; then # glib-2.0 is installed in the system standard path
		GLIB_LIB_DIR="/usr/lib"
        fi
	GLIB_DIR=$(dirname ${GLIB_LIB_DIR})
	if [ -z ${GLIB_DIR}  -o ! -d ${GLIB_DIR} ]
	then
		echo ""
		echo "Error: Cannot find glib-2.0 directory."
		exit 1
	fi
	cd ${GLIB_DIR}; #echo $GLIB_DIR
	tar chf - lib/*glib* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in glib libraries"
	tar chf - lib/*gmodule* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gmodule library files"
	tar chf - lib/*gobject* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gobject library files"
	tar chf - lib/*gthread* | tar xf - -C $imagedir
	res_array[${#res_array[@]}]=$?
	op_array[${#op_array[@]}]="copy in gthread library files"
	cd - >/dev/null 2>&1
	#libw3c
	if [ $(ls -1 $toolchaindir/lib/libwww*.* 2>/dev/null | wc -l) -gt 0 ]; then
		echo -n "w3c-libwww "
		[ -f $toolchaindir/bin/libwww-config ] && install $exe_flags $toolchaindir/bin/libwww-config $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install w3c-libwww binaries"
		[ -f $toolchaindir/bin/w3c ] && install $exe_flags $toolchaindir/bin/w3c $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="install w3c-libwww binaries"
		[ -f $toolchaindir/bin/webbot ] && install $exe_flags $toolchaindir/bin/webbot $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww binaries"
		[ -f $toolchaindir/bin/www ] && install $exe_flags $toolchaindir/bin/www $imagedir/bin
		cd $toolchaindir >/dev/null 2>&1
	       	op_array[${#op_array[@]}]="install w3c-libwww binaries"
		tar chf - lib/libwww*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		tar chf - lib/libmd5*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		tar chf - lib/libpics*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		tar chf - lib/libxmlparse*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		tar chf - lib/libxmltok*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install w3c-libwww libraries"
		cd - >/dev/null 2>&1
	else
		echo -n "w3c-libwww "
		TOOLCHAINDIR=$(libwww-config --prefix)	
		[ -f $TOOLCHAINDIR/bin/libwww-config ] && install $exe_flags $TOOLCHAINDIR/bin/libwww-config $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="libwww-config install w3c-libwww binaries"
		[ -f $TOOLCHAINDIR/bin/w3c ] && install $exe_flags $TOOLCHAINDIR/bin/w3c $imagedir/bin
		res_array[${#res_array[@]}]=$?
		op_array[${#op_array[@]}]="w3c install w3c-libwww binaries"
		[ -f $TOOLCHAINDIR/bin/webbot ] && install $exe_flags $TOOLCHAINDIR/bin/webbot $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="webbot install w3c-libwww binaries"
		[ -f $TOOLCHAINDIR/bin/www ] && install $exe_flags $TOOLCHAINDIR/bin/www $imagedir/bin
		cd $TOOLCHAINDIR >/dev/null 2>&1
		tar chf - lib/libwww*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="libwww install w3c-libwww libraries"
		tar chf - lib/libmd5*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="md5 install w3c-libwww libraries"
		tar chf - lib/libxmlparse*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="xmlparse install w3c-libwww libraries"
		tar chf - lib/libxmltok*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="xmltok install w3c-libwww libraries"
		tar chf - lib/libpics*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="libpics install w3c-libwww libraries"
		cd - >/dev/null 2>&1
	fi
	#xmlrpc
	if [ $(ls -1 $toolchaindir/lib/libxmlrpc*.* | wc -l) -gt 0 ]; then
		echo -n "xmlrpc "
		[ -f $toolchaindir/bin/xmlrpc ] && install $exe_flags $toolchaindir/bin/xmlrpc $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		[ -f $toolchaindir/bin/xmlrpc-c-config ] && install $exe_flags $toolchaindir/bin/xmlrpc-c-config $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		[ -f $toolchaindir/bin/xmlrpc_transport ] && install $exe_flags $toolchaindir/bin/xmlrpc_transport $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		cd $toolchaindir >/dev/null 2>&1
		tar chf - lib/libxmlrpc*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc libraries"
		cd - >/dev/null 2>&1
	else
		echo -n "xmlrpc "
		TOOLCHAINDIR=$(xmlrpc-c-config --prefix)
		[ -f $TOOLCHAINDIR/bin/xmlrpc ] && install $exe_flags $TOOLCHAINDIR/bin/xmlrpc $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		[ -f $TOOLCHAINDIR/bin/xmlrpc-c-config ] && install $exe_flags $TOOLCHAINDIR/bin/xmlrpc-c-config $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		[ -f $TOOLCHAINDIR/bin/xmlrpc_transport ] && install $exe_flags $TOOLCHAINDIR/bin/xmlrpc_transport $imagedir/bin
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc binaries"
		cd $TOOLCHAINDIR >/dev/null 2>&1
		tar chf - lib/libxmlrpc*.* 2>/dev/null| tar xf - -C $imagedir
		res_array[${#res_array[@]}]=$?
	       	op_array[${#op_array[@]}]="install xmlrpc libraries"
		cd - >/dev/null 2>&1
	fi

	i=0
	res=0
	while [ $i -lt ${#res_array[@]} ]; do
		if [ ${res_array[$i]} -ne 0 ]; then
			echo "Failure detected while trying to ${op_array[$i]}."
			res=1
		fi
		i=$(expr $i + 1)
	done
	if [ $res != 0 ]; then
		exit 1
	fi

fi

echo " done"
exit 0
