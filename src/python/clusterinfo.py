"""@namespace clusterinfo
import clusterinfo
print clusterinfo.ci.entities
"""

import sys, os, os.path, time, types
import traceback,pdb
import argparse
# import configparser
import re
from tabulate import tabulate
import random, errno
import amfctrl
import readline
import threading
import localaccess as access
import xml.etree.ElementTree as ET
import amfMgmtApi

class ClusterInfo:
    """This class represents the current cluster state and configuration"""
    def __init__(self):
        self.d = None
        # self.lock = threading.RLock()
        self.load()
    
    def load(self):
        n = data()
        n.load(self.d)
        self.d = n

    def __getattr__(self,name):
        """Get the cluster data"""
        try:
        #   self.lock.acquire()
          t = self.d.__dict__[name]
        finally:
            pass
        #   self.lock.release()
        return t

class data:
    """This helper class contains all of the changing cluster data, and is used to ensure that updates occur atomically and quickly. Do not create directly!"""
    def __init__(self):
        self.clear()

    def clear(self):
        """Remove all AMF database Python objects"""
        self.alive = None

        self.entities = {}  ##< Public interface returning a dictionary to access anything by name

        self.nodeList = []  ##< Public interface returning the list of "up" nodes
        self.nodes = {}     ##< Public interface returning a dictionary to access nodes by slot, name or IP address

        self.sgs   = {}     # Public interface returning a dictionary of service groups
        self.sgList = []

        self.sus = {}
        self.suList = []

        self.sis = {}
        self.siList = []

        self.csis = {}
        self.csiList = []

        self.comps = {}
        self.compList = []

        self.apps = {}
        self.appList = []

    def load(self, oldObject = None):
        """Load or re-load the AMF database into Python objects"""
        self.clear()
        self.loadNodes()
        self.loadSGs()
        self.loadSUs()
        self.loadSIs()
        self.loadCSIs()
        self.loadComps()

    def loadNodes(self, oldObject = None):
        """Load the node information"""
        listNode = getNameOfEntity("Node")
        if not isinstance(listNode, list):
            return
        for nodeName in listNode:
            node = getInformationOfEntity("Node", nodeName)
            self.nodes[nodeName] = node
            self.entities[nodeName] = node
            self.nodeList.append(node)
        clusterView = access.grpCliClusterViewGet()
            # Node0       controller     active   108
            # Node1       controller     standby  109
            # Node2       payload        -        110
        rows = clusterView.split('\n')
        for node in self.nodeList:
            for row in rows:
                if len(row) == 0:
                    continue
                cells = row.split()
                if node.name==cells[0]:
                    node.slot = cells[3]
                    node.clusterRole = cells[1]
                    node.haRole = cells[2]
            node.setIntraclusterAccess()

    def loadSGs(self, oldObject = None):
        """Load the SG information"""
        listSG = getNameOfEntity("ServiceGroup")
        if not isinstance(listSG, list):
            return
        for sgName in listSG:
            sg = getInformationOfEntity("ServiceGroup", sgName)
            self.sgs[sgName] = sg
            self.entities[sgName] = sg
            self.sgList.append(sg)

    def loadSUs(self, oldObject = None):
        """Load the SU information"""
        listSU = getNameOfEntity("ServiceUnit")
        if not isinstance(listSU, list):
            return
        for suName in listSU:
            su = getInformationOfEntity("ServiceUnit", suName)
            self.sus[suName] = su
            self.entities[suName] = su
            self.suList.append(su)

    def loadSIs(self, oldObject = None):
        """Load the SI information"""
        listSI = getNameOfEntity("ServiceInstance")
        if not isinstance(listSI, list):
            return
        for siName in listSI:
            si = getInformationOfEntity("ServiceInstance", siName)
            self.sis[siName] = si
            self.entities[siName] = si
            self.siList.append(si)

    def loadCSIs(self, oldObject = None):
        """Load the CSI information"""
        listCSI = getNameOfEntity("ComponentServiceInstance")
        if not isinstance(listCSI, list):
            return
        for csiName in listCSI:
            csi = getInformationOfEntity("ComponentServiceInstance", csiName)
            self.csis[csiName] = csi
            self.entities[csiName] = csi
            self.csiList.append(csi)

    def loadComps(self, oldObject = None):
        """Load the Component information"""
        listComp = getNameOfEntity("Component")
        if not isinstance(listComp, list):
            return
        for compName in listComp:
            comp = getInformationOfEntity("Component", compName)
            self.comps[compName] = comp
            self.entities[compName] = comp
            self.compList.append(comp)

