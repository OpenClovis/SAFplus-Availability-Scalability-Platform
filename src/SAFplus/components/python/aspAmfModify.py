"""\namespace aspAmfModify
This module contains routines that will modify existing AMF entities.  Currently it is possible to add work (SAF SI & CSIs) to an existing service group, modify service group configuration, and extend a service group onto other nodes (create new SAF SUs & components).
"""

import os
from types import *

import asp
import aspAmf
from misc import *
from aspLog import Log
from aspAmfEntity import *
from aspAmfCreate import *
from clusterinfo import ci, calcCompToCsiType

def addWork(sg,workName,kvdict,amfSession=None):
  """Extend a SG by adding SUs with all the components defined in the SG
  @param sg           Service group name (string) or entity (aspAmfEntity.ServiceGroup) to add the SI to
  @param workName     Name of the work (SAF SI) to be created
  @param kvdict       Dictionary of name/value pairs to put in all of the CSIs
  @param amfSession   (optional) aspAmf.Session object used to install the new work into the AMF -- if not passed a temporary session will be created

  @returns (newEntities,modifiedEntities) a tuple of 2 lists containing the new and modified entities
  """

  # Translate the incoming variables into a canonical format
  if type(sg) in StringTypes:
    sg = ci.entities[sg]

  (newEntities,modifiedEntities) = CreateServiceInstance(sg,workName, kvdict)

  if not amfSession:
    amfSession = aspAmf.Session()

  amfSession.InstallApp(newEntities,modifiedEntities)
  ci.load()
  return (newEntities,modifiedEntities)

def updateSgCfg(sg,cfgDict,amfSession=None):
  """Modify the SG's configuration by the values in cfgDict

  @param sg           Service group name (string) or entity (aspAmfEntity.ServiceGroup) to modify
  @param cfgDict      A Dot() object or other entity with SG configuration parameters expressed as member variables.  The SG configuration parameters are exactly as written in ClAmsSGConfigT C structure located in clAmsEntities.h
  @param amfSession   (optional) aspAmf.Session object used to install the new work into the AMF -- if not passed a temporary session will be created

  @returns Nothing
  """
  # Translate the incoming variables into a canonical format
  if type(sg) in StringTypes:
    sg = ci.entities[sg]

  print "CFGDICT: ", str(cfgDict)

  Modifiers2Sg(cfgDict,sg)  
  Log("Modified SG: %s" % str(sg.__dict__))
  if not amfSession: amfSession = aspAmf.Session()

  amfSession.InstallSgConfig(sg)



def extendSg(sg,nodes,basename=None,index=None,amfSession=None):
  """Extend a SG by adding SUs with all the components defined in the SG
  @param sg           Service group name (string) or entity (aspAmfEntity.ServiceGroup) to extend
  @param nodes        A list of strings or entities (aspAmfEntity.Node) onto which you want to extend the SG
  @param basename     (optional) The name prefix for the new SUs & other created entities.  If None, the SG's basename will be used
  @param index        (optional) What number to start the SU name with (eg N in: mySUiN).  If None, the number of current SUs will be used
  @param amfSession   (optional) aspAmf.Session object used to install the new work into the AMF -- if not passed a temporary session will be created

  @returns (newEntities,modifiedEntities) a tuple of 2 lists containing the new and modified entities
  """

  # Translate the incoming variables into a canonical format
  if type(sg) in StringTypes:
    sg = ci.entities[sg]
  if type(nodes) is not ListType:
    nodes = [nodes]
  nl = []
  for n in nodes:
    if type(n) in StringTypes:
      nl.append(ci.entities[n])
    else: nl.append(n)
  nodes = nl

  csiTypes = calcCompToCsiType(sg)

  appCfg = sg.appVer.cfg.values()[0]  # GAS TODO: Right now, just picking the first defined application in the bundle

  comps = appCfg.programNames.values()

  # Note: We do not verify that a new SU is not created on the same node as an existing one... should we do so?


  if index is None:
    index = len(flatten([x.comp for x in sg.su]))
  if basename is None:
    basename = sg.name[0:sg.name.find("SG")]

  (newEntities,modifiedEntities,appInstLut) = CreateServiceUnits(sg,nodes, comps, appCfg, basename, csiTypes, index)

  # Always extend by adding actives
  # GAS TODO: Add the same logic in Create to recalculate the best values for these fields.
  sg.activeServiceUnits = sg.cconfig.numPrefActiveSUs + len(nodes)
  sg.instantiatedServiceUnits = sg.cconfig.numPrefInserviceSUs + len(nodes)


  # Create all the new SUs et al.

  if not amfSession:
    amfSession = aspAmf.Session()

  amfSession.InstallApp(newEntities,modifiedEntities)
  ci.load()
  return (newEntities,modifiedEntities)


def newNode(self,nodeInfo):
    """Create a new node entity in the AMF
    @param nodeInfo.  A dictionary containing the following fields:
        name:         Name of the node entity
        slot:         What slot (or fake slot) it is in
        aspdir:       Where is ASP installed on this node?
        localIp:      String specifying the intracluster IP address for this node (used in SW deployment)
        localUser     String specifying a valid user on this node (used in SW deployment)
        localPasswd   String specifying the user's password this node (used in SW deployment)
    """
    node = AmfNode(nodeInfo['name'],nodeInfo['slot'])
    node.localIp     = nodeInfo['localIp']
    node.localUser   = nodeInfo['localUser']
    node.localPasswd = nodeInfo['localPasswd']
    node.aspdir      = nodeInfo['aspdir']

    self.InstallEntities([node])
    return node

