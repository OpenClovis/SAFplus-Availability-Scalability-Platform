from types import *
import copy

import clusterinfo; import aspAmf
from aspAmfEntity import *


class NameMgr:
  def __init__(self,taken=None):
    self.names = {}
    for n in taken: self.names[n] = True
    self.cnt = 0
    
  def fix(self,name):
    if self.names.has_key(name):
      self.cnt+=1
      return self.fix(name + "_" + str(self.cnt))
    else:
      self.names[name] = True
      return name


def clone(sgName,newName,index,numClones=1,_nodeList=None):
  created = []
  modified = []
  xlat = {}  # Conversion from the old to the new objects
  idx=0
  if 1:  # try:
    error = []
    ci = clusterinfo.ci     # The clusterinfo.ci objects reflect the entire state of the cluster
    ci.load()
    amf = aspAmf.Session()  # The aspAmf.Session class lets you modify the cluster state
    namesTaken = NameMgr(ci.d.entities.keys())

    try:
      osg = ci.sgs[sgName]
    except KeyError:
      error.append("SG %s does not exist" % sgName)
      raise

    if _nodeList:
      # Convert an integer (slot number) or string (node name) to the node object
      nodeList = []
      for n in _nodeList:
        if type(n) in StringTypes or type(n) is IntType:
          nodeList.append(ci.nodes[n])
        else:
          nodeList.append(n)

      if len(osg.su) != len(nodeList):
        error.append("Number of deployment nodes != number of SUs" % sgName)
        raise
    else:  # Deploy onto the same nodes
      nodeList = [None] * len(osg.su)
        

    # Try to split the name based on our standard nomenclature to figure out the base and index.  If it does not work, that's ok, just use the raw name.
    #namespl = osg.name.split("SG")   
    #basename = osg.name.split("SG")[0]
    #if len(namespl)>1:
    #  try:
    #    index = int(namespl[1][1:])+1
    #  except:
    #    index = 0
    #else: index = 0
    basename = newName

    # create the service group
    nsg = AmfServiceGroup(namesTaken.fix(basename + "SG" + "i" + str(index)))
    nsg.cconfig = copy.copy(osg.cconfig)
    created.append(nsg)

    # create the service units & components
    nsu = []
    for (osu,dstNode) in zip(osg.su,nodeList):
      if dstNode is None:
        dstNode = osu.node
      su =  AmfServiceUnit(namesTaken.fix(basename + "SU" + "i" +  str(index) + "_on_" + osu.node.name), nsg, dstNode)
      su.cconfig = copy.copy(osu.cconfig)
      dstNode.su.append(su)  # Add this SU into the node
      nsg.su.append(su) # Add this SU into the SG
      nsu.append(su)
      modified.append(dstNode)
      created.append(su)
#      ncomp = []
      for ocomp in osu.comp:
        app = str(ocomp.cconfig.instantiateCommand)
        comp = AmfComponent(namesTaken.fix(basename + "Ci" + str(index) + "_" + app + "_on_" + dstNode.name))
        comp.cconfig = copy.copy(ocomp.cconfig)
        comp.supportedCsis.add(ocomp.cconfig.pSupportedCSITypes.value)
        comp.su = su          # Hook the SU up to the comp 
        su.comp.append(comp)  # Hook the comp up to the SU
#        ncomp.append(comp)
        created.append(comp)        

    # create the service instances & component service instances
    for osi in osg.si:
      siname = osi.name.split("SI")[0]  
      svcInst = AmfServiceInstance(namesTaken.fix(basename + "SIi" + str(index)))
      # si has no cconfig... svcInst.cconfig = copy.copy(svcInst.cconfig)
      nsg.si.append(svcInst)  # Add this SI into the SG
      created.append(svcInst)
      for ocsi in osi.csi:
        csiname = ocsi.name.split("CSI")[0]
        d = dict(ocsi.kvdict)
        csiType = str(ocsi.csiType)
        print "CSI type: ", csiType
        csi = AmfComponentServiceInstance(namesTaken.fix(csiname + "CSIi" + str(index) + ocsi.csiType),d, csiType)
        #csi.cconfig = copy.copy(csi.cconfig)
        svcInst.csi.append(csi)
        created.append(csi)

    amf.InstallApp(created, modified)

  if 0: # except:
    pass

  return (created,modified,{})

  
