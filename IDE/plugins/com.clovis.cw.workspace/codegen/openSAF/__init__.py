import time
import os.path
import xml.dom.minidom as minidom
import logging

import scripts.openSAFMain
import microdom

import tarfile
from StringIO import StringIO

# The XML files that contain the info we need to generate an openSAF model.
InterestingFiles = ["resourcedata.xml","componentdata.xml","alarmdata.xml","modelchanges.xmi","component_resource_map.xml","resource_alarm_map.xml","component_templateGroup_map.xml","alarmrule.xml","clAmfDefinitions.xml","clAmfConfig.xml"]


class OpenSafGenError(Exception):
    pass

def generate(filedict, mdlName):

    # Convert the filedict to a list of microdom objects
    doms = []
    for (name,txt) in filedict.items():
      basename = os.path.basename(name)
      if basename in InterestingFiles:
        dom = minidom.parseString(txt)
        doms.append(microdom.LoadMiniDom(dom.childNodes[0]))
      else:
        #print "Unnecessary file %s" % name
        pass

    # Combine all microdom objects into one
    model = microdom.Merge("top",doms)

    out = scripts.openSAFMain.DictionaryOutput()
    scripts.openSAFMain.mainMethod(".",".", mdlName, model, out)

    return out.dict


def makeTarInfo(name,txt):
  ret = tarfile.TarInfo(name)
  ret.size = len(txt)

  return ret

def tar2dict(tf):
    files = tf.getmembers()
    ret = {}
    for f in files:
        if f.isfile():
          fileobj = tf.extractfile(f)
          ret[f.name] = fileobj.read()
          logging.info(str(f))


    return ret

def cvtModel(mdlFp,outputFp,mdlName):
#    print "New load"
#    print mdlFp
    mdltar = tarfile.open(fileobj=mdlFp)
    if not mdltar:
        raise OpenSafGenError("Error opening Tar file")
    ctnts = mdltar.getmembers()
    d = tar2dict(mdltar)
    
    out = generate(d, mdlName)

    # Now open an in-ram tarball and add the files
    tf = tarfile.open(mode='w|gz', fileobj=outputFp)
    timestamp = time.time()
    for (name,txt) in out.items():
      ti = makeTarInfo(name,txt)
      ti.mtime = timestamp
      tf.addfile(ti, StringIO(txt))
    
    return mdltar.getnames()


if __name__ == "__main__":
  import sys
  inp = open(sys.argv[1], "r")
  outp = open(sys.argv[2], "w")
  modelName = os.path.basename(sys.argv[1]).split(".")[0]
  cvtModel(inp, outp, modelName)

def Test():
  inp = open("/code/mdltar.tgz", "r")
  outp = open("gentest.tgz", "w")
    
  cvtModel(inp,outp,'SampleModel')
