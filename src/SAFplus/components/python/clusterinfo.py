"""@namespace clusterinfo
This module provides an object-oriented perspective on the entire ASP AMF information model and the current state of all ASP AMF entities in the cluster.  It does so through a set of data structures that are available through the ClusterInfo class.  To preserve atomicity the variables actually reference through a contained class, so please see the ClusterInfo class documentation to discover what member variables are available instead of looking at actual class members.

It is recommended that you use the global variable "ci" instead of creating your own clusterinfo object.
For example:
import clusterinfo
print clusterinfo.ci.entities

"""

import pdb
import os
import psh
import time
import threading
from types import *

try:
  import asp
  import asppycustom
  from aspMisc import *
  from aspLog import *
  from aspAmf import *
  fake = False
except:
  fake = True
  print "Cannot import asp.  Running outside of SAFplus environment"
  import aspfake as asp
  import asppycustomfake as asppycustom
  def Session(): return None
  def Log(str,sev,stk): print str
  NodeType = 1

from misc import *
from dot import *

import xml2dict

cfgpath = os.environ.get("ASP_CONFIG",os.environ.get("ASP_BINDIR","."))

RefreshInterval = 1.9

moduleLogSeverityCutoff = asp.CL_LOG_SEV_WARNING

class ClusterInfo:
  """This singleton class represents the current cluster state and configuration; use the @ref clusterinfo.ci global variable instead of creating one yourself.

  The class reloads itself from the underlying hardware only ON DEMAND -- see the @ref load(), and @ref refresh() functions.  To implement this reload atomically (and therefore guarantee a consistent cluster view), it uses the following "trick" described below.

  All member variables are actually members of a child object called "d" (of type @ref clusterinfo.data), and the "getattr" member function is overloaded to automatically translate access through the this child. Therefore the member variables may not appear in some Python idioms such as this.__dict__.  When load() is called, a completely NEW set of member objects are created by creating a new data object and then setting the "d" member variable to this object.

  This means that references to any objects, entities, etc that you hold will never change.  In order to get updated copies, you must first call ci.log() and then access the new copies through the ci global variable.  This system makes writing software simpler because you can rely on objects not disappearing during some computation (unless you do a load()), even though the underlying entity in the AMF might have disappeared.

  Note that this class also uses a file called "clustercfg.xml" located in the "asp/etc" directory that supplies data about nodes that is not accessible from the AMF (see the \ref addSuppliedData function).  This data includes IP addresses, usernames and passwords that are necessary to deploy software.
  """

  ## @var entities
  #  Dictionary to access any entity (@ref aspAmfEntity.AmfEntity) by AMF name
 
  ## @var nodes   
  #  Dictionary to access nodes (@ref aspAmfEntity.AmfNode) by slot, name or IP address

  ## @var nodeList   
  #  List of all nodes (@ref aspAmfEntity.AmfNode)

  ## @var sgs   
  #  Dictionary to access service groups (@ref aspAmfEntity.AmfServiceGroup) by name

  ## @var sgList   
  #  List of all service groups (@ref aspAmfEntity.AmfServiceGroup)
 
  ## @var  sus   
  #  Dictionary to access service units (@ref aspAmfEntity.AmfServiceUnit) by name

  ## @var suList   
  #  List of all service units (@ref aspAmfEntity.AmfServiceUnit)

  ## @var sis   
  #  Dictionary to access service instances (@ref aspAmfEntity.AmfServiceInstance) by name

  ## @var siList   
  #  List of all service instances (@ref aspAmfEntity.AmfServiceInstance)

  ## @var comps   
  #  Dictionary to access components (@ref aspAmfEntity.AmfComponent) by name

  ## @var compList   
  #  List of components (@ref aspAmfEntity.AmfComponent)

  ## @var d
  #  All the cluster data -- only use to take a thread-safe "snapshot" of the cluster state.

  ## @var lock
  #  Thread lock -- do not use

  ## @var associatedDataKey
  #  The key to use in the AMF to access clusterinfo specific data associated with each entity.  Do not use.

  def neverCallMeDoxygenTrick():
    self.entities = None
    self.nodes = None
    self.nodeList = None
    self.sgs = None
    self.sgList = None
    self.sus = None
    self.suList = None
    self.sis = None
    self.siList = None
    self.comps = None
    self.compList = None
    raise "NEVER CALL ME -- This function is just to trigger doxygen documentation of not-explicitly declared member variables"

  def __init__(self,appDb=None,associatedDataKey=None):
    self.amf = Session()
    self.appDb = appDb
    self.associatedDataKey = associatedDataKey
    self.cfgfile =   cfgpath + os.sep + "clustercfg.xml"
    self.d       = None
    self.lock    = threading.RLock()

    try:
      self.suppliedData = xml2dict.decode_file(self.cfgfile).cluster_config
      print self.suppliedData
    except IOError: # If the supplied data file does not exist just print a warning
      self.log("Supplied data file %s does not exist.  You may need to enter node info manually" % self.cfgfile,asp.CL_LOG_SEV_WARNING)
      self.suppliedData = Dot()
  
    self.load()

  def retrieve_css(self):
    return []
  def retrieve_javascript(self):
    return []

  def log(self,s,severity=asp.CL_LOG_SEV_DEBUG):
    """Private, log wrapper for clusterinfo class"""
    #print str(s)
    if moduleLogSeverityCutoff < severity: return False
    return Log(str(s),severity,2)

  def setAppDb(self,appDb):
    """Tell me about the application database
    @param appDb Object of type @ref aspApp.AppDb
    """
    self.appDb = appDb
   
  def addSuppliedData(self,d,below=None):
    """Tell me about data from other sources (like node usernames/passwords)
    @param d     Dictionary of key/value pairs to be added to the data
    @param below (Optional) location to add the Supplied data
    """
    dict = self.suppliedData
    if below:
      for b in below:
        try:
          dict = dict[b]
        except KeyError:  # If it doesn't have this key, create it
          dict[b] = Dot()
          dict = dict[b]
    dict.update(d)

  def refresh(self):
    """Reload the AMF database if it hasn't been reloaded recently (within the \ref RefreshInterval time)"""
    global RefreshInterval
    if (time.time() - self.lastReload) > RefreshInterval: self.load()

  def load(self):
    """Load or re-load the AMF database into Python objects.  Note that this is an expensive operation, so unless you MUST
    have the latest it is better to use @ref refresh.
    """
    n = data(self.amf, self.suppliedData, self.appDb, self.associatedDataKey)
    worked = 0

    while worked < 4:  # Loop around if there is an inconsistency in the AMF db
      worked += 1
      try:
        n.load(self.d)
        worked = 100000
      except KeyError, e:
        if worked > 3: raise
      time.sleep(.5)

    self.lock.acquire()
    try:
      self.d = n
    finally:
      self.lock.release()

  def __getattr__(self,name):
    """Get the cluster data"""
    try:
      self.lock.acquire()
      t = self.d.__dict__[name]
    finally:
      self.lock.release()
    return t 

  def getDependentEntities(self,entity):
    """Returns a list of entities that contains this entity and all children/dependents.
       Dependents are entities that would have no meaning if this entity was deleted.
       @param entity String name of the entity or @ref aspAmfEntity object
    """
    if type(entity) is StringType:
      entity = self.entities[entity]

    ret = [entity]
    if entity.type == NodeType:
      for su in entity.su:
        ret += self.getDependentEntities(su)
    if entity.type == ServiceGroupType:
      for e in entity.su + entity.si:
        ret += self.getDependentEntities(e)
    if entity.type == ServiceUnitType:
      for e in entity.comp:  # SIs are not dependent on SUs because they can be reassigned (+ ent.si)
        ret += self.getDependentEntities(e)
    if entity.type == ServiceInstanceType:
      for e in entity.csi:
        ret += self.getDependentEntities(e)

    return ret

  def redundancyMigrate(self,sgName,prefix,active,standby):
    """Change redundancy modes using the underlying C clAmsMgmtMigrateSG "one-stop-shopping" function
    Note that you can also modify redundancy modes by hand (add Service Units) using the Python layers directly.
    This may be necessary if this function does not provide that exact functionality you need.
    See \ref aspAmfEntity and \ref aspAmfModify APIs

    @param sgName A string containing the name of the Service Group to be modified
    @param prefix A string containing the basename for any newly created entities
    @param active The desired number of active SUs in this SG  (SUs will be added or removed to match this value)
    @param standby The desired number of standby SUs in this SG (SUs will be added or removed to match this value)
    """

    compCmd = self.entities[sgName].su[0].comp[0].command()  # Grab the command from any other component in the SG.
    entRefList = self.amf.RedundancyMigrate(sgName,prefix,active,standby)
    self.load()
    newEntities = [self.entities[x[1]] for x in entRefList]
    for entity in newEntities:
      if entity.type == ComponentType:
        entity.instantiateCommand = compCmd
        self.amf.InstallCompConfig(entity)
    return newEntities

 

