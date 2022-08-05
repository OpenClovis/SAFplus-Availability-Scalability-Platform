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
import aspApp

handle = 0
refreshInterval = 1.9

class ClusterInfo:
    """This class represents the current cluster state and configuration"""
    def __init__(self):
        # self.d = None
        self.d = data()
        # self.lock = threading.RLock()
        self.load()
        

    def refresh(self):
        global refreshInterval
        if (time.time()-self.d.lastReload)>refreshInterval:
            self.load()
 
    def load(self):
        # n = data()
        # n.load(self.d)
        # self.d = n
        self.d.load()

    def __getattr__(self,name):
        """Get the cluster data"""
        try:
        #   self.lock.acquire()
          t = self.d.__dict__[name]
        except KeyError:
            # function name is not available in d.__dict__
            # Do this, we can call ci.addSuppliedData() instead of ci.d.addSuppliedData()
            t = getattr(self.d, name)
        finally:
            pass
        #   self.lock.release()
        return t

class data:
    """This helper class contains all of the changing cluster data, and is used to ensure that updates occur atomically and quickly. Do not create directly!"""
    def __init__(self):
        self.clear()
        self.suppliedData = {}  #stores usernames, passwords needed to gain access to a node (input by user)
        self.associatedData = {} #stores appName, appVer of all deployed SGs

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
        get_amf_master_handle()
        self.loadNodes()
        self.loadSGs()
        self.loadSUs()
        self.loadSIs()
        self.loadCSIs()
        self.loadComps()
        self.loadAssociatedData()
        self.lastReload = time.time()

    def addSuppliedData(self, nodeName, localUser, localPasswd):
        accountData = {"localUser" : localUser, "localPasswd" : localPasswd}
        dataToUpdate = {nodeName : accountData}
        
        self.suppliedData.update(dataToUpdate)

    def setSuppliedData(self, nodeEntity):
        accountData = self.suppliedData.get(nodeEntity.name, None)

        if accountData:
            nodeEntity.localUser = accountData["localUser"]
            nodeEntity.localPasswd = accountData["localPasswd"]
        else:
            print("No account data available for node [%s] in ci.suppliedData"%nodeEntity.name)

    def addAssociatedData(self, sgName, appName, appVer):
        data = appName + " " + appVer
        dataToUpdate = {sgName : data}

        self.associatedData.update(dataToUpdate)

    def loadAssociatedData(self):
        for sg in self.sgList:
            data = self.associatedData.get(sg.name, "")
            sg.associatedData = data
        
        try:
            global appDatabase
            appDatabase.ConnectToServiceGroups()    # attach apps to sgs and vice versa
        except NameError:
            pass # appDatabase is not initialized when ci.load() is first ever called

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
            self.setSuppliedData(node)

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

#global handle

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
    kvpairs = {}
    for child in root:
        if child.text == None:
            if child.tag == "data":
                tagAtrr = child.attrib
                attrValues = list(tagAtrr.values()) # view obj to list
                key = attrValues[0]
                kvpair = parseKvpair(t+"/data/"+key)
                kvpairs.update(kvpair)
                continue

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

    initObject.entityType = entityType
    initObject.data = kvpairs

    return initObject

def parseKvpair(dataPath):
    depth=1
    gs = "{d=%s}%s" % (depth,str(dataPath))
    xml = access.mgtGet(handle, gs)

    regexp = '<name>(.*?)</name><val>(.*?)</val>'
    pairs = re.findall(regexp, xml)
    try:
        (key, val) = pairs[0]
        kvpair = {key: val}
    except KeyError as e:
        print("Malformed data tag.")
        kvpair = {}
    
    return kvpair

