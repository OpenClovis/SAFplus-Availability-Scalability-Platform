export LOGTOOLPATH=$PWD
export BUILDTOOL=$(dirname $(dirname $LOGTOOLPATH))/buildtools/local
if [ -d $BUILDTOOL/jre1.5.0_03 ]; then
    export PATH=$BUILDTOOL/jre1.5.0_03/bin:$PATH
fi
export CLASSPATH=$CLASSPATH:$LOGTOOLPATH/lib/swt.jar:$LOGTOOLPATH/lib/org.eclipse.jface_3.2.0.I20060605-1400.jar:$LOGTOOLPATH/lib/org.eclipse.core.commands_3.2.0.I20060605-1400.jar:$LOGTOOLPATH/lib/org.eclipse.equinox.common_3.2.0.v20060603.jar:$LOGTOOLPATH/lib/castor-0.9.6.jar:$LOGTOOLPATH/lib/commons-logging-api.jar:$LOGTOOLPATH/lib/xerces-J_1.4.0.jar:$LOGTOOLPATH/lib/LogTool.jar
cd $LOGTOOLPATH
java com.clovis.logtool.ui.LogDisplay
