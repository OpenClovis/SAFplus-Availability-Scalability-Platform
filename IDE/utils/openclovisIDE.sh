#!/bin/sh
#$Id$
#$Header$
script_path=`which $0`
PRODCUT_NAME=ClovisWorks
APPLICATION_NAME=org.eclipse.ui.ide.workbench
PERSPECTIVE=com.clovis.cw.ui.CWPerspective
CMD_ARGS="-application $APPLICATION_NAME -perspective $PERSPECTIVE -configuration . "
exec ../eclipse $CMD_ARGS $@