def getDependentEntites(entity):
    ret = []
    entities = []

    #if a child entity got deleted, it's name in parent's attr will be replaced with "?", ignore it.
    if entity.entityType == "Node":
        suNames = [] if entity.serviceUnits == "" else entity.serviceUnits.split(", ")
        entities = [getInformationOfEntity("ServiceUnit", suName) for suName in suNames if suName != "?"]

    elif entity.entityType == "ServiceGroup":
        suNames = [] if entity.serviceUnits == "" else entity.serviceUnits.split(", ")
        siNames = [] if entity.serviceInstances == "" else entity.serviceInstances.split(", ")

        entities += [getInformationOfEntity("ServiceUnit", suName) for suName in suNames if suName != "?"]
        entities += [getInformationOfEntity("ServiceInstance", siName) for siName in siNames if siName != "?"]

    elif entity.entityType == "ServiceUnit":
        compNames = [] if entity.components == "" else entity.components.split(", ")

        entities = [getInformationOfEntity("Component", compName) for compName in compNames if compName != "?"]

    elif entity.entityType == "ServiceInstance":
        csiNames = [] if entity.componentServiceInstances == "" else entity.componentServiceInstances.split(", ")

        entities = [getInformationOfEntity("ComponentServiceInstance", csiName) for csiName in csiNames if csiName != "?"]

    for ent in entities:
        if ent != "":   #ent == "" when it's already deleted but ci.refresh() hasn't been called, so getInformationOfEntity returns ""
            ret += getDependentEntites(ent)

    ret.append(entity)

    return ret

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
        self.entityType = ""
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
        self.associatedData = ""
        self.app = None # stores app object corresponding to deployed SGs
        self.appVer = None # stores appFile object
        self.data = {} # key/val dict

    def setIntraclusterAccess(self):
        print ('get install info')
        (ip,aspdir)=amfMgmtApi.getSafplusInstallInfo(self.name)
        print ('end')
        self.localIp = ip
        self.aspDir = aspdir

    def isPresent(self):
        return self.presenceState=='instantiated' or self.presenceState==''

    def isRunning(self):
        return self.isPresent() and self.adminState != 'off'

    def isIdle(self):
         return self.isRunning() and self.adminState=='idle'

    def command(self):

        t = os.path.normpath(os.path.join("/safplusAmf/Component",self.name,"instantiate"))
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

        for child in root:
            if child.tag == "command":
                return child.text

        return ""

    def getRestartCount(self):
        #works for SU or Comp
        t = os.path.normpath(os.path.join("/safplusAmf",self.entityType,self.name,"restartCount"))
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

        for child in root:
            if child.tag == "current":
                return child.text

    def getTotalRestartCount(self):
        suNames = [] if self.serviceUnits == "" else self.serviceUnits.split(", ")

        suEntities = [getInformationOfEntity("ServiceUnit", suName) for suName in suNames if suName != "?"]

        totalCount = 0

        for suEntity in suEntities:
            restartCount = int(suEntity.getRestartCount())
            totalCount += restartCount

        return str(totalCount)

    def getAllComps(self):
        #function works for Node, SG, SU
        suNames = [] if self.serviceUnits == "" else self.serviceUnits.split(", ")
        
        suEntities = [getInformationOfEntity("ServiceUnit", suName) for suName in suNames if suName != "?"]

        compNames = [] if self.components == "" else self.components.split(", ")
        for suEntity in suEntities:
            compNames += [] if suEntity.components == "" else suEntity.components.split(", ")

        compEntities = [getInformationOfEntity("Component", compName) for compName in compNames if compName != "?"]

        return compEntities

    def getAllDeployments(self):
        suNames = [] if self.serviceUnits == "" else self.serviceUnits.split(", ")
        
        suEntities = [getInformationOfEntity("ServiceUnit", suName) for suName in suNames if suName != "?"]

        sgNames = []
        sgEntities = []
        for suEntity in suEntities:
            if suEntity.serviceGroup not in sgNames:
                sgNames.append(suEntity.serviceGroup)
                sgEntity = getInformationOfEntity("ServiceGroup", suEntity.serviceGroup)
                sgEntities.append(sgEntity)

                global appDatabase  #setting app information after getting SG from safplus
                data = ci.associatedData.get(sgEntity.name, "")
                sgEntity.associatedData = data
                [appName, appVer] = data.split() if data else ["", ""]

                app = appDatabase.entities.get(appName, None)
                sgEntity.app = app
                sgEntity.appVer = app.version.get(appVer, None) if app else None

        return sgEntities

    def getAllNodes(self):
        suNames = [] if self.serviceUnits == "" else self.serviceUnits.split(", ")

        nodeNames = []  # for duplicate checking
        nodeEntities = []
        for suName in suNames:
            suEntity = getInformationOfEntity("ServiceUnit", suName) if suName != "?" else None

            if suEntity and suEntity.node not in nodeNames:
                nodeNames.append(suEntity.node)
                nodeEntity = getInformationOfEntity("Node", suEntity.node)
                nodeEntities.append(nodeEntity)

        return nodeEntities

    def getAllServiceInstances(self):
        # works for SG
        siNames = [] if self.serviceInstances == "" else self.serviceInstances.split(", ")

        siEntities = []
        for siName in siNames:
            siEntity = getInformationOfEntity("ServiceInstance", siName) if siName != "?" else None

            if siEntity:
                siEntities.append(siEntity)

        return siEntities

    def getAllCSIs(self):
        # works for SI
        csiNames = [] if self.componentServiceInstances == "" else self.componentServiceInstances.split(", ")

        csiEntities = []
        for csiName in csiNames:
            siEntity = getInformationOfEntity("ComponentServiceInstance", csiName) if csiName != "?" else None

            if siEntity:
                csiEntities.append(siEntity)

        return csiEntities

    def getActiveSu(self):
        suNames = [] if self.serviceUnits == "" else self.serviceUnits.split(", ")  # for SG
        suNames += [] if self.activeAssignments == "" else self.activeAssignments.split(", ")   # for SI

        for suName in suNames:
            suEntity = getInformationOfEntity("ServiceUnit", suName) if suName != "?" else None
            if suEntity and suEntity.haState == "active":
                return suEntity
        return None

    def getStandbySu(self):
        suNames = [] if self.serviceUnits == "" else self.serviceUnits.split(", ")
        suNames += [] if self.standbyAssignments == "" else self.standbyAssignments.split(", ")

        for suName in suNames:
            suEntity = getInformationOfEntity("ServiceUnit", suName) if suName != "?" else None
            if suEntity and suEntity.haState == "standby":
                return suEntity
        return None

    def getActiveNode(self):
        activeSU = self.getActiveSu()

        if activeSU:
            activeNode = getInformationOfEntity("Node", activeSU.node)
            return activeNode

        return None

    def getStandbyNode(self):
        standbySU = self.getStandbySu()

        if standbySU:
            standbyNode = getInformationOfEntity("Node", standbySU.node)
            return standbyNode

        return None

ci = ClusterInfo()
appDatabase = aspApp.AppDb()

def main():
    print("start clusterinfo")
    #get_amf_master_handle()
    print('init ci')
    #clinfo = ClusterInfo()
    print("length: ", len(ci.nodeList))
    for node in ci.nodeList:
        print(node.name, " : ", node.adminState, ", ", node.presenceState)
        print ("service units: %s" % str(node.serviceUnits))
        print ('Node Type:%s' % node.clusterRole)

if __name__ == '__main__':
    main()