global handle

def get_amf_master_handle():
  global handle
  info = access.grpCliClusterViewGet()
  # NodeName    NodeType       HAState  NodeAddr
  # Node0       controller     active   108
  # Node1       controller     standby  109
  # Node2       payload        -        110
  rows = info.split('\n')
  for row in rows:
      if len(row) == 0:
          continue
      cells = row.split()
      if cells[1] == 'controller' and cells[2] == 'active':
          masterNodeId = int(cells[3])
          handle = access.getProcessHandle(1, masterNodeId)
          return masterNodeId

def getNameOfEntity(entityType):
  """Get name of an entity"""

  t = os.path.normpath(os.path.join("/safplusAmf/", entityType))
  depth=1
  gs = "{d=%s}%s" % (depth,str(t))
  try:
    xml = access.mgtGet(handle, gs)
    xml = "<top>" + xml + "</top>"
  except RuntimeError as e:
    if str(e) == "Route has no implementer":
      return "<error>Invalid path [%s]</error>" % str(t)
    return "<error>" + str(e) + "</error>"
  except Exception as e:
    return "<error>" + str(e) + "</error>"
  try:
    # print xml
    root = ET.fromstring(xml)
  except IndexError as e:
    print ("<error>xml [%s] error [%s]</error>" % (xml, str(e)))
    return ""
  except ET.ParseError as e:
    print ("<error>xml [%s] error [%s]</error>" % (xml, str(e)))
    return ""
  listEntityName = []
  for child in root:
    dictAtrr = child.attrib
    listEntityName.append(dictAtrr["listkey"])
  return listEntityName


def getInformationOfEntity(entityType, entityName):
    """syntax: entityPrint entityType entityName
    Display attributes in an entity
    """

    t = os.path.normpath(os.path.join("/safplusAmf",entityType,entityName))
    depth=1
    gs = "{d=%s}%s" % (depth,str(t))
    try:
        xml = access.mgtGet(handle, gs)
    except RuntimeError as e:
        if str(e) == "Route has no implementer":
            return "<error>Invalid path [%s]</error>" % str(t)
        return "<error>" + str(e) + "</error>"
    except Exception as e:
        return "<error>" + str(e) + "</error>"

    try:
        root = ET.fromstring(xml)
    except IndexError as e:
        print ("<error>xml [%s] error [%s]</error>" % (xml, str(e)))
        return ""
    except ET.ParseError as e:
        print ("<error>xml [%s] error [%s]</error>" % (xml, str(e)))
        return ""
    initObject = initialObject()
    for child in root:
        if child.text == None:
            t1 = os.path.normpath(os.path.join("/safplusAmf",entityType,entityName,child.tag))
            depth1=1
            gs1 = "{d=%s}%s" % (depth1,str(t1))
            xml1 = access.mgtGet(handle, gs1)
            root1 = ET.fromstring(xml1)
            for child1 in root1.iter('current'):
                if hasattr(initObject, child.tag):
                    setattr(initObject, child.tag, child1.text)
        else:
            if hasattr(initObject, child.tag):
                setattr(initObject, child.tag, child.text)
    return initObject

