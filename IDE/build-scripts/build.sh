#!/bin/bash


echo "---- ClovisWorks build starting ----"
#Clear environment.
unset ANT_HOME
unset JAVA_HOME
unset ECLIPSE_HOME
#Check for ant
cd `dirname $0`/..
source build-scripts/cwenv.sh
#OS_NAME=$3
ANT="$ANT_HOME/bin/ant -Declipse.home=$ECLIPSE_HOME -Dos=linux"
#Change path to take new binaries.
export PATH=$JAVA_HOME/bin:$ANT_HOME/bin:$PATH

echo "Building ClovisWorks version=$1"

#Build each plugin.
plugins_list="
com.clovis.common.utils
com.clovis.cw.data
com.clovis.cw.genericeditor
com.clovis.cw.editor.ca
com.clovis.cw.licensing
com.clovis.cw.workspace
com.clovis.cw.cmdtool
com.clovis.cw.help
"
$ANT -f build-scripts/build.xml clean || exit 1

for plugin in $plugins_list
do
    echo "Plugin : $plugin"
    $ANT -f build-scripts/build.xml -Dplugin_name=$plugin -Dversion=$1 || exit 1
done

$ANT -f build-scripts/build.xml update -Dplugin_name=$plugin -Dversion=$1 || exit 1
#Build the Release Binary
target=dist

$ANT -f build-scripts/build.xml $target -Dsample_models=$2 -Dversion=$1 -Dversion=$1
