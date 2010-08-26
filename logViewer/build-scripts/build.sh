################################################################################
# ModuleName  : build
# $File$
# $Author$
# $Date$
################################################################################
# Description :
################################################################################
#!/bin/bash


echo "---- LogTool build starting ----"
#Clear environment.
#unset ANT_HOME
#unset JAVA_HOME
#Check for ant
#source ./env.sh
echo $ANT_HOME
ANT="$ANT_HOME/bin/ant -Dos=linux"
echo "Ant is:" $ANT
#Change path to take new binaries.
export PATH=$JAVA_HOME/bin:$ANT_HOME/bin:$PATH
echo "Pah is:" $PATH
echo "Building LogTool"

$ANT -f build.xml clean || exit 1

$ANT -f build.xml dist || exit 1