class initialObject:
    """Initial all attributes in an entity"""

    def __init__(self):
        self.slot = ""
        self.clusterRole = "" # controller or payload
        self.haRole = "" # active or standby. If payload, no role (-)
        self.adminState = ""
        self.localIp = ""
        self.localUser = ""
        self.localPasswd = ""
        self.aspDir = ""
        self.autoRepair = ""
        self.canBeInherited = ""
        self.currentRecovery = ""
        self.disableAssignmentOn = ""
        self.failFastOnCleanupFailure = ""
        self.failFastOnInstantiationFailure = ""
        self.id = ""
        self.lastSUFailure = ""
        self.name = ""
        self.restartable = ""
        self.serviceUnitFailureEscalationPolicy = ""
        self.userDefinedType = ""
        self.autoAdjust = ""
        self.autoAdjustInterval = ""
        self.componentRestart = ""
        self.maxActiveWorkAssignments = ""
        self.maxStandbyWorkAssignments = ""
        self.preferredNumActiveServiceUnits = ""
        self.preferredNumIdleServiceUnits = ""
        self.preferredNumStandbyServiceUnits = ""
        self.serviceInstances = ""
        self.serviceUnitRestart = ""
        self.serviceUnits = ""
        self.assignedServiceInstances = ""
        self.compRestartCount = ""
        self.components = ""
        self.failover = ""
        self.lastCompRestart = ""
        self.lastRestart = ""
        self.node = ""
        self.rank = ""
        self.saAmfSUHostNodeOrNodeGroup = ""
        self.serviceGroup = ""
        self.capabilityModel = ""
        self.cleanup = ""
        self.csiType = ""
        self.delayBetweenInstantiation = ""
        self.instantiate = ""
        self.instantiateLevel = ""
        self.maxInstantInstantiations = ""
        self.maxStandbyAssignments = ""
        self.proxied = ""
        self.proxyCSI = ""
        self.recovery = ""
        self.serviceUnit = ""
        self.terminate = ""
        self.timeouts = ""
        self.isFullStandbyAssignment = ""
        self.isFullActiveAssignment = ""
        self.preferredActiveAssignments = ""
        self.preferredStandbyAssignments = ""
        self.dependencies = ""
        self.type = ""
        self.instantiationSuccessDuration = ""
        self.maxActiveAssignments = ""
        self.maxDelayedInstantiations = ""
        self.componentServiceInstances = ""
        self.serviceInstance = ""
        self.operState = ""
        self.presenceState = ""
        self.stats = ""
        self.numAssignedServiceUnits = ""
        self.numIdleServiceUnits = ""
        self.numSpareServiceUnits = ""
        self.haReadinessState = ""
        self.haState = ""
        self.numActiveServiceInstances = ""
        self.numStandbyServiceInstances = ""
        self.preinstantiable = ""
        self.probationTime = ""
        self.readinessState = ""
        self.restartCount = ""
        self.activeAssignments = ""
        self.compCategory = ""
        self.compProperty = ""
        self.lastError = ""
        self.lastInstantiation = ""
        self.numInstantiationAttempts = ""
        self.pendingOperation = ""
        self.pendingOperationExpiration = ""
        self.procStats = ""
        self.processId = ""
        self.safVersion = ""
        self.swBundle = ""
        self.numActiveAssignments = ""
        self.numStandbyAssignments = ""
        self.standbyAssignments = ""
        self.activeComponents = ""
        self.isProxyCSI = ""
        self.standbyComponents = ""
        self.assignmentState = ""

    def setIntraclusterAccess(self):
        print ('get install info')
        (ip,aspdir)=amfMgmtApi.getSafplusInstallInfo(self.name)
        print ('end')
        self.localIp = ip
        self.aspDir = aspdir

    def isPresent(self):
        return self.presenceState=='instantiated'

    def isRunning(self):
        return self.isPresent() and self.adminState != 'off'

    def isIdle(self):
         return self.isRunning() and self.adminState=='idle'

def main():
    print("start clusterinfo")
    get_amf_master_handle()
    print('init ci')
    clinfo = ClusterInfo()
    print("length: ", len(clinfo.nodeList))
    for node in clinfo.nodeList:
        print(node.name, " : ", node.adminState, ", ", node.presenceState)
        print ("service units: %s" % str(node.serviceUnits))
        print ('Node Type:%s' % node.clusterRole)

if __name__ == '__main__':
    main()
