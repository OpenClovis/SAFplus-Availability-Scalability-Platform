import sys
import xml.dom.minidom
import os
import os.path

myDir = os.path.dirname(__file__)
if not myDir: myDir = os.getcwd()
  
print "me: %s, My DIR: %s" % (__file__,myDir)
sys.path.append("%s/../common" % myDir)

import common
import microdom
import xmlutils

from scripts import openSAFMain


def main(projectLoc,pkgLoc,dataTypeXmi,codeGenMode,srcDir,mibCompiler,mibsPath):
  modelsDir = os.path.join(projectLoc, "models")
  configsDir = os.path.join(projectLoc, "configs")
  codeGenDir = os.path.join(projectLoc, "codegen")

  model = xmlutils.model2Microdom(modelsDir)
  
  prjName = os.path.basename(projectLoc)
  
  openSAFMain.mainMethod(srcDir,myDir, prjName, model)

  


if __name__ == "__main__":
    print "Running with args %s" % str(sys.argv)
    main(*sys.argv[1:])
