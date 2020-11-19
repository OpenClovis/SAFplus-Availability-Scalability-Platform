from string import Template
import os, types
import time
from types import *
import inspect
import share
import sys

import instanceEditor

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

    # Get infomation about model
    nodeIntances = []
    for (name, e) in share.detailsPanel.model.instances.items():
      if e.data['entityType'] == "Node":
        nodeIntances.append(name)

    for img in nodeIntances:
      image = {}
      image['name'] = img
      try:
        image['slot'] = str(md[img].slot.data_).strip()
        image['netInterface'] = str(md[img].netInterface.data_).strip()
      except:
        image['slot'] = "SC"
        image['netInterface'] = ""
    #print 'generate[%d]:  %s.tgz = %s'%(sys._getframe().f_lineno, image['name'], image)
    
    
    
    
    makeSubsDict['nameTgz'] = image['name']+'.tgz'
    s = mkSubdirTmpl.safe_substitute(**makeSubsDict)
    output.write(srcDir + os.sep + "Makefile", s)
    return [srcDir + os.sep + "Makefile"]


def cpp(output, srcDir, comp,ts_comp, proxy=False):
    # Create source main
    compName = str(comp.data["name"])

    ts_comp['name'] = comp.data["name"]
    ts_comp['instantiate_command'] = comp.data["name"] # Default bin name

    try:
      ts_comp['instantiate_command'] = comp.data["instantiate"]["command"].split()[0]
    except:
      pass
    
    if not proxy:
      print 'gen cpp for non-proxy'
      cpptmpl = templateMgr.loadPyTemplate(TemplatePath + "main.cpp.ts")
    else:
      print 'gen cpp for proxy'
      cpptmpl = templateMgr.loadPyTemplate(TemplatePath + "mainProxy.cpp.ts")

    main = cpptmpl.safe_substitute(**ts_comp)

    # Create Makefile
    makefiletmpl = templateMgr.loadPyTemplate(TemplatePath + "Makefile.cpp.ts")
    makefile = makefiletmpl.safe_substitute(**ts_comp)


    parentFrame = share.instancePanel.guiPlaces.frame
    template = {}
    if not os.path.exists(srcDir+os.sep+compName) or os.path.exists(srcDir+os.sep+compName) and not any(item.endswith('.cxx') for item in os.listdir(srcDir+os.sep+compName)):
      print 'Source code does not exist so create them'
      # Create main.cxx
      if not proxy:
        print 'create cpp for non-proxy'
        output.write(srcDir + os.sep + compName + os.sep + "main.cxx", main)
      else:
        print 'create cpp for proxy'
        output.write(srcDir + os.sep + compName + os.sep + "proxyMain.cxx", main)
      # Create Makefile
      output.write(srcDir + os.sep + compName + os.sep + "Makefile", makefile)
    else:
      for item in os.listdir(srcDir+os.sep+compName):
        if item.endswith('.cxx'):
          template[item] = main
        elif item == 'Makefile':
          template[item] = makefile
      for i in list(template.keys()):
        with open(srcDir+os.sep+compName+os.sep+i) as reader:
          if reader.read() != template[i] and parentFrame.project.prjProperties['mergeMode'] == "prompt":
            dlg = instanceEditor.Reminder()
            result = instanceEditor.Reminder().Dialog(srcDir+os.sep+compName+os.sep+i)
            if not result:
              return
            elif result == 2:
              if i == 'Makefile':
                output.write(srcDir + os.sep + compName + os.sep + "Makefile", makefile)
              else:
                if not proxy:
                  print 'create cpp for non-proxy'
                  output.write(srcDir + os.sep + compName + os.sep + "main.cxx", main)
      
                else:
                  print 'create cpp for proxy'
                  output.write(srcDir + os.sep + compName + os.sep + "proxyMain.cxx", main)
            elif result == 1:
              pass   
          # Do not show the dialog again and always overwrite the source code.         
          elif parentFrame.project.prjProperties['mergeMode'] == "always":
            if i == 'Makefile':
              output.write(srcDir + os.sep + compName + os.sep + "Makefile", makefile)    
            else:
              if not proxy:
                print 'create cpp for non-proxy'
                output.write(srcDir + os.sep + compName + os.sep + "main.cxx", main)
      
              else:
                print 'create cpp for proxy'
                output.write(srcDir + os.sep + compName + os.sep + "proxyMain.cxx", main)
          # Do not show the dialog again and never overwrite the source code.
          elif parentFrame.project.prjProperties['mergeMode'] == "never":
            pass

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
