#!/bin/sh 

ECLIPSE_SDK_DIR=$1
CDT_SRC_DIR=$2

$ECLIPSE_SDK_DIR/eclipse -nosplash -application org.eclipse.equinox.p2.director \
 -repository file:$CDT_SRC_DIR -destination $ECLIPSE_SDK_DIR \
 -installIU org.eclipse.cdt.feature.group \
 -installIU org.eclipse.cdt.autotools.feature.group \
 -installIU org.eclipse.cdt.gdb.feature.group \
 -installIU org.eclipse.cdt.gnu.multicorevisualizer.feature.group \
 -installIU org.eclipse.cdt.util.feature.group \
 -installIU org.eclipse.cdt.gnu.debug.feature.group \
 -installIU org.eclipse.cdt.sdk.feature.group \
 -installIU org.eclipse.cdt.core.parser.upc.sdk.feature.group \
 -installIU org.eclipse.cdt.testsrunner.feature.feature.group \
 -installIU org.eclipse.cdt.bupc.feature.group \
 -installIU org.eclipse.cdt.msw.feature.group \
 -installIU org.eclipse.cdt.core.lrparser.feature.feature.group \
 -installIU org.eclipse.cdt.build.crossgcc.feature.group \
 -installIU org.eclipse.cdt.examples.dsf.feature.group \
 -installIU org.eclipse.cdt.gnu.build.feature.group \
 -installIU org.eclipse.cdt.core.parser.upc.feature.feature.group \
 -installIU org.eclipse.cdt.debug.gdbjtag.feature.group \
 -installIU org.eclipse.cdt.visualizer.feature.group \
 -installIU org.eclipse.cdt.xlc.sdk.feature.group \
 -installIU org.eclipse.cdt.xlc.feature.feature.group \
 -installIU org.eclipse.cdt.feature.group \
 -installIU org.eclipse.cdt.autotools.feature.group \
 -installIU org.eclipse.cdt.platform.feature.group
