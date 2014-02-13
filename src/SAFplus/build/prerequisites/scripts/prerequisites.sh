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
# Populate platform image with third party prerequisite binaries, libraries,
# config files, etc. from the local system or toolchain as necessary.
##############################################################################

MACH=`uname -m`
populate_prereqs() {
    if [ $# -ne 5 ]
    then
        echo "Usage: populate_image source copyreq target arch system buildtools"
        return 1
    fi
    BUILDTOOL_DIR=$1
    COPY_PREREQUISITES=$2
    TARGET_MODEL=$3
    ARCH=$4
    SYS=$5

    # verify target images directory

    targetmodeldir=$PROJECT_ROOT/target/$ASP_MODEL_NAME
    if [ ! -d $targetmodeldir/images ]
    then
        echo "Images directory does not exist.  It really should by now."
        echo "Images directory = ${targetmodeldir}/images"
        exit 1
    fi

    #
    # verify and create target images directory
    # ideally, at this point, the directory structure under $imagedir should
    # already exist.  however, if it doesn't (in case this is invoked out of
    # order manually), create the tree automatically.
    imagedir=$targetmodeldir/images/${ARCH}/${SYS}
#    if [ ! \( -d $imagedir/bin -a -d $imagedir/lib -a -d $imagedir/etc -a -d $imagedir/modules \)  ]; then
#        echo "Creating directory $imagedir..."
#        mkdir -p $imagedir/bin
#        mkdir $imagedir/lib
#        mkdir $imagedir/etc
#        mkdir $imagedir/modules

        # If the directories were not all successfully created then it's a bad
        # failure.  Exit with error
#        if [ ! \( -d $imagedir/bin -a -d $imagedir/lib -a -d $imagedir/etc -a $imagedir/modules \) ]; then
#            echo "Image directory at $imagedir is incomplete.  Could not create."
#            exit 1
#        fi
#    fi
    if [ ! -d ${imagedir} ]
    then
        echo "Target image directory does not exist.  It should exist by now."
        echo "target image = ${imagedir}"
        exit 1
    fi
    if [ ! \( -d $imagedir/bin -a -d $imagedir/lib -a -d $imagedir/etc -a -d $imagedir/modules \)  ]
    then
        echo "We don't have a full complement (bin, lib, etc, and modules)"
        echo "of directories in the target imagedir."
        echo "The target imagedir is ${imagedir}."
        exit 1
    fi

    # For debugging
#    echo "Prerequisite population config:"
#    echo "  target model = ${TARGET_MODEL}"
#    echo "  ARCH = ${ARCH}"
#    echo "  SYS = ${SYS}"
#    echo "  BUILDTOOL_DIR = ${BUILDTOOL_DIR}"
#    echo "  targetmodeldir = ${targetmodeldir}"
#    echo "  imagedir = ${imagedir}"

#echo "CROSS_BUILD = ${CROSS_BUILD}"

    # Populate image directory

    echo ""
    echo "*** Populating platform specific blade image at $imagedir with third party prerequisites... "

    if [ ! "$BUILDTOOL_DIR" = "local" -a "${COPY_PREREQUISITES}" = "true" ]; then
        # If this is a cross build, use prerequisite files from the toolchain
        echo "Cross build detected, using prerequisites from $BUILDTOOL_DIR toolchain"

        toolchaindir=$TOOLCHAIN_DIR
        if [ ! \( "$toolchaindir" != "/" -a -d $toolchaindir \) ]; then
            echo "Tool chain directory: $toolchaindir is not a proper directory...skipping third party prerequisites"
            
        else
        
        cd $toolchaindir
        source config.mk
        
        wrstoolchain=0
        if [ -d local ]; then
            wrstoolchain=1
        fi

        mvtoolchain=0
        if [ "$WIND_VER" = "0" ]; then
            mvtoolchain=1
        fi

        # There may be multiple problems with prerequisites.  Keep track of the
        # problems and tell about them at the end.  If a problem was observed
        # then exit at the end.  I think that this is slightly less yucky than
        # spreading lots of if tests through the code.
        declare -a res_array
        declare -a op_array
        set -o pipefail
        # db
        echo -n "db "
        ls lib/libdb* > /dev/null 2> /dev/null
        if [ $? -eq 0 ]; then
            tar cfh - lib/libdb* | tar xf - -C $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in db"
        else
            cd $TARGET
            tar cfh - lib/libdb* | tar xf - -C $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in db"
            cd ..
        fi

        # sqlite3
        echo -n "sqlite3 "
        if [ $wrstoolchain = 1 ]; then
            cd local
            tar cfh - lib/libsqlite* | tar xf - -C $imagedir
            cd ..
        else
            tar cfh - lib/libsqlite* | tar xf - -C $imagedir
        fi
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in sqlite3"

        # gdbm
        echo -n "gdbm "
        tar cfh - lib/libgdbm* | tar xf - -C $imagedir
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in gdbm"

        if [ $SNMP_BUILD == "1" ]; then
        # net-snmp
        echo -n "net-snmp "
        if [ $wrstoolchain = 1 -a $mvtoolchain = 0 ]; then
            install $exe_flags $toolchaindir/local/sbin/snmp* $imagedir/bin
        elif [ $mvtoolchain = 1 ]; then
            install $exe_flags $toolchaindir/target/sbin/snmp* $imagedir/bin
        else
            install $exe_flags $toolchaindir/sbin/snmp* $imagedir/bin
        fi
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in install snmp binaries"
        if [ $wrstoolchain = 1 -a $mvtoolchain = 0 ]; then
                cd local
                cp -R --parents -L lib/libnetsnmp* $imagedir
                cd ..
        else
            cp -R --parents -L lib/libnetsnmp* $imagedir
        fi
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in libnetsnmp"
        if [ $wrstoolchain = 1 -a $mvtoolchain = 0 ]; then
            cd local
            cp -R --parents -L share/snmp $imagedir
            cd ..
        elif [ $mvtoolchain = 1 ]; then
            cd target
            cp -R --parents -L share/snmp $imagedir
            cd ..
        else
            cp -R --parents -L share/snmp $imagedir
        fi
        fi

        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in db"
        if [ $(ls -1 lib/libcrypto*.* 2>/dev/null | wc -l) -gt 0 ]; then
            cp -R --parents -L lib/libcrypto*.* $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in libcrypto"
        fi
        if [ $(ls -1 lib/libssl*.* 2>/dev/null | wc -l) -gt 0 ]; then
            cp -R --parents -L lib/libssl* $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in libssl"
        fi

        if [ -n "$OPENHPICFLAGS" ]; then
        # openhpi
        echo -n "openhpi "
        if [ $wrstoolchain = 1 -o $mvtoolchain = 1 ]; then
            if [ -f $toolchaindir/local/sbin/openhpid ]; then
                install $exe_flags $toolchaindir/local/sbin/openhpid $imagedir/bin
            fi
        else
            install $exe_flags $toolchaindir/sbin/openhpid $imagedir/bin
        fi
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="install openhpid"
        if [ $wrstoolchain = 1 -o $mvtoolchain = 1 ]; then
            if [ -f $toolchaindir/local/bin/hpi_cmd ]; then
                install $exe_flags $toolchaindir/local/bin/*hpi* $imagedir/bin
            fi
        else
            install $exe_flags $toolchaindir/bin/*hpi* $imagedir/bin
        fi
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="install hpi binaries"
        if [ $wrstoolchain = 1 -o $mvtoolchain = 1 ]; then
            cd local
            if [ -f $toolchaindir/local/lib/libopenhpi.so ]; then
                tar cfh - lib/*hpi* | tar xf - -C $imagedir
                tar cfh - lib/*oh* | tar xf - -C $imagedir
            fi
            cd ..
        else
            tar cfh - lib/*hpi* | tar xf - -C $imagedir
            tar cfh - lib/*oh* | tar xf - -C $imagedir
        fi
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in hpi libraries"
       
        
        tar cfh - lib/*glib* | tar xf - -C $imagedir
        #res_array[${#res_array[@]}]=$?
        #op_array[${#op_array[@]}]="copy in glib libraries"
        
        tar cfh - lib/*gmodule* | tar xf - -C $imagedir
        #res_array[${#res_array[@]}]=$?
        #op_array[${#op_array[@]}]="copy in gmodule library files"
        
        tar cfh - lib/*gobject* | tar xf - -C $imagedir
        #res_array[${#res_array[@]}]=$?
        #op_array[${#op_array[@]}]="copy in gobject library files"
        
        tar cfh - lib/*gthread* | tar xf - -C $imagedir
        #res_array[${#res_array[@]}]=$?
        #op_array[${#op_array[@]}]="copy in gthread library files"
        fi
        # libhcl
        if [ -f $toolchaindir/lib/libhcl.so ]; then
            echo -n "hcl "
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

        # Emerson HPI
        if [ -d $toolchaindir/emerson/lib ]; then
            echo -n "Emerson HPI "
            cp -rf $toolchaindir/emerson/lib/*.so $imagedir/lib
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in Emerson HPI libraries"
        fi

        # python wrapper for openhpi and libhcl
        echo -n "pyhpi "
        if [ -d $toolchaindir/lib/python/site-packages ]; then
            mkdir -p $imagedir/lib/python
            install $installflags  $toolchaindir/lib/python/site-packages/*.py \
                $toolchaindir/lib/python/site-packages/*.pyc \
                $toolchaindir/lib/python/site-packages/*.so \
                $imagedir/lib/python
            if [ $HCL_CM == "1" ]; then
                (cd $imagedir/lib/python; ln -s libhcl.py hpi.py)
            else
                (cd $imagedir/lib/python; ln -s openhpi.py hpi.py)
            fi
        fi

        #libw3c
        if [ $(ls -1 $toolchaindir/lib/libwww*.* 2>/dev/null| wc -l) -gt 0 ]; then
            echo -n "w3c-libwww "
            if [ $mvtoolchain = 0 ]; then
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
            fi
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

        echo ""

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
        fi

    elif [ "$BUILDTOOL_DIR" = "local" ]; then
        echo "Installing prerequisites from local system:"
        echo -n "  "

        toolchaindir=$TOOLCHAIN_DIR
        
        touch $imagedir/local_build

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
        elif [ -f /usr/lib64/libdb.so ]; then
            cd /usr/lib64
            tar cfh - libdb[.-]* | tar xf - -C $imagedir/lib
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in db"
            cd - >/dev/null 2>&1
        elif [ -f /usr/lib/${MACH}-linux-gnu/libdb.so ]; then
            cd /usr/lib/${MACH}-linux-gnu
            tar cfh - libdb[.-]* | tar xf - -C $imagedir/lib
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in db"
            cd - >/dev/null 2>&1
        elif [ -f /usr/lib/`uname -i`-linux-gnu/libdb.so ]; then
            cd /usr/lib/`uname -i`-linux-gnu
            tar cfh - libdb[.-]* | tar xf - -C $imagedir/lib
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in db"
            cd - >/dev/null 2>&1
       elif [ -f /usr/lib/i386-linux-gnu/libdb.so ]; then
            cd /usr/lib/i386-linux-gnu
            tar cfh - libdb[.-]* | tar xf - -C $imagedir/lib
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
        elif [ -f /usr/lib64/libgdbm.so ]; then
            cd /usr/lib64
            tar cfh - libgdbm.* | tar xf - -C $imagedir/lib
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in gdbm"
            cd - >/dev/null 2>&1
        elif [ -f /usr/lib/${MACH}-linux-gnu/libgdbm.so ]; then
            cd /usr/lib/${MACH}-linux-gnu
            tar cfh - libgdbm.* | tar xf - -C $imagedir/lib
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in gdbm"
            cd - >/dev/null 2>&1
        elif [ -f /usr/lib/`uname -i`-linux-gnu/libgdbm.so.3 ]; then
            cd /usr/lib/`uname -i`-linux-gnu
            tar cfh - libgdbm.* | tar xf - -C $imagedir/lib
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in gdbm"
            cd - >/dev/null 2>&1
        elif [ -f /usr/lib/i386-linux-gnu/libgdbm.so.3 ]; then
            cd /usr/lib/i386-linux-gnu
            tar cfh - libgdbm.* | tar xf - -C $imagedir/lib
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
        if [ $SNMP_BUILD == "1" ]; then
        echo -n "net-snmp "
        if [ -f $toolchaindir/lib/libnetsnmp.so ]; then
            cd $toolchaindir
            install $exe_flags $toolchaindir/sbin/snmp* $imagedir/bin
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="install net-snmp"
            cp -R --parents -L lib/libnetsnmp* $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in libnetsnmp"
            cp -R --parents -L share/snmp $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in snmp"
            if [ $(ls -1 lib/libcrypto*.* 2>/dev/null | wc -l) -gt 0 ]; then
                cp -R --parents -L lib/libcrypto*.* $imagedir
                res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="copy in libcrypto"
            fi
            if [ $(ls -1 lib/libssl*.* 2>/dev/null | wc -l) -gt 0 ]; then
                cp -R --parents -L lib/libssl*.* $imagedir
                res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="copy in libsl"
            fi
            cd - >/dev/null 2>&1
        else
            cd $(net-snmp-config --prefix)
            # pwd
            # echo install $exe_flags sbin/snmp* $imagedir/bin
            install $exe_flags sbin/snmp* $imagedir/bin  # May not exist
            install $exe_flags bin/net-s* $imagedir/bin
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="install net-snmp"
            cp -R --parents -L lib/libnetsnmp* $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in libnetsnmp"
            cp -R --parents -L share/snmp $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in snmp"
            if [ $(ls -1 lib/libcrypto*.* 2>/dev/null | wc -l) -gt 0 ]; then
                cp -R --parents -L lib/libcrypto*.* $imagedir
                res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="copy in libcrypto"
            fi
            if [ $(ls -1 lib/libssl*.* 2>/dev/null | wc -l) -gt 0 ]; then
                cp -R --parents -L lib/libssl* $imagedir
                res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="copy in libssl"
            fi
            cd - >/dev/null 2>&1
        fi
        fi

        # openhpi
        if [ -n "$OPENHPICFLAGS" ]; then
        BUILDTOOLS_PATH=$SAFPLUS_BTL_DIR$TOOLCHAIN_DIR
        echo -n "openhpi: trying toolchain build at $BUILDTOOLS_PATH/lib/libopenhpi.a "
        if [ -f $BUILDTOOLS_PATH/lib/libopenhpi.a ]; then
            cd $BUILDTOOLS_PATH
            if [ -f $BUILDTOOLS_PATH/sbin/openhpid ]; then
                install $exe_flags $BUILDTOOLS_PATH/sbin/openhpid $imagedir/bin
                res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="install openhpid"
            fi
            install $exe_flags $BUILDTOOLS_PATH/bin/*hpi* $imagedir/bin
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="install openhpi"
            # there may be no *oh* (openhpi 2.16.0) so don't remember any error from the next line 
            if [ -f lib/*oh* ]; then
              tar chf - lib/*oh* | tar xf - -C $imagedir
            fi
            tar chf - lib/*hpi* | tar xf - -C $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in openhpi libraries"
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
            if [ -f lib64/openhpi.so ]; then
                cd lib64
                tar chf - *hpi* | tar xf - -C $imagedir/lib
                cd - >/dev/null 2>&1
            else
                tar chf - lib/*hpi* | tar xf - -C $imagedir
            fi
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in openhpi libraries"
            cd - >/dev/null 2>&1
        fi
        fi
	
	# tipc
	echo -n " tipc "
	if [ -f $toolchaindir/bin/tipc-config ]; then
        if [ ! -d $imagedir/modules ]; then
            mkdir -p $imagedir/modules
        fi
        if [ ! -d $imagedir/bin ]; then
            mkdir -p $imagedir/bin
        fi
        if [ -f $toolchaindir/bin/tipc-config ]; then
            cp $toolchaindir/bin/tipc-config $imagedir/bin
        fi
        if [ -f $toolchaindir/modules/tipc.ko ]; then
            cp $toolchaindir/modules/tipc.ko $imagedir/modules
        fi
	fi

        if [ $HPI_EMERSON == "1" ]; then
        # Emerson HPI
          if [ -d $toolchaindir/emerson/lib ]; then
            echo -n "Emerson HPI "
            cp -rf $toolchaindir/emerson/lib/*.so $imagedir/lib
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in Emerson HPI libraries"
          else
            echo -n -e "\nWARNING: Emerson HPI libraries not found in $toolchaindir/emerson/lib"
          fi
        fi    

        # libhcl
        if [ -f $toolchaindir/local/lib/libhcl.so ]; then
            echo -n " hcl"
            cd $toolchaindir/local
            tar chf - lib/*hcl* | tar xf - -C $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in hcl libraries"
            cd - >/dev/null 2>&1
        elif [ -f $toolchaindir/lib/libhcl.so ]; then
            cd $toolchaindir
            tar chf - lib/*hcl* | tar xf - -C $imagedir
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="copy in hcl libraries"
            cd - >/dev/null 2>&1
        fi
        if [ -f $toolchaindir/local/bin/hpiapp ]; then
            install $exe_flags $toolchaindir/local/bin/hpiapp $imagedir/bin
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="install hpiapp"
        elif [ -f $toolchaindir/bin/hpiapp ]; then
            install $exe_flags $toolchaindir/bin/hpiapp $imagedir/bin
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="install hpiapp"
        fi
        if [ -f $toolchaindir/local/bin/saHpiAlarmAdd ] ;then
            install $exe_flags $toolchaindir/local/bin/saHpiAlarmAdd $imagedir/bin
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="install saHpiAlarmAdd"
        elif [ -f $toolchaindir/bin/saHpiAlarmAdd ] ;then
            install $exe_flags $toolchaindir/bin/saHpiAlarmAdd $imagedir/bin
            res_array[${#res_array[@]}]=$?
            op_array[${#op_array[@]}]="install saHpiAlarmAdd"
        fi

        echo -n " glib-2.0"
        if [ -f ${toolchaindir}/lib/libglib-2.0.so ]; then
          GLIB_LIB_DIR=${toolchaindir}/lib
        else
          if [ -f /usr/lib/`uname -i`-linux-gnu/libglib-2.0.so ]; then
            GLIB_LIB_DIR=/usr/lib/`uname -i`-linux-gnu
          elif [ -f /usr/lib/${MACH}-linux-gnu/libglib-2.0.so ]; then
            GLIB_LIB_DIR=/usr/lib/${MACH}-linux-gnu
          elif [ -f /usr/lib/i386-linux-gnu/libglib-2.0.so ]; then
            GLIB_LIB_DIR=/usr/lib/i386-linux-gnu
          else
            export PKG_CONFIG_PATH=${toolchaindir}/lib/pkgconfig:$PKG_CONFIG_PATH
            GLIB_LIB_DIR=$(pkg-config --libs-only-L glib-2.0 | sed -e 's/^.*-L//g')
            if [ -z ${GLIB_LIB_DIR} ]; then # glib-2.0 is installed in the system standard path
              if [ -f /usr/lib64/libglib-2.0.so ]; then
                GLIB_LIB_DIR="/usr/lib64/"
              else
                GLIB_LIB_DIR="/usr/lib/"
              fi
            fi
          fi
        fi

        GLIB_DIR=${GLIB_LIB_DIR}
        if [ -z "${GLIB_LIB_DIR}" -o -z "${GLIB_DIR}"  -o ! -d ${GLIB_DIR} ]
        then
            echo ""
            echo "Error: Cannot find glib-2.0 directory."
            echo "pkg-config reports \"${GLIB_LIB_DIR}\""
            exit 1
        fi
        cd ${GLIB_DIR}; #echo $GLIB_DIR

        tar chf - *glib* | tar xf - -C $imagedir/lib
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in glib libraries"
        
        tar chf - *gmodule* | tar xf - -C $imagedir/lib
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in gmodule library files"

        tar chf - *gobject* | tar xf - -C $imagedir/lib
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in gobject library files"

        tar chf - *gthread* | tar xf - -C $imagedir/lib
        res_array[${#res_array[@]}]=$?
        op_array[${#op_array[@]}]="copy in gthread library files"

        cd - >/dev/null 2>&1

        #libw3c
        if [ $(ls -1 $toolchaindir/lib/libwww*.* 2>/dev/null | wc -l) -gt 0 ]; then
            echo "w3c-libwww "
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
        fi
        #xmlrpc
        if [ $(ls -1 $toolchaindir/lib/libxmlrpc*.* 2> /dev/null | wc -l) -gt 0 ]; then
            echo "xmlrpc "
            install $exe_flags $toolchaindir/bin/xmlrpc $imagedir/bin
            res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="install xmlrpc binaries"
            install $exe_flags $toolchaindir/bin/xmlrpc-c-config $imagedir/bin
            res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="install xmlrpc binaries"
            install $exe_flags $toolchaindir/bin/xmlrpc_transport $imagedir/bin
            res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="install xmlrpc binaries"
            tar chf - lib/libxmlrpc*.* 2>/dev/null| tar xf - -C $imagedir
            res_array[${#res_array[@]}]=$?
                op_array[${#op_array[@]}]="install xmlrpc libraries"
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
    else
        echo "Not Installing prerequisites."
        echo -n "  "
    fi
}

export MODEL_TARGET_ROOT=$(dirname $(dirname $MODEL_TARGET))

#
# Look to the MODEL_PATH.  There should be such a directory.  Look in
# the ${MODEL_TARGET_ROOT} directory (which should also be present)
if [ ! -d ${MODEL_TARGET_ROOT} ]
then
    echo "There is no directory: ${MODEL_TARGET_ROOT}"
    exit 1
fi

# Source our project area config file and target.conf

source $MODEL_PATH/target.conf

# don't continue if we don't need to
if [ "$INSTALL_PREREQUISITES" = "NO" ]; then
    echo "Skipping prerequisites."
    exit 0
fi

# Setting up and deriving required variables 

export installflags='-m 644'
export exe_flags='-m 755'

# env | sort


# In the MODEL_PATH directory there should be a number of architecture 
# directories  .For each architecture directory look for subdirectories.
# Those subdirectories are the "system" directories.  Invoke the
# populate_prereqs function for each architecture/system directory
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

                # If there is no build_label file then just skip.
                if [ ! -f ${sys}/build_label ]
                then
                    echo "No build label file in ${sys}"
                    echo "skipping..."

                # If there is no build_label information in the build_label
                # file then something must have gone wrong.  This will cause
                # problems when copying prerequisite programs and libraries.
                # Bail out here.
                elif [ ! -s ${sys}/build_label ]
                then
                    echo "No build label information in ${sys}/build_label"
                    echo "This is an error, aborting..."
                    exit 1

                # Now, it would be really nice if we could do some
                # extra validation on the SYS and ARCH names.  We'll
                # settle for looking for directories in the ${sys}
                # directory
                elif [ -d ${sys}/bin -a -d ${sys}/lib ]
                then
                    source ${sys}/build_label
                    populate_prereqs "${BUILD_LABEL}" "${COPY_PREREQUISITES}" "${PROJECT_ROOT}/target/${ASP_MODEL_NAME}" "${ARCH}" "${SYS}"
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
echo "  done"
exit 0
