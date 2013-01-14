################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python makefile.py
# create Makefile
################################################################################

import os
import string
from templates import configMakeTemplate

def mainMethod(srcDir):
	makeFile = os.path.join(srcDir, "config/Makefile")
	out_file = open(makeFile, "w")
	out_file.write(configMakeTemplate.makefileTemplate.safe_substitute())
	out_file.close()

