#!/bin/bash

# kid-evn.sh is a very useful shell script to setup a testing environment
# for Kid development and testing.
# $Author: dstanek $
# $Rev: 435 $
# $Date: 2006-11-26 04:50:31 -0500 (Sun, 26 Nov 2006) $"


KIDHOME=$HOME/src/kid
PY23SRC=$KIDHOME/python/Python-2.3.5.tar.bz2
PY23DIST=http://www.python.org/ftp/python/2.3.6/Python-2.3.6.tar.bz2
PY24SRC=$KIDHOME/python/Python-2.4.3.tar.bz2
PY24DIST=http://www.python.org/ftp/python/2.4.4/Python-2.4.4.tar.bz2
PY25SRC=$KIDHOME/python/Python-2.5.tar.bz2
PY25DIST=http://www.python.org/ftp/python/2.5/Python-2.5.tar.bz2
PATH=$KIDHOME/bin:${PATH}
EZSETUP=$KIDHOME/bin/ez_setup.py
EZSETUPDIST=http://peak.telecommunity.com/dist/ez_setup.py

function install_python {
	DIST_PATH=`dirname ${1}`
	DIST_FILE=`basename ${1}`
    VERSION=`echo ${1} | sed -s 's/.*Python-\(.\..\).*/\1/'`
	(
	    cd $DIST_PATH
	    cd `tar vxjf $DIST_FILE | tail -n 1 | sed -s 's/\(\.*\)\/.*/\1/'`

 	    ./configure --prefix=$KIDHOME
	    make
	    make install
	)
    python$VERSION $EZSETUP
    easy_install-$VERSION setuptools
    easy_install-$VERSION docutils
    easy_install-$VERSION elementtree
    easy_install-$VERSION celementtree
}

echo "Setting up ${KIDHOME}"
[ ! -e $KIDHOME ] && mkdir $KIDHOME
[ ! -e $KIDHOME/python ] && mkdir $KIDHOME/python
[ ! -e $KIDHOME/bin ] && mkdir $KIDHOME/bin
[ ! -e $EZSETUP ] && wget -O $EZSETUP $EZSETUPDIST

#echo "Installing Python2.3 in ${KIDHOME}"
#[ ! -e $PY23SRC ] && wget -O $PY23SRC $PY23DIST
#install_python $PY23SRC

#echo "Installing Python2.4 in ${KIDHOME}"
#[ ! -e $PY24SRC ] && wget -O $PY24SRC $PY24DIST
#install_python $PY24SRC

echo "Installing Python2.5 in ${KIDHOME}"
[ ! -e $PY25SRC ] && wget -O $PY25SRC $PY25DIST
install_python $PY25SRC

echo "Setting up the release environment"
easy_install-2.5 http://lesscode.org/svn/pudge/trunk
easy_install-2.5 http://lesscode.org/svn/buildutils/trunk

#install_python $PY23
#python2.3 ${EZSETUP}
#easy_install-2.3 setuptools
#easy_install-2.3 docutils
##easy_install-2.3 kid

#install_python $PY25
#python2.5 ${EZSETUP}
#easy_install-2.5 setuptools
#easy_install-2.5 docutils
##easy_install-2.5 kid

#install_python $PY24
#python2.4 ${EZSETUP}
#easy_install-2.4 setuptools
#easy_install-2.4 docutils
##easy_install-2.4 kid
#easy_install-2.4 http://lesscode.org/svn/pudge/trunk
#easy_install-2.4 http://lesscode.org/svn/buildutils/trunk