class data:
  """This helper class contains all of the changing cluster data, and is used to ensure that updates occur atomically and quickly.
  Do not create directly!

  In general, you will want to access cluster data via the ci global variable.  However, if you want to take a top-down snapshot
  of the entire cluster state, then you may do something like:

  snapShotState = ci.d
  
  And then use "snapShotState" in a similar manner to ci:

  print [x.name for x in snapShotState.entities.values()]
  """

  ## @var entities
  #  Dictionary to access any entity by AMF name
 
  ## @var nodes   
  #  Dictionary to access nodes by slot, name or IP address

  ## @var nodeList   
  #  List of all nodes

  ## @var sgs   
  #  Dictionary to access service groups by name

  ## @var sgList   
  #  List of all service groups
 
  ## @var  sus   
  #  Dictionary to access service units by name

  ## @var suList   
  #  List of all service units

  ## @var sis   
  #  Dictionary to access service instances by name

  ## @var siList   
  #  List of all service instances

  ## @var comps   
  #  Dictionary to access components by name

  ## @var compList   
  #  List of components

  ## @var associatedDataKey
  #  The key to use in the AMF to access clusterinfo specific data associated with each entity.  Do not use.


  def __init__(self,amf, suppliedData,appDb,dataKey):
    self.clear() 
    self.amf   = amf
    self.appDb = appDb
    self.suppliedData = suppliedData   
    self.associatedDataKey = dataKey

  def log(self,s,severity=asp.CL_LOG_SEV_DEBUG):
    #print str(s)
    return Log(str(s),severity,2)

  def clear(self):
    """Remove all AMF database Python objects """

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

  def load(self,oldObj=None):
    """Load or re-load the AMF database into Python objects """
    self.clear()  # Blow away any old objects
    if fake: return

    s = self.amf
    
    self.loadNodes(oldObj)  # Nodes pull data from a variety of sources and so are more complex

    self.loadSgs()

    self.log("Loading SU config and status")
    suEntRefs = s.GetConfiguredEntities(ServiceUnitType)
    for su in suEntRefs:
      config = asp.clAmsMgmtServiceUnitGetConfig(s.hdl,su[1])
      status = asp.clAmsMgmtServiceUnitGetStatus(s.hdl,su[1])
      asu = AmfServiceUnit(su[1],None,None)
      asu.setConfig(config)
      asu.setStatus(status)

      self.suList.append(asu)
      self.sus[su[1]] = asu
      self.entities[su[1]] = asu
    self.suList.sort()

    self.log("Loading SI config and status")
    siEntRefs = s.GetConfiguredEntities(WorkAssignmentType)
    for si in siEntRefs:
      config = asp.clAmsMgmtServiceInstanceGetConfig(s.hdl,si[1])
      status = asp.clAmsMgmtServiceInstanceGetStatus(s.hdl,si[1])
      asi = AmfServiceInstance(si[1])
      asi.setConfig(config)
      asi.setStatus(status)

      self.siList.append(asi)
      self.sis[si[1]] = asi
      self.entities[si[1]] = asi
    self.siList.sort()

    self.log("Loading CSI config and status")
    csiEntRefs = s.GetConfiguredEntities(ComponentWorkAssignmentType)
    nvbuf = asp.ClAmsCSINVPBufferT()
    for csi in csiEntRefs:
      config = asp.clAmsMgmtCompServiceInstanceGetConfig(s.hdl,csi[1])
      status = asp.clAmsMgmtCompServiceInstanceGetStatus(s.hdl,csi[1])
      SetCEntity(s.entity,csi)
      asp.clAmsMgmtGetCSINVPList(s.hdl,s.entity,nvbuf)
      nvdict = {}
      for i in range(0,nvbuf.count):
        t = asp.ClAmsCSINameValuePairT_array_getitem(nvbuf.nvp,i)
        nvdict[t.paramName.value] = t.paramValue.value 
      acsi = AmfComponentServiceInstance(csi[1],nvdict)
      acsi.setConfig(config)
      acsi.setStatus(status)
      acsi.csiType = config.type.value

      self.csiList.append(acsi)
      self.csis[csi[1]] = acsi
      self.entities[csi[1]] = acsi
    self.csiList.sort()

    self.loadComps() 

    self.log("Establishing entity parent-child relationships")
    self.connectEntities()
    self.log("Loading entity associated data")
    self.loadAssociatedData()

    self.log("Loading controller nodes information")
    #Create controllers list
    self.controllers = []
    for node in self.nodeList:
      if node.safClass() == 'Controller':
         self.controllers.append(node.slot)

    #Load the master and deputy information
    master = s.GetMasterNode();
    self.master = master

    #find the deputy
    if len(self.controllers) < 2:
      self.deputy=-1
    else:
      if self.controllers[0] == self.master:
        self.deputy=self.controllers[1]
      else:
        self.deputy=self.controllers[0]

    self.log("Clusterinfo load complete")


  def loadComps(self, comps2Load=None):
    s = self.amf
    self.log("Loading Component config and status")
    if not comps2Load: compEntRefs = s.GetConfiguredEntities(ComponentType)
    else: compEntRefs = [(ComponentType,x.name) for x in comps2Load]

    for comp in compEntRefs:
      config = asp.clAmsMgmtCompGetConfig(s.hdl,comp[1])
      status = asp.clAmsMgmtCompGetStatus(s.hdl,comp[1])
      acomp = AmfComponent(comp[1])
      acomp.setConfig(config)
      acomp.setStatus(status)

      self.compList.append(acomp)
      self.comps[comp[1]] = acomp
      self.entities[comp[1]] = acomp
    self.compList.sort()

  def loadAssociatedData(self,key=None):
    """Iterates through the entity list loading all associated data into each entities' self.associatedData field.
       @param key Optional associated data key that will override the clusterinfo default key
    """
    if key is None: key = self.associatedDataKey
    if key is not None:
      ckey = asp.ClNameT()
      setCNameT(ckey,key)
    
    self.log("Loading associated data using key [%s]" % key)

    for (n,e) in self.entities.items():
      SetCEntity(self.amf.entity,e)
      try:
        if key is None:
          result = asp.clAmsMgmtEntityUserDataGet(self.amf.hdl,self.amf.entity)
        else:
          result = asp.clAmsMgmtEntityUserDataGetKey(self.amf.hdl,self.amf.entity,ckey)
        self.log("Associated data result [%s]" % str(result))
      except SystemError,ex:
        if ErrCode(ex[0]) != asp.CL_ERR_NOT_EXIST:  # Associated data may not exist...
          self.log("Associated data key '%s'.  Exception on entity %s. Return Code: 0x%x" % (key, e.name,ex[0]))
          raise
        result = None

      if result:
        e.associatedData = result
        self.log("Entity [%s] data [%s]" % (str(e.name),str(e.associatedData)))
        try:
          e.unpickleAssociatedData(result)
        except AttributeError:  # No parser defined
          pass

    if key is not None: del ckey

    if self.appDb: 
      self.log("Connecting Service Groups to Applications")
      self.appDb.ConnectToServiceGroups(self)
      self.log("SG app connect done")

    self.lastReload = time.time()


  
  def connectEntities(self):
    """Private: This routine connects all entities parent/child relationships using the underlying C parent field"""

    # Go thru all components and hook them to SUs

    # components
    for i in self.compList:
      try:
        parentName = getCNameT(i.cconfig.parentSU.entity.name)
      except AttributeError, e:
        self.log("Comp [%s] has an inconsistency [%s], reloading" % (i.name,str(e)))
        self.loadComps([i])
        parentName = getCNameT(i.cconfig.parentSU.entity.name) # Try again

      if not parentName == "ParentSUUndefined":
        parent = self.entities[parentName]
        if parent:
          if i not in parent.comp: parent.comp.append(i)
          i.su = parent
      else:
        self.log("Comp [%s] has nonexistent parent [%s]" % (i.name,parentName))

      # Hook the possible CSIs to the component
      for j in range(0,i.cconfig.numSupportedCSITypes):
        name = asp.ClNameT_array_getitem(i.cconfig.pSupportedCSITypes,j).value
        i.supportedCsis.add(name)


    # SUs
    for i in self.suList:
      parentSgName = getCNameT(i.cconfig.parentSG.entity.name)
      if not parentSgName == "ParentSGUndefined":
        parentSg = self.entities[parentSgName]
        if i not in parentSg.su: parentSg.su.append(i)
        i.sg = parentSg
      else:
        self.log("SU [%s] has nonexistent parent [%s]" % (i.name,parentSgName))

      parentNodeName = getCNameT(i.cconfig.parentNode.entity.name)
      if not parentNodeName == "ParentNodeUndefined":
        parentNode =  self.entities[parentNodeName]
        if i not in parentNode.su: parentNode.su.append(i)
        i.node = parentNode
      else:
        self.log("SU [%s] has nonexistent parent [%s]" % (i.name,parentNodeName))


    for i in self.siList:
      parentSgName = getCNameT(i.cconfig.parentSG.entity.name)
      parentSg = self.entities.get(parentSgName,None)
      if not parentSg:
        self.log("SI [%s] has nonexistent parent [%s]" % (i.name,parentSgName))
      else:
        if i not in parentSg.si: parentSg.si.append(i)
        i.sg = parentSg

    for i in self.csiList:
      parentName = getCNameT(i.cconfig.parentSI.entity.name)
