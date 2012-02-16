################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python boottime.py <compXmi> <proj location> <gen location>
# Copies files from boottime to genere location. To maintain uniqueness
# of names, NodeName is prefixed in each file.
################################################################################
import os
import sys
import string
import shutil
import xml.dom.minidom
from string import Template


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Script Execution Starts Here.
componentXml  = sys.argv[1]
projLocation  = sys.argv[2]
genLocation   = sys.argv[3]
componentDoc  = xml.dom.minidom.parse(componentXml)
componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
nodeList      = componentInfoObj[0].getElementsByTagName("node");
for node in nodeList:
    nodeName  = node.attributes["name"].value
    bootdir   = os.path.join(projLocation, nodeName);
    bootdir   = os.path.join(bootdir, "aspcomponents");
    bootdir   = os.path.join(bootdir, "boottime");
    files     = os.listdir(bootdir)
    for file in files:
        src   = os.path.join(bootdir, file)
        shutil.copy2(src, os.path.join(genLocation, nodeName + "_" + file))
