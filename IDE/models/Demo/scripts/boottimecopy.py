################################################################################
# ModuleName  : Demo
# $File$
# $Author$
# $Date$
################################################################################
# Description :
################################################################################
# boottime.py
# usage: python boottime.py <compXmi> <proj location> <gen location>

# Copies files from boottime to genere location. To maintain uniqueness
# of names, NodeName is prefixed in each file.
#
# Copyright (c) 2005 Clovis Solutions
#
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
nodeList      = componentDoc.getElementsByTagName("component:Node");
for node in nodeList:
    nodeName  = node.attributes["Name"].value
    bootdir   = os.path.join(projLocation, nodeName);
    bootdir   = os.path.join(bootdir, "aspcomponents");
    bootdir   = os.path.join(bootdir, "boottime");
    files     = os.listdir(bootdir)
    for file in files:
        src   = os.path.join(bootdir, file)
        shutil.copy2(src, os.path.join(genLocation, nodeName + "_" + file))