#      if parentName != "ParentSIUndefined":  # Not hooked up at the C level
#        parent = self.entities[parentName]
      parent = self.entities.get(parentName,None)
      if parent:
        if i not in parent.csi: parent.csi.append(i)
        i.si = parent
      else:
        self.log("CSI [%s] has nonexistent parent [%s]" % (i.name,parentName))


      
    wa = self.alive  #self.amf.GetAllWorkAssignments()
    for (nodename,ndict) in wa.items():
      for (suname,sudict) in ndict.items():
        if type(sudict) is DictType:  # Its actually an SU instead of data about the node
          for (workname,wdict) in sudict.items():
            try:
              work = self.entities[workname]
              su = self.entities[suname]
              su.si.append(work)  # Add work to su list
              if wdict["csiHaState"] == "Active":
                work.activeSu = su
              else: work.standbySu = su
            except KeyError, e:  # Screwed up AMF DB...
              self.log("'Alive' returned nonexistent entity, error %s" % str(e))    


  def loadNodes(self,oldObj=None):
    """Private:  Reloads the node information from the C layers (call load() instead)"""
    self.log("Loading node config and status")
    if fake: return
    nodes = asppycustom.GetConfiguredEntities(NodeType)
    
    self.log("Loading live node list")
    #livenodelist = asppycustom.GetRunningNodeList()
    livenodes = self.alive = self.amf.GetAllWorkAssignments()
    self.log("Loading live nodes complete")

    for node in nodes:
      name = node[1]

      liveInfo = livenodes.get(name,None)

      # Create the node
      if liveInfo: nodeSlot = liveInfo['slot']
      else: nodeSlot = 0
      n = AmfNode(name,nodeSlot)

      try:
        installInfo = asp.clAmsMgmtGetAspInstallInfo(self.amf.hdl,name,255)        
        installInfo = dict([x.split("=") for x in installInfo.split(",")])
        n.setIntraclusterAccess(installInfo['interface'].split(":")[1],None,None,installInfo.get('dir',None))
        n.aspVersion = installInfo['version']
      except SystemError,e:
        pass
      
      # Add the configured info
      try:
        nsd = self.suppliedData.nodes[name]
      except KeyError:
        self.log("Supplied configuration does not contain blade definition [%s]" % name)
        nsd = None
      except AttributeError:  # it doesn't even have .nodes
        self.log("Supplied configuration does not contain any blade definitions")
        nsd = None

      # Add the AMF status and config
      config = asp.clAmsMgmtNodeGetConfig(self.amf.hdl,name)
      status = asp.clAmsMgmtNodeGetStatus(self.amf.hdl,name)
      n.setConfig(config)
      n.setStatus(status)

      if nsd:
        # Set Intracluster Access
        n.setIntraclusterAccess(nsd.ip,nsd.user,nsd.password,nsd.aspdir)
        # Allow it to be indexed by IP 
        self.nodes[nsd.ip] = n

      if oldObj:
        oldnode=oldObj.entities.get(name,None)
        if oldnode:
          n.copyLocalState(oldnode)

      # Allow it to be indexed by name
      self.nodes[name] = n
      self.entities[name] = n
      # Allow it to be indexed by slot (if it has one)
      if n.slot: self.nodes[n.slot] = n
      # Allow an iterator to hit only the nodes
      self.nodeList.append(n)
    self.nodeList.sort()




  def loadSgs(self):
    """Private:  Reloads the service group from the C layers (call load() instead)"""
    self.log("Loading SG config and status")
    sgs   = asppycustom.GetConfiguredEntities(ServiceGroupType)
    for sg in sgs:
      name = sg[1]
      # Create the entity
      asg = AmfServiceGroup(name)
      config = asp.clAmsMgmtServiceGroupGetConfig(self.amf.hdl,name)
      asg.setConfig(config)
      state  = asp.clAmsMgmtServiceGroupGetStatus(self.amf.hdl,name)
      asg.setState(state)  

      # Allow it to be indexed by name
      self.sgs[name] = asg
      self.entities[name] = asg
      # Allow an iterator to hit only the SGs
      self.sgList.append(asg)
    self.sgList.sort()

  def calcCompToCsiType(self,sg):
    return calcCompToCsiType(sg)

def calcCompToCsiType(sg):
  """Create a table of what components need what CsiType in a Service Group -- used during when you want to add new components to a SG.
  @param sg Service Group entity
  """
  ret = {}
  for su in sg.su:
    for comp in su.comp:
      prog = comp.command()
      prog = prog.split()[0]
      prog = os.path.basename(prog)
      if ret.has_key(prog):
        ret[prog].union(comp.supportedCsis)
      else:
        ret[prog] = set(comp.supportedCsis)

  return ret


## ClusterInfo is a singleton class; this global variable is for use by everyone
ci = ClusterInfo()
