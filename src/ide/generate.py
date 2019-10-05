from string import Template
import os, types
import time
from types import *
import inspect

import common

class TemplateMgr:
  """ This class caches template files for later use to reduce disk access"""
  def __init__(self):
    self.cache = {}
  
  def loadPyTemplate(self,fname):
    """Load a template file and return it as a string"""
    if self.cache.has_key(fname): return self.cache[fname]

    fname = common.fileResolver(fname);
    f = open(fname,"r")
    s = f.read()
    t = Template(s)
    self.cache[fname] = t
    return t

  def loadKidTemplate(self,fname):
    pass


# The global template manager
templateMgr = TemplateMgr()

#TODO: Get relative directiry with model.xml
TemplatePath = ""

makeBinApp = """
$(BIN_DIR)/%s:
	$(MAKE) SAFPLUS_SRC_DIR=$(SAFPLUS_SRC_DIR) -C %s
"""

cleanApp = """	$(MAKE) SAFPLUS_SRC_DIR=$(SAFPLUS_SRC_DIR) -C %s clean """

# the safplus directory (full path)
safplusDir=''

def topMakefile(output, srcDir,dirNames):
    global safplusDir
    thisDir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    global TemplatePath
    TemplatePath = thisDir+'/codegen/templates/'
    safplusDir=thisDir+'/../..'
    mkSubdirTmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.subdir.ts")
    makeSubsDict = {}
    makeSubsDict['safplusDir'] = "SAFPLUS_DIR=%s" % safplusDir
    makeSubsDict['subdirs'] = " ".join(["$(BIN_DIR)/%s" % c for c in dirNames])
    makeSubsDict['labelApps'] = "\n".join([makeBinApp % (c,c) for c in dirNames])
    makeSubsDict['cleanupApps'] = "\n".join([cleanApp % c for c in dirNames])
    s = mkSubdirTmpl.safe_substitute(**makeSubsDict)
    output.write(srcDir + os.sep + "Makefile", s)
    return [srcDir + os.sep + "Makefile"]


#def cpp(output, srcDir, comp,ts_comp):
def cpp(output, srcDir, comp,ts_comp, proxyProxied):#proxy-proxied feature
    # Create main
    compName = str(comp.data["name"])

    ts_comp['name'] = comp.data["name"]
    ts_comp['instantiate_command'] = comp.data["name"] # Default bin name

    try:
      ts_comp['instantiate_command'] = comp.data["instantiate"]["command"].split()[0]
    except:
      pass
    #proxy-proxied feature
    if len(proxyProxied)!=0:
      if compName in proxyProxied.keys():
        #proxy case
        cpptmpl = templateMgr.loadPyTemplate(TemplatePath + "nonSAF_main.cpp.ts")
        s = cpptmpl.safe_substitute(**ts_comp)
        output.write(srcDir + os.sep + compName + os.sep + "main.cxx", s)
        tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.cpp.ts")
        s = tmpl.safe_substitute(**ts_comp)
        output.write(srcDir + os.sep + compName + os.sep + "Makefile", s)
        return [srcDir + os.sep + compName + os.sep + "Makefile",srcDir + os.sep + compName + os.sep + "main.cxx"]
      elif compName in [x for v in proxyProxied.values() for x in v]:
        #proxied case
        tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.cpp.ts")
        s = tmpl.safe_substitute(**ts_comp)
        output.write(srcDir + os.sep + compName + os.sep + "Makefile", s)
        return [srcDir + os.sep + compName + os.sep + "Makefile"]
    else:
      #SA comp
      cpptmpl = templateMgr.loadPyTemplate(TemplatePath + "main.cpp.ts")
      s = cpptmpl.safe_substitute(**ts_comp)
      output.write(srcDir + os.sep + compName + os.sep + "main.cxx", s)

      # Create Makefile
      tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.cpp.ts")
      s = tmpl.safe_substitute(**ts_comp)
      output.write(srcDir + os.sep + compName + os.sep + "Makefile", s)
      return [srcDir + os.sep + compName + os.sep + "Makefile",srcDir + os.sep + compName + os.sep + "main.cxx"]

def c(output, srcDir, comp,ts_comp):
    #? TODO:
    return
    compName = str(comp.data["name"])
  
    # Create main
    cpptmpl = templateMgr.loadPyTemplate(TemplatePath + "main.c.ts")
    s = cpptmpl.safe_substitute(addmain=False)
    output.write(srcDir + os.sep + compName + os.sep + "main.c", s)
  
    # Create Makefile
    tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.c.ts")
    s = tmpl.safe_substitute(**ts_comp)
    output.write(srcDir + os.sep + compName + os.sep + "Makefile", s)

def python(output, srcDir, comp,ts_comp):
    #? TODO:
    return
    compName = str(comp.data["name"])
  
    # Create src files
    for (inp,outp) in [("main.py.ts",str(comp.data[instantiateCommand]) + ".py"),("pythonexec.sh",str(comp.data[instantiateCommand]))]:
      tmpl = templateMgr.loadPyTemplate(TemplatePath + inp)
      s = tmpl.safe_substitute(**ts_comp)
      output.write(srcDir + os.sep + compName + os.sep + outp, s)
    
  
    # Create Makefile
    tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.python.ts")
    s = tmpl.safe_substitute(**ts_comp)
    output.write(srcDir + os.sep + compName + os.sep + "Makefile", s)

def java(output, srcDir, comp,ts_comp):
    #? TODO:
    return
    compName = str(comp.data["name"])
    # Create Makefile
    tmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.java.ts")
    s = tmpl.safe_substitute(**ts_comp)
    output.write(srcDir + os.sep + compName + os.sep + "Makefile", s)
